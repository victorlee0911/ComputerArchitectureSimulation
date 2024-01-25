#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include <unordered_map>
using namespace std;


class instruction {
public:
	bitset<32> instr;//instruction
	instruction(bitset<32> fetch); // constructor
};

class CPU {
private:
	int dmemory[4096]; //data memory byte addressable in little endian fashion;
	int32_t reg[32];
	unsigned long PC; //pc 
	unordered_map<string, bool> signals;
	string aluOp;

public:
	CPU();
	unsigned long readPC();
	bitset<32> Fetch(bitset<8> *instmem);
	bool Decode(instruction* instr);		
	string controller(bitset<32> instr);	
	int32_t alu(string aluOp, int32_t op1, int32_t op2);
	int32_t get_op1(bitset<32> instr);
	int32_t get_op2(bitset<32> instr, bool aluSrc, string opcode);
	int32_t mem(bool memWr, bool memRe, int32_t addr, int32_t write_data);
	int32_t get_reg(int32_t reg_id);
	int32_t sign_extend(int32_t msb, bitset<32> imm);
	
};

// add other functions and objects here
