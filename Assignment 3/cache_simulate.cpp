#include<iostream>
#include<fstream>
#include<bits/stdc++.h>
#define ulli unsigned long long int

using namespace std;

struct Cache{

    ulli L2_read_hits = 0, L2_read_misses = 0, L2_write_hits = 0, L2_write_misses = 0, L2_write_backs = 0;
    ulli L1_read_hits = 0, L1_read_misses = 0, L1_write_hits = 0, L1_write_misses = 0, L1_write_backs = 0;

    struct Line{
        ulli tag, dirty, lru, valid;
        Line(){
            tag = 0;
            dirty = 0;
            lru = 0;
            valid = 0;
        }
    };

    Line** L2; ulli L2_totalsetno, L2_assoc, L2_byteoffset;
    Line** L1; ulli L1_totalsetno, L1_assoc, L1_byteoffset;

    Cache(ulli blocksize, ulli L1size, ulli L1assoc, ulli L2size, ulli L2assoc){
        L2_totalsetno = L2size/(blocksize*L2assoc); L2_assoc = L2assoc; L2_byteoffset = (ulli)log2(blocksize);
        L2 = new Line*[L2_totalsetno];
        for(ulli i=0; i<L2_totalsetno; i++){
            Line* temp = new Line[L2_assoc];
            L2[i] = temp;
        }

        L1_totalsetno = L1size/(blocksize*L1assoc); L1_assoc = L1assoc; L1_byteoffset = (ulli)log2(blocksize);
        L1 = new Line*[L1_totalsetno];
        for(ulli i=0; i<L1_totalsetno; i++){
            Line* temp = new Line[L1_assoc];
            L1[i] = temp;
        }
    }

    void updateLRU(Line* block, ulli assoc){
        for(ulli j=0; j<assoc; j++){
            if(block[j].valid) block[j].lru++;
        }
    }

    ulli L2_read(ulli address){
        ulli setno = (address>>L2_byteoffset)%L2_totalsetno;
        ulli tag = address >> (L2_byteoffset + (ulli)log2(L2_totalsetno));
        for(ulli i=0; i<L2_assoc; i++){
            if((L2[setno][i].tag == tag) && L2[setno][i].valid){
                updateLRU(L2[setno], L2_assoc);
                L2[setno][i].lru = 0;
                return 1;
            }
        }
        return 0;
    }

    ulli L2_write(ulli address){
        ulli setno = (address>>L2_byteoffset)%L2_totalsetno;
        ulli tag = address >> (L2_byteoffset + (ulli)log2(L2_totalsetno));
        for(ulli i=0; i<L2_assoc; i++){
            if((L2[setno][i].tag == tag) && L2[setno][i].valid){
                L2[setno][i].dirty = 1;
                updateLRU(L2[setno], L2_assoc);
                L2[setno][i].lru = 0;
                return 1;
            }
        }
        return 0;
    }

    ulli L2_replace(ulli address){
        ulli setno = (address>>L2_byteoffset)%L2_totalsetno;
        ulli tag = address >> (L2_byteoffset + (ulli)log2(L2_totalsetno));
        ulli max = 0;
        for(ulli i=0; i<L2_assoc; i++){
            if(!L2[setno][i].valid){
                max = i;
                break;
            }
            if(L2[setno][i].lru > L2[setno][max].lru){
                max = i;
            }
        }
        if(L2[setno][max].valid){
            L1_invalidate((L2[setno][max].tag << (L2_byteoffset + (ulli)log2(L2_totalsetno))) + (setno << L2_byteoffset));
            if(L2[setno][max].dirty == 1){
                L2_write_backs++;
            }
        }
        L2[setno][max].dirty = 0;
        L2[setno][max].tag = tag;
        L2[setno][max].valid = 1;
        updateLRU(L2[setno], L2_assoc);
        L2[setno][max].lru = 0;
        return 1;
    }

    ulli L2_set_dirty(ulli address){
        ulli setno = (address>>L2_byteoffset)%L2_totalsetno;
        ulli tag = address >> (L2_byteoffset + (ulli)log2(L2_totalsetno));
        for(ulli i=0; i<L2_assoc; i++){
            if((L2[setno][i].tag == tag) && L2[setno][i].valid){
                L2[setno][i].dirty = 1;
                updateLRU(L2[setno], L2_assoc);
                L2[setno][i].lru = 0;
                return 1;
            }
        }
        return 0;
    }

