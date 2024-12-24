/**
 * @file MIPS_Pipelining.hpp
 * @author Garv Nagori and Aryan Sharma
 * Starter Code provided by Mallika Prabhakar and Sayam Sethi
 * Part of COL216 Assignment
 */

#ifndef __MIPS_79PIPELINING_HPP__
#define __MIPS_79PIPELINING_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>

struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext = 1;
	std::unordered_map<std::string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;
	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};

	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		// instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};

		for (int i = 0; i < 32; ++i)
			registerMap["$" + std::to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
		std::cout << "\nFollowing are the non-zero data values:\n";
		for (int i = 0; i < MAX / 4; ++i)
			if (data[i] != 0)
				std::cout << 4 * i << '-' << 4 * i + 3 << std::hex << ": " << data[i] << '\n'
						  << std::dec;
		std::cout << "\nTotal number of cycles: " << cycleCount << '\n';
		std::cout << "Count of instructions executed:\n";
		for (int i = 0; i < (int)commands.size(); ++i)
		{
			std::cout << commandCount[i] << " times:\t";
			for (auto &s : commands[i])
				std::cout << s << ' ';
			std::cout << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		if(line[line.length()-1] == '\n' | line[line.length()-1] == '\r') line = line.substr(0, line.length()-1);
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			// command[0] = command[0].substr(0, command[0].size() - 1);
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

    int ALU(int data1, int data2, int command, int clockCycles){
        int result; bool zero;
        switch (command){
        case 0: // nop operation
            result = 0;
            break;
        case 1: // Add operation
            result = data1 + data2;
            break;
        case 2: // Sub operation
            result = data1 - data2;
            break;
        case 3: // Mul operation
            result = data1 * data2;
            break;
        case 4: // Slt operation
            result = data1 < data2;
            break;
        case 5: // beq operation
            result = data1 == data2;
            break;
        case 6: // bne operation
            result = data1 != data2;
            break;
		case 7: // and operation
			result = data1 & data2;
			break;
		case 8: // or operation
			result = data1 | data2;
			break;
		case 9: // nor operation
			result = ~ (data1 | data2);
			break;
		case 10: // sll operation
			result = data1 << data2;
			break;
		case 11: // srl operation
			result = data1 >> data2;
			break;
        default:
            handleExit((exit_code) 3, clockCycles);
            break;
        }
        return result;
    }

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

    struct If1If2Reg{
        std::vector<std::string> instr = {"", "", "", ""};
        int RegPC = 0;
    } *If1If2 ;

    struct If2Id1Reg{
        std::vector<std::string> instr = {"", "", "", ""};
        int RegPC = 0;
    } *If2Id1 ;

    struct Id1Id2Reg{
        std::vector<std::string> instr = {"", "", "", ""};
        std::vector<bool> EXcontrol = {0,0}; // 0 - ALUSrc, 1 - RegDst
        std::vector<bool> Mcontrol = {0,0,0}; // 0 - MemWrite, 1 - MemRead, 2 - PCSrc
        std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        bool type = 0; // 0 - R, 1 - I
        std::string branch = "";
        std::string rs = "";
        std::string rt = "";
        std::string rd = "";
        int dataIm = 0;
        int ALUOp = 0;
        int RegPC = 0;
    } *Id1Id2 ;

    void clear(Id1Id2Reg *Id1Id2){
        Id1Id2->EXcontrol = {0,0}; Id1Id2->Mcontrol = {0,0,0}; Id1Id2->WBcontrol = {0,0};
        Id1Id2->type = 0; Id1Id2->branch = ""; Id1Id2->rs = ""; Id1Id2->rt = ""; Id1Id2->rd = "";
        Id1Id2->dataIm = 0; Id1Id2->ALUOp = 0; Id1Id2->RegPC = 0; Id1Id2->instr = {"", "", "", ""};
    }

    struct Id2RRReg{
        std::vector<bool> EXcontrol = {0,0}; // 0 - ALUSrc, 1 - RegDst
		std::vector<bool> Mcontrol = {0,0,0}; // 0 - MemWrite, 1 - MemRead, 2 - PCSrc
		std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        bool type = false; // 0 - R, 1 - L
        std::string branch = "";
        std::string rs = "";
        std::string rt = "";
		std::string rd = "";
        int ALUOp = 0;
        int dataIm = 0;
		int RegPC = 0;
    } *Id2RR ;

    void clear(Id2RRReg *Id2RR){
        Id2RR->EXcontrol = {0,0}; Id2RR->Mcontrol = {0,0,0}; Id2RR->WBcontrol = {0,0}; Id2RR->ALUOp = 0;
        Id2RR->branch = ""; Id2RR->dataIm = 0; Id2RR->rd = Id2RR->rs = Id2RR->rt = ""; Id2RR->type = false;
    }

    struct RRExReg{
        std::vector<bool> EXcontrol = {0,0}; // 0 - ALUSrc, 1 - RegDst
        std::vector<bool> Mcontrol = {0,0,0}; // 0 - MemWrite, 1 - MemRead, 2 - PCSrc
		std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        bool type = false; // 0 - R, 1 - I
        std::string rs = "";
        std::string rt = "";
        std::string rd = "";
        std::string branch = "";
        int ALUOp = 0;
        int data1 = 0;
        int data2 = 0;
        int dataIm = 0;
        int RegPC = 0;
    } *RREx ;

    struct ExDM1Reg{
        std::vector<bool> Mcontrol = {0,0,0}; // 0 - MemWrite, 1 - MemRead, 2 - PCSrc
		std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        std::string regDest = "";
        int writeData = 0;
        int result = 0;
        int RegPC = -1;
    } *ExDM1 ;

    struct DM1DM2Reg{
        std::vector<bool> Mcontrol = {0,0,0}; // 0 - MemWrite, 1 - MemRead, 2 - PCSrc
        std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        std::string regDest = "";
        int result = 0;
        int memData = 0;
        int writeData = 0;
        int RegPC = -1;
    } *DM1DM2 ;

    struct DM2RWReg{
        std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        std::string regDest = "";
        int memData = 0;
        int result = 0;
        int RegPC = 0;
    } *DM2RW ;

    void clear(DM2RWReg *DM2RW){
        DM2RW->WBcontrol = {0,0}; DM2RW->regDest = ""; DM2RW->memData = 0; DM2RW->result = 0;
    }

	std::tuple<int, std::string> locateAddress (std::string location){
		if(location.back() == ')'){
			try{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg)) return std::make_tuple(-3, "");
				// int address = registers[registerMap[reg]];
				// if ((address+offset) % 4 || (address+offset) < int(4 * commands.size()) || address >= MAX) return std::make_tuple(-3, "");
				return std::make_tuple(offset, reg);
			}
			catch (std::exception &e){ return std::make_tuple(-4, "");}
		}
		try{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX) return std::make_tuple(-3, "");
			return std::make_tuple(0, location);
		}
		catch (std::exception &e){ return std::make_tuple(-4, "");}
	}

	void ControlUnit  (std::vector<std::string> instr, Id1Id2Reg* Id1Id2){
		Id1Id2->rt = instr[3]; Id1Id2->rd = instr[1]; Id1Id2->rs = instr[2];
		if(instr[0] ==  "add"){
			Id1Id2->ALUOp = 1; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
            Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "addi"){
			Id1Id2->ALUOp = 1; Id1Id2->branch = ""; Id1Id2->dataIm = stoi(instr[3]); Id1Id2->type = 0;
			Id1Id2->EXcontrol = {1, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1}; Id1Id2->rt = "$31";
		}
		else if(instr[0] ==  "sub"){
			Id1Id2->ALUOp = 2; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "mul"){
			Id1Id2->ALUOp = 3; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "slt"){
			Id1Id2->ALUOp = 4; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "and"){
			Id1Id2->ALUOp = 7; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "or"){
			Id1Id2->ALUOp = 8; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "nor"){
			Id1Id2->ALUOp = 9; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "andi"){
            Id1Id2->ALUOp = 7; Id1Id2->branch = ""; Id1Id2->dataIm = stoi(instr[3]); Id1Id2->type = 0;
			Id1Id2->EXcontrol = {1, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1}; Id1Id2->rt = "$31";
		}
		else if(instr[0] ==  "ori"){
			Id1Id2->ALUOp = 8; Id1Id2->branch = ""; Id1Id2->dataIm = stoi(instr[3]); Id1Id2->type = 0;
			Id1Id2->EXcontrol = {1, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1}; Id1Id2->rt = "$31";
		}
		else if(instr[0] ==  "sll"){
			Id1Id2->ALUOp = 10; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "srl"){
			Id1Id2->ALUOp = 11; Id1Id2->branch = ""; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 1};
		}
		else if(instr[0] ==  "lw"){
			Id1Id2->ALUOp = 1; Id1Id2->branch = ""; Id1Id2->rt = ""; Id1Id2->type = 1;
			Id1Id2->EXcontrol = {1, 0}; Id1Id2->Mcontrol = {0, 1, 0}; Id1Id2->WBcontrol = {1, 1};
			std::tie(Id1Id2->dataIm, Id1Id2->rs) = locateAddress(instr[2]);
		}
		else if(instr[0] == "sw"){
			Id1Id2->ALUOp = 1; Id1Id2->branch = ""; Id1Id2->rt = instr[1]; Id1Id2->type = 1;
			Id1Id2->EXcontrol = {1, 0}; Id1Id2->Mcontrol = {1, 0, 0}; Id1Id2->WBcontrol = {1, 0};
			std::tie(Id1Id2->dataIm, Id1Id2->rs) = locateAddress(instr[2]);
		}
        else if(instr[0] == "j"){
            Id1Id2->ALUOp = 0; Id1Id2->branch = instr[1]; Id1Id2->dataIm = 0; Id1Id2->type = 0;
            Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 0}; Id1Id2->WBcontrol = {0, 0};
            Id1Id2->rs = "$0"; Id1Id2->rt = "$0"; Id1Id2->rd = "$0";
        }
		else if(instr[0] == "beq"){
			Id1Id2->ALUOp = 5; Id1Id2->branch = instr[3]; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 1}; Id1Id2->WBcontrol = {0, 0};
			Id1Id2->rs = instr[1]; Id1Id2->rt = instr[2]; Id1Id2->rd = "";
		}
		else if(instr[0] == "bne"){
			Id1Id2->ALUOp = 6; Id1Id2->branch = instr[3]; Id1Id2->dataIm = 0; Id1Id2->type = 0;
			Id1Id2->EXcontrol = {0, 0}; Id1Id2->Mcontrol = {0, 0, 1}; Id1Id2->WBcontrol = {0, 0};
			Id1Id2->rs = instr[1]; Id1Id2->rt = instr[2]; Id1Id2->rd = "";
		}
		else Id1Id2->ALUOp = -4;
    }

	void executeCommands79Pipelined()
	{
        If1If2Reg *If1If2 = new If1If2Reg();
        If2Id1Reg *If2Id1 = new If2Id1Reg();
        Id1Id2Reg *Id1Id2 = new Id1Id2Reg();
        Id2RRReg *Id2RR = new Id2RRReg();
        RRExReg *RREx = new RRExReg();
        ExDM1Reg *ExDM1 = new ExDM1Reg();
        DM1DM2Reg *DM1DM2 = new DM1DM2Reg();
        DM2RWReg *DM2RW = new DM2RWReg();
		std::string memChange = "";
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
		bool halfCycle = false; // false corresponds to first half cycle, true corresponds to second half cycle
		int brstall = 0, brnext = 0;
		while (!(PCcurr >= commands.size() && If1If2->RegPC == -1 && If2Id1->RegPC == -1 && Id1Id2->RegPC == -1 && Id2RR->RegPC == -1 && RREx->RegPC == -1 && ExDM1->RegPC == -1 && DM1DM2->RegPC == -1 && DM2RW->RegPC == -1)){
			++clockCycles;
			halfCycle = false;

			// RW stage
			if(clockCycles>=7 && DM2RW->RegPC != -1){
				int writeData = (DM2RW->WBcontrol[0]) ? DM2RW->memData : DM2RW->result;
				if(DM2RW->WBcontrol[1]){
					if (!checkRegister(DM2RW->regDest)){
						handleExit((exit_code) 1, clockCycles);
						return;
					}
					registers[registerMap[DM2RW->regDest]] = writeData;
				}
			}

            // DM2 stage
            if(clockCycles>=8 && DM1DM2->RegPC != -1){
                DM2RW->memData = DM1DM2->memData; DM2RW->regDest = DM1DM2->regDest;
                if(DM1DM2->Mcontrol[0]){
					if (DM1DM2->result < 0){
						handleExit( (exit_code) abs(DM1DM2->result), clockCycles);
						return;
					}
					data[(DM1DM2->result)/4] = DM1DM2->writeData;
					DM2RW->memData = 0;
					memChange = std::to_string((DM1DM2->result)/4) + " " + std::to_string(DM1DM2->writeData);
				}
                DM2RW->WBcontrol = DM1DM2->WBcontrol; DM2RW->result = DM1DM2->result; DM2RW->regDest = DM1DM2->regDest;
            }
            DM2RW->RegPC = DM1DM2->RegPC; DM1DM2->RegPC = -1;

			halfCycle = true;

			// DM1 stage
			if(clockCycles>=7 && ExDM1->RegPC != -1){
                DM1DM2->memData = 0; DM1DM2->Mcontrol = ExDM1->Mcontrol; DM1DM2->writeData = ExDM1->writeData;
				if(ExDM1->Mcontrol[1]){
					if (ExDM1->result < 0){
						handleExit( (exit_code) abs(ExDM1->result), clockCycles);
						return;
					}
					DM1DM2->memData = data[(ExDM1->result)/4];
				}
				DM1DM2->WBcontrol = ExDM1->WBcontrol; DM1DM2->result = ExDM1->result; DM1DM2->regDest = ExDM1->regDest;
			}
			DM1DM2->RegPC = ExDM1->RegPC; ExDM1->RegPC = -1;

            // Ex Stage
            if(clockCycles>=6 && RREx->RegPC != -1){
                int dataR = (RREx->EXcontrol[0])? RREx->dataIm : RREx->data2;
                int result = ALU(RREx->data1, dataR, RREx->ALUOp, clockCycles);
                std::string regDest = (RREx->EXcontrol[1])? RREx->rt : RREx->rd;

                if(RREx->Mcontrol[2]){
					if(result == 1){
		        		if (address.find(RREx->branch) == address.end() || address[RREx->branch] == -1){
                		    handleExit( (exit_code) 2, clockCycles);
                		    return;
                		}
						PCcurr =  address[RREx->branch];
					}
					else{ PCcurr = RREx->RegPC + 1;}
                    clear(DM2RW); DM2RW->RegPC = RREx->RegPC;
                    If1If2->RegPC = If2Id1->RegPC = Id1Id2->RegPC = Id2RR->RegPC = RREx->RegPC = -1;
					printRegisters(clockCycles);
					if(memChange != ""){
						std::cout<<"1 "<<memChange<<std::endl;
						memChange = "";
					}
					else{ std::cout<<"0\n"; }
					continue;
				}

                if(RREx->type){
                    ExDM1->Mcontrol = RREx->Mcontrol; ExDM1->WBcontrol = RREx->WBcontrol; ExDM1->result = result;
                    ExDM1->regDest = regDest; ExDM1->writeData = RREx->data2; ExDM1->RegPC = RREx->RegPC;
                }
                else{
                    DM2RW->result = result; DM2RW->memData = 0; DM2RW->regDest = regDest; DM2RW->WBcontrol = RREx->WBcontrol;
                    DM2RW->RegPC = RREx->RegPC;
                }
			}
            else if(clockCycles>=6 && RREx->RegPC == -1 && RREx->type == 0){
                DM2RW->RegPC = -1;
            }
            else if(clockCycles>=6 && RREx->RegPC == -1 && RREx->type == 1){
                ExDM1->RegPC = -1;
            }

            // RR stage
            if(clockCycles >= 5 && Id2RR->RegPC != -1){
                RREx->data1 = registers[registerMap[Id2RR->rs]]; RREx->data2 = registers[registerMap[Id2RR->rt]];
                RREx->ALUOp = Id2RR->ALUOp; RREx->dataIm = Id2RR->dataIm; RREx->Mcontrol = Id2RR->Mcontrol; RREx->type = Id2RR->type;
                RREx->rd = Id2RR->rd; RREx->rs = Id2RR->rs; RREx->rt = Id2RR->rt; RREx->WBcontrol = Id2RR->WBcontrol; 
                RREx->EXcontrol = Id2RR->EXcontrol; RREx->branch = Id2RR->branch;
            }
            RREx->RegPC = Id2RR->RegPC;

            // Id2 stage
            if(clockCycles >= 4 && Id1Id2->RegPC != -1){
                if(Id1Id2->instr[0] == "j"){
                    if (address.find(Id1Id2->instr[1]) == address.end() || address[Id1Id2->instr[1]] == -1){
						handleExit((exit_code) 2, clockCycles);
						return;
					}
                    PCcurr = address[Id1Id2->instr[1]]; clear(Id2RR);
                    If1If2->RegPC = If2Id1->RegPC = Id1Id2->RegPC = -1;
					printRegisters(clockCycles);
					if(memChange != ""){
						std::cout<<"1 "<<memChange<<std::endl;
						memChange = "";
					}
					else{ std::cout<<"0\n"; }
					continue;
                }

                // Stalling for hazard
                bool RWhazard = ((DM2RW->RegPC != -1) && (DM2RW->WBcontrol[1]) && (registerMap[DM2RW->regDest] == registerMap[Id1Id2->rs] || registerMap[DM2RW->regDest] == registerMap[Id1Id2->rt]));
                bool DM2hazard = ((DM1DM2->RegPC != -1) && (DM1DM2->WBcontrol[1]) && (registerMap[DM1DM2->regDest] == registerMap[Id1Id2->rs] || registerMap[DM1DM2->regDest] == registerMap[Id1Id2->rt])); 
                bool DM1hazard = ((ExDM1->RegPC != -1) && (ExDM1->WBcontrol[1]) && (registerMap[ExDM1->regDest] == registerMap[Id1Id2->rs] || registerMap[ExDM1->regDest] == registerMap[Id1Id2->rt]));
                bool Exhazard = ((RREx->RegPC != -1) && (RREx->WBcontrol[1]) && (registerMap[RREx->rd] == registerMap[Id1Id2->rs] || registerMap[RREx->rd] == registerMap[Id1Id2->rt]) && (RREx->EXcontrol[1] == 0));
                if(RWhazard || DM2hazard || DM1hazard || Exhazard){
                    Id2RR->ALUOp = 0; Id2RR->branch = ""; Id2RR->dataIm = 0; Id2RR->RegPC = -1;
                    Id2RR->EXcontrol = {0,0}; Id2RR->Mcontrol = {0,0,0}; Id2RR->WBcontrol = {0,0,0};
                    Id2RR->rs = Id2RR->rs = Id2RR->rt = ""; Id2RR->type = 0;
					printRegisters(clockCycles);
					if(memChange != ""){
						std::cout<<"1 "<<memChange<<std::endl;
						memChange = "";
					}
					else{ std::cout<<"0\n"; }
                    continue;
                }

                Id2RR->rs = Id1Id2->rs; Id2RR->rt = Id1Id2->rt; Id2RR->rd = Id1Id2->rd; Id2RR->dataIm = Id1Id2->dataIm;
                Id2RR->ALUOp = Id1Id2->ALUOp; Id2RR->Mcontrol = Id1Id2->Mcontrol; Id2RR->WBcontrol = Id1Id2->WBcontrol;
                Id2RR->type = Id1Id2->type; Id2RR->EXcontrol = Id1Id2->EXcontrol; Id2RR->branch = Id1Id2->branch;
            }
            Id2RR->RegPC = Id1Id2->RegPC;

            // Id1 Stage
            if(clockCycles >= 3 && If2Id1->RegPC != -1){
                ControlUnit(If2Id1->instr, Id1Id2);
                if(Id2RR->RegPC != -1 && Id1Id2->type == 0 && Id2RR->type == 1){
                    PCcurr = If2Id1->RegPC; clear(Id1Id2); 
                    Id1Id2->RegPC = If2Id1->RegPC = If1If2->RegPC = -1;
                }
                Id1Id2->instr = If2Id1->instr;
            }
            Id1Id2->RegPC = If2Id1->RegPC;

            // If2 stage
            if(clockCycles >= 2 && If1If2->RegPC != -1){
                If2Id1->instr = If1If2->instr;
            }
            If2Id1->RegPC = If1If2->RegPC;

			// If1 stage
			if((PCcurr < commands.size())){
				If1If2->instr = commands[PCcurr];
				If1If2->RegPC = PCcurr;
			}
			else{ If1If2->RegPC = -1; }
			PCcurr++;
			printRegisters(clockCycles);
			if(memChange != ""){
				std::cout<<"1 "<<memChange<<std::endl;
				memChange = "";
			}
			else{ std::cout<<"0\n"; }
		}	
	}

	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		for (int i = 0; i < 32; i++)
			std::cout << registers[i] << ' ';
		std::cout << '\n';
	}
};

#endif