#ifndef __MIPS_PIPELINING_HPP__
#define __MIPS_PIPELINING_HPP__

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
	// std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
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

    struct IfIdReg{
        std::vector<std::string> instr = {"", "", "", ""};
		int RegPC = 0;
    } *IfId ;

	void clear(IfIdReg *IfId){ IfId->instr = {"", "", "", ""}; IfId->RegPC = 0; }

    struct IdExReg{
		std::vector<bool> EXcontrol = {0,0}; // 0 - ALUSrc, 1 - RegDst
		std::vector<bool> Mcontrol = {0,0,0}; // 0 - MemWrite, 1 - MemRead, 2 - PCSrc
		std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        std::string branch = "";
        std::string rs = "";
        std::string rt = "";
		std::string rd = "";
        int data1 = 0;
        int data2 = 0;
        int dataIm = 0;
        int ALUOp = 0;
		int RegPC = 0;
    } *IdEx ;

	void clear(IdExReg *IdEx){
		IdEx->EXcontrol = {0,0}; IdEx->Mcontrol = {0,0,0}; IdEx->WBcontrol={0,0}; IdEx->RegPC = 0;
		IdEx->branch = ""; IdEx->rd = ""; IdEx->rd = ""; IdEx->data1 = IdEx->data2 = IdEx->dataIm = IdEx->ALUOp = 0;	
	}

    struct ExMReg{
		std::vector<bool> Mcontrol = {0,0}; // 0 - MemWrite, 1 - MemRead, 2 - PCSrc
		std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        std::string branch = "";
        std::string regDest = "";
		int result = 0;
        int writeData = 0;
		int PCNext = 0;
		int RegPC = 0;
    } *ExM ;

	void clear(ExMReg *ExM){
		ExM->Mcontrol = {0,0}; ExM->WBcontrol = {0,0}; ExM->branch = ""; ExM->regDest = "";
		ExM->result = ExM->writeData = ExM->result = 0; ExM->RegPC = 0;
	}

    struct MWReg{
		std::vector<bool> WBcontrol = {0,0}; // 0 - MemToReg, 1 - RegWrite
        int memData = 0;
        int result = 0;
        std::string regDest = "$s0";
		int RegPC = 0;
    } *MW ;

	void clear(MWReg *MW){ MW->WBcontrol = {0,0}; MW->regDest = "$s0"; MW->regDest = MW->result = 0; MW->RegPC = 0; }

	std::tuple<int, int, std::string> locateAddress (std::string location){
		if(location.back() == ')'){
			try{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg)) return std::make_tuple(-3, 0, "");
				int address = registers[registerMap[reg]];
				return std::make_tuple(address / 4, offset / 4, reg);
			}
			catch (std::exception &e){ return std::make_tuple(-4, 0, "");}
		}
		try{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX) return std::make_tuple(-3, 0, "");
			return std::make_tuple(address / 4, 0, "");
		}
		catch (std::exception &e){ return std::make_tuple(-4, 0, "");}
	}

	void ControlUnit  (std::vector<std::string> instr, IdExReg* IdEx){
		IdEx->rt = instr[3]; IdEx->rd = instr[1]; IdEx->rs = instr[2];
		if(instr[0] ==  "add"){
			IdEx->ALUOp = 1; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "addi"){
			IdEx->ALUOp = 1; IdEx->branch = ""; IdEx->data2 = 0;
			IdEx->EXcontrol = {1, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[1]]]; IdEx->dataIm = stoi(instr[3]);
		}
		else if(instr[0] ==  "sub"){
			IdEx->ALUOp = 2; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "mul"){
			IdEx->ALUOp = 3; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "slt"){
			IdEx->ALUOp = 4; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "and"){
			IdEx->ALUOp = 7; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "or"){
			IdEx->ALUOp = 8; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "nor"){
			IdEx->ALUOp = 9; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "andi"){
			IdEx->ALUOp = 7; IdEx->branch = ""; IdEx->data2 = 0;
			IdEx->EXcontrol = {1, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[1]]]; IdEx->dataIm = stoi(instr[3]);
		}
		else if(instr[0] ==  "ori"){
			IdEx->ALUOp = 8; IdEx->branch = ""; IdEx->data2 = 0;
			IdEx->EXcontrol = {1, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[1]]]; IdEx->dataIm = stoi(instr[3]);
		}
		else if(instr[0] ==  "sll"){
			IdEx->ALUOp = 10; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "srl"){
			IdEx->ALUOp = 11; IdEx->branch = ""; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 0}; IdEx->WBcontrol = {0, 1};
			if (!checkRegisters({instr[1], instr[2], instr[3]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[2]]]; IdEx->data2 = registers[registerMap[instr[3]]];
		}
		else if(instr[0] ==  "lw"){
			IdEx->ALUOp = 1; IdEx->branch = ""; IdEx->data2 = 0;
			IdEx->EXcontrol = {1, 0}; IdEx->Mcontrol = {0, 1, 0}; IdEx->WBcontrol = {1, 1};
			std::tie(IdEx->data1, IdEx->dataIm, IdEx->rs) = locateAddress(instr[2]);
			if(IdEx->data1 < 0){
				IdEx->ALUOp = IdEx->data1;
				return;
			}
		}
		else if(instr[0] == "sw"){
			IdEx->ALUOp = 1; IdEx->branch = "";
			if (!checkRegister(instr[1])){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data2 = registers[registerMap[instr[1]]]; IdEx->rs = instr[1];
			IdEx->EXcontrol = {1, 0}; IdEx->Mcontrol = {1, 0, 0}; IdEx->WBcontrol = {1, 0};
			std::tie(IdEx->data1, IdEx->dataIm, IdEx->rt) = locateAddress(instr[2]);
			if(IdEx->data1 < 0){
				IdEx->ALUOp = IdEx->data1;
				return;
			}
		}
		else if(instr[0] == "beq"){
			IdEx->ALUOp = 5; IdEx->branch = instr[3]; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 1}; IdEx->WBcontrol = {0, 0};
			if (!checkRegisters({instr[1], instr[2]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[1]]]; IdEx->data2 = registers[registerMap[instr[2]]];
			IdEx->rs = instr[1]; IdEx->rt = instr[2]; IdEx->rd = "";
		}
		else if(instr[0] == "bne"){
			IdEx->ALUOp = 6; IdEx->branch = instr[3]; IdEx->dataIm = 0;
			IdEx->EXcontrol = {0, 0}; IdEx->Mcontrol = {0, 0, 1}; IdEx->WBcontrol = {0, 0};
			if (!checkRegisters({instr[1], instr[2]}) || registerMap[instr[1]] == 0){
				IdEx->ALUOp = -1;
				return;
			}
			IdEx->data1 = registers[registerMap[instr[1]]]; IdEx->data2 = registers[registerMap[instr[2]]];
			IdEx->rs = instr[1]; IdEx->rt = instr[2]; IdEx->rd = "";
		}
		else IdEx->ALUOp = -4;
    }

	void executeCommandsPipelined()
	{
		MWReg *MW = new MWReg();
		ExMReg *ExM = new ExMReg();
		IdExReg *IdEx = new IdExReg();
		IfIdReg *IfId = new IfIdReg();
		std::string memChange = "";
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
		bool halfCycle = false; // false corresponds to first half cycle, true corresponds to second half cycle
		int brstall = 0, brnext = 0;
		while (!(PCcurr >= commands.size() && IfId->RegPC == -1 && IdEx->RegPC == -1 && ExM->RegPC == -1 && MW->RegPC == -1)){
			++clockCycles;
			halfCycle = false;
			
			// W stage
			if(clockCycles>=5 && (PCcurr < (commands.size()+4)) && MW->RegPC != -1){
				int writeData = (MW->WBcontrol[0]) ? MW->memData : MW->result;
				if(MW->WBcontrol[1]){
					if (!checkRegister(MW->regDest)){
						handleExit((exit_code) 1, clockCycles);
						return;
					}
					registers[registerMap[MW->regDest]] = writeData;
				}
			}

			halfCycle = true;
			// M stage
			if(clockCycles>=4 && (PCcurr < (commands.size()+3)) && ExM->RegPC != -1){
				if(ExM->Mcontrol[0]){
					if (ExM->result < 0){
						handleExit( (exit_code) abs(ExM->result), clockCycles);
						return;
					}
					data[ExM->result] = ExM->writeData;
					MW->memData = 0;
					memChange = std::to_string(ExM->result) + " " + std::to_string(ExM->writeData);
				}
				else if(ExM->Mcontrol[1]){
					if (ExM->result < 0){
						handleExit( (exit_code) abs(ExM->result), clockCycles);
						return;
					}
					MW->memData = data[ExM->result];
				}
				MW->WBcontrol = ExM->WBcontrol; MW->result = ExM->result; MW->regDest = ExM->regDest;
			}
			MW->RegPC = ExM->RegPC;

            // Ex Stage
            if(clockCycles>=3 && (PCcurr < (commands.size()+2)) && IdEx->RegPC != -1){
                int dataR = (IdEx->EXcontrol[0])? IdEx->dataIm : IdEx->data2;
                ExM->result = ALU(IdEx->data1, dataR, IdEx->ALUOp, clockCycles);
				ExM->WBcontrol = IdEx->WBcontrol; ExM->Mcontrol = IdEx->Mcontrol; ExM->branch = IdEx->branch; 
				ExM->regDest = (IdEx->EXcontrol[1])? IdEx->rt : IdEx->rd; ExM->writeData = IdEx->data2;
				if(ExM->Mcontrol[2]){
					if(ExM->result == 1){
		        		if (address.find(ExM->branch) == address.end() || address[ExM->branch] == -1){
                		    handleExit( (exit_code) 2, clockCycles);
                		    return;
                		}
						PCcurr =  address[ExM->branch];
					}
					else{ PCcurr = IdEx->RegPC + 1;}
					ExM->RegPC = IdEx->RegPC;
					clear(IfId); clear(IdEx); clear(ExM); IfId->RegPC = IdEx->RegPC = -1;
					printRegisters(clockCycles);
					if(memChange != ""){
						std::cout<<"1 "<<memChange<<std::endl;
						memChange = "";
					}
					else{ std::cout<<"0\n"; }
					continue;
				}
			}
			ExM->RegPC = IdEx->RegPC;

            // Id Stage
            if(clockCycles>=2 && (PCcurr < (commands.size()+1)) && IfId->RegPC != -1){
				if(IfId->instr[0] == "j"){
					if (address.find(IfId->instr[1]) == address.end() || address[IfId->instr[1]] == -1){
						handleExit((exit_code) 2, clockCycles);
						return;
					}
					PCcurr = address[IfId->instr[1]]; PCnext = PCcurr  + 1;
					clear(IfId); clear(IdEx); IfId->RegPC = IdEx->RegPC = -1;
					printRegisters(clockCycles);
					if(memChange != ""){
						std::cout<<"1 "<<memChange<<std::endl;
						memChange = "";
					}
					else{ std::cout<<"0\n"; }
					continue;
				}
				
                ControlUnit(IfId->instr, IdEx);
				bool EXhazard1 = (!(ExM->RegPC == -1) && ExM->WBcontrol[1] && (registerMap[ExM->regDest] != 0) && (registerMap[ExM->regDest] == registerMap[IdEx->rs]));
				bool EXhazard2 = (!(ExM->RegPC == -1) && ExM->WBcontrol[1] && (registerMap[ExM->regDest] != 0) && (registerMap[ExM->regDest] == registerMap[IdEx->rt]));
				bool Mhazard1 = (!(MW->RegPC == -1) && MW->WBcontrol[1] && (registerMap[MW->regDest] != 0) && (registerMap[MW->regDest] == registerMap[IdEx->rs]));
				bool Mhazard2 = (!(MW->RegPC == -1) && MW->WBcontrol[1] && (registerMap[MW->regDest] != 0) && (registerMap[MW->regDest] == registerMap[IdEx->rt]));
				bool hazard = EXhazard1 || EXhazard2 || Mhazard1 || Mhazard2;
				if(hazard){
					PCcurr = IfId->RegPC;
					IdEx->RegPC = -1;
					IfId->RegPC = -1;
				}
				if(!hazard){
					if(IdEx->ALUOp < 0){
						handleExit( (exit_code) abs(IdEx->ALUOp), clockCycles);
						return;
					}
				}
            }
			IdEx->RegPC = IfId->RegPC;

			// If stage
			if((PCcurr < commands.size())){
				IfId->instr = commands[PCcurr];
				IfId->RegPC = PCcurr;
			}
			else{ IfId->RegPC = -1; }
			PCcurr++;
			printRegisters(clockCycles);
			if(memChange != ""){
				std::cout<<"1 "<<memChange<<std::endl;
				memChange = "";
			}
			else{ std::cout<<"0\n"; }
	}

	// print the register data
	void printRegisters(int clockCycle)
	{
		for (int i = 0; i < 32; i++)
			std::cout << registers[i] << ' ';
		std::cout << '\n';
	}
};

#endif