    ulli L1_read(ulli address){
        ulli setno = (address>>L1_byteoffset)%L1_totalsetno;
        ulli tag = address >> (L1_byteoffset + (ulli)log2(L1_totalsetno));
        for(ulli i=0; i<L1_assoc; i++){
            if((L1[setno][i].tag == tag) && L1[setno][i].valid){
                updateLRU(L1[setno], L1_assoc);
                L1[setno][i].lru = 0;
                return 1;
            }
        }
        return 0;
    }

    ulli L1_write(ulli address){
        ulli setno = (address>>L1_byteoffset)%L1_totalsetno;
        ulli tag = address >> (L1_byteoffset + (ulli)log2(L1_totalsetno));
        for(ulli i=0; i<L1_assoc; i++){
            if((L1[setno][i].tag == tag) && L1[setno][i].valid){
                updateLRU(L1[setno], L1_assoc);
                L1[setno][i].lru = 0;
                L1[setno][i].dirty = 1;
                return 1;
            }
        }
        return 0;
    }

    ulli L1_replace(ulli address){
        ulli setno = (address>>L1_byteoffset)%L1_totalsetno;
        ulli tag = address >> (L1_byteoffset + (ulli)log2(L1_totalsetno));
        ulli max = 0;
        for(ulli i=0; i<L1_assoc; i++){
            if(!L1[setno][i].valid){
                max = i;
                break;
            }
            if(L1[setno][i].lru > L1[setno][max].lru){
                max = i;
            }
        }
        if((L1[setno][max].dirty == 1) && L1[setno][max].valid){
            L1_write_backs++;
            L2_write_hits++;
            L2_set_dirty((L1[setno][max].tag << (L1_byteoffset + (ulli)log2(L1_totalsetno))) + (setno << L1_byteoffset));
            L1[setno][max].dirty = 0;
        }
        L1[setno][max].tag = tag;
        L1[setno][max].valid = 1;
        updateLRU(L1[setno], L1_assoc);
        L1[setno][max].lru = 0;
        return 1;
    }

    ulli L1_invalidate(ulli address){
        ulli setno = (address>>L1_byteoffset)%L1_totalsetno;
        ulli tag = address >> (L1_byteoffset + (ulli)log2(L1_totalsetno));
        for(ulli i=0; i<L1_assoc; i++){
            if((L1[setno][i].tag == tag) && L1[setno][i].valid){
                L1[setno][i].valid = 0;
                if(L1[setno][i].dirty == 1){
                    L1_write_backs++;
                    L2_set_dirty((L1[setno][i].tag << (L1_byteoffset + (ulli)log2(L1_totalsetno))) + (setno << L1_byteoffset));
                }
                return 1;
            }
        }
        return 0;
    }

    // L1 miss, L2 hit :- If dirty bit is set, write back to L2 else nothing for inclusive
    // Both L1, L2 miss :- Replace in both L1, L2. For L1 do as above, for L2 if block evicted send invalidate signal to L1
    // L1 read hit check :- L1 read_hit++
    // L1 read miss, L2 read hit :- L1 read_miss++, L2 read_hit++, L1 replace, if evicted block is dirty, set dirty bit in L2, L2_write_hit++
    // L1 read miss, L2 read miss :- L1 read_miss++, L2 read_miss++. 
        // L1 replace do as above, L2 replace. If L2 block evicted, send invalidate signal to L1
    void run_trace(vector<tuple<char, ulli>> trace, ulli trace_size){
        for (ulli i=0; i<trace_size; i++){
            char op = get<0>(trace[i]);
            ulli address = get<1>(trace[i]);
            if(op == 'r'){
                if(L1_read(address)){
                    //hit at L1
                    L1_read_hits++;
                }
                else if(L2_read(address)){
                    //hit at L2, miss at L1
                    L1_read_misses++;
                    L2_read_hits++;
                    L1_replace(address);
                }
                else{
                    //missed everyone
                    L1_read_misses++;
                    L2_read_misses++;
                    L2_replace(address);
                    L1_replace(address); // assumption here is that when there is a miss at both L1 and L2, the data block is brought in both the caches.
                }
            }
            else{
                if(L1_write(address)){
                    //hit at L1
                    L1_write_hits++;
                }
                else if(L2_read(address)){
                    //hit at L2, miss at L1
                    L1_write_misses++;
                    L2_read_hits++;
                    L1_replace(address);
                    L1_write(address);
                }
                else{
                    //miss
                    L1_write_misses++;
                    L2_read_misses++;
                    L2_replace(address);
                    L1_replace(address);
                    L1_write(address);
                }
            }
        }
    }
};

