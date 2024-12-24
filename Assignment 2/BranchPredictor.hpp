#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__

#include <vector>
#include <bitset>
#include<bits/stdc++.h>
using namespace std;

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2> > table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        // your code here
        uint32_t val = ((pc << 18) >> 18);
        bool flag;
        std::bitset<2> set = table[val];
        (set[1]) ? flag = true : flag = false;
        return flag;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        uint32_t val = ((pc << 18) >> 18);
        std::bitset<2> set = table[val];
        int eval = 2*set[0] + set[1];
        (taken) ? eval = std::min(3, eval + 1) : eval = std::max(0, eval - 1);
        (eval > 1) ? set[0] = 1 : set[0] = 0;
        (eval%2) ? set[1] = 1 : set[1] = 0;
        table[val] = set;
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2> > bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        // your code here
        int val = 2*bhr[0] + bhr[1];
        bool flag;
        (bhrTable[val][0]) ? flag = true : flag = false; 
        return flag;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        int val = 2*bhr[0] + bhr[1];
        bhr[0] = bhr[1]; bhr[1] = taken;
        std::bitset<2> set = bhrTable[val];
        int eval = 2*set[0] + set[1];
        (taken) ? eval = std::min(3, eval + 1) : eval = std::max(0, eval - 1);
        (eval > 1) ? set[0] = 1 : set[0] = 0;
        (eval%2) ? set[1] = 1 : set[1] = 0;
        bhrTable[val] = set;
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    // strategy is to 
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    bool predict(uint32_t pc) {
        // your code here
        uint32_t val = ((pc << 18) >> 18);
        bool flag;
        std::bitset<2> set = table[val];
        if(!(set[1]^set[0])) (set[1]) ? flag =  true : flag = false; return flag;
        if(set[1]^set[0]){
            uint32_t temp = 2*bhr[0] + bhr[1];
            set = bhrTable[temp];
            if(!(set[0]^set[1])) (set[1]) ? flag = true : flag = false; return flag;
            if(set[1]^set[0]){
                temp = temp << 14;
                temp += val;
                (combination[temp][0]) ? flag = true : flag = false; return flag;
            }
        }
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        uint32_t val = 2*bhr[0] + bhr[1];
        bhr[0] = bhr[1]; bhr[1] = taken;
        std::bitset<2> set = bhrTable[val];
        int eval = 2*set[0] + set[1];
        (taken) ? eval = std::min(3, eval + 1) : eval = std::max(0, eval - 1);
        (eval > 1) ? set[0] = 1 : set[0] = 0;
        (eval%2) ? set[1] = 1 : set[1] = 0;
        bhrTable[val] = set;
        pc = ((pc << 18) >> 18);
        set = combination[pc + (val << 14)];
        eval = 2*set[0] + set[1];
        (taken) ? eval = std::min(3, eval + 1) : eval = std::max(0, eval - 1);
        (eval > 1) ? set[0] = 1 : set[0] = 0;
        (eval%2) ? set[1] = 1 : set[1] = 0;
        combination[pc + (val << 14)] = set;
        val = pc;
        set = table[val];
        eval = 2*set[0] + set[1];
        (taken) ? eval = std::min(3, eval + 1) : eval = std::max(0, eval - 1);
        (eval > 1) ? set[0] = 1 : set[0] = 0;
        (eval%2) ? set[1] = 1 : set[1] = 0;
        table[val] = set;
    }
};

#endif