// changes in write need to be made in case of accounting dirty bits;
ulli hextodeci(string s){
    ulli ans = 0;
    ulli val = 1;
    int n = s.size();int i= n-1;
    while(i >= 0){
        (int(s[i]) >= 97) ? ans += val*(int(s[i]) - 87) : ans += val*(int(s[i]) - 48);
        val *= 16;
        i--;
    }
    return ans;
}

int main(int argc, char *argv[]){
    // ifstream file(argv[1]);
    // std::ifstream file("C:/IIT Delhi/Sem 4/3. COL216/Assignment 3/memory_trace_files/trace7.txt");
    std::ifstream file(argv[6]);
    // file.open("trace1.txt", ios::in);
    vector<tuple<char, ulli>> trace;
    string s;
    while(getline(file, s)){
        ulli addr = hextodeci(s.substr(2, s.size()-2));
        char op = s[0];
        trace.push_back(make_tuple(op, addr));
    }
    Cache* cache = new Cache((ulli)stoi(argv[1]), (ulli)stoi(argv[2]), (ulli)stoi(argv[3]), (ulli)stoi(argv[4]), (ulli)stoi(argv[5]));
    cache->run_trace(trace, trace.size());
    cout << "\t   ===== Simulation Results =====" << "\n";
    cout << "i. number of L1 reads:\t\t\t\t" << (cache->L1_read_hits + cache->L1_read_misses) << "\n";
    cout << "ii. number of L1 read misses:\t\t\t" << (cache->L1_read_misses) << "\n";
    cout << "iii. number of L1 writes:\t\t\t" << (cache->L1_write_hits + cache->L1_write_misses) << "\n";
    cout << "iv. number of L1 write misses:\t\t\t" << cache->L1_write_misses << "\n";
    cout << "v. L1 miss rate:\t\t\t\t" << setprecision(4) << float(cache->L1_read_misses + cache->L1_write_misses) / float(cache->L1_read_hits + cache->L1_read_misses + cache->L1_write_hits + cache->L1_write_misses) << "\n";
    cout << "vi. number of writebacks from L1 memory:\t" <<  cache->L1_write_backs << "\n";
    cout << "vii. number of L2 reads:\t\t\t" << (cache->L2_read_hits + cache->L2_read_misses) << "\n";
    cout << "viii. number of L2 read misses:\t\t\t" << cache->L2_read_misses << "\n";
    cout << "ix. number of L2 writes:\t\t\t" << (cache->L2_write_hits + cache->L2_write_misses) << "\n";
    cout << "x. number of L2 write misses:\t\t\t" << cache->L2_write_misses << "\n";
    cout << "xi. L2 miss rate:\t\t\t\t" << setprecision(4) << float(cache->L2_read_misses + cache->L2_write_misses) / float(cache->L2_read_hits + cache->L2_read_misses + cache->L2_write_hits + cache->L2_write_misses) << "\n";
    cout << "xii. number of writebacks from L2 memory:\t" <<  cache->L2_write_backs << "\n";
    cout << "xiii. Total Access Time :\t\t\t" << 1*(cache->L1_read_hits + cache->L1_read_misses + cache->L1_write_hits + cache->L1_write_misses) + 20*(cache->L2_read_hits + cache->L2_read_misses + cache->L1_write_backs) + 200*(cache->L2_read_misses + cache->L2_write_misses + cache->L2_write_backs) << "\n";
}