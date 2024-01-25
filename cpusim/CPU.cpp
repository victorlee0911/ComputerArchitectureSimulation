#include "CPU.h"

instruction::instruction(bitset<32> fetch)
{
	//cout << fetch << endl;
	instr = fetch;
	//cout << instr << endl;
}

CPU::CPU()
{
	PC = 0; //set PC to 0
	for (int i = 0; i < 4096; i++) //copy instrMEM
	{
		dmemory[i] = (0);
	}
}

bitset<32> CPU::Fetch(bitset<8> *instmem) {
	bitset<32> instr = ((((instmem[PC + 3].to_ulong()) << 24)) + ((instmem[PC + 2].to_ulong()) << 16) + ((instmem[PC + 1].to_ulong()) << 8) + (instmem[PC + 0].to_ulong()));  //get 32 bit instruction
	PC += 4;//increment PC
	return instr;
}


bool CPU::Decode(instruction* curr)
{
//cout<<curr->instr<<endl;
	//cout << PC << " " << curr->instr << endl;

	string opcode = controller(curr->instr);

	// terminate program
	if (opcode == "0000000")
	{
		return false;
	}

	// get 2 ops for alu
	int32_t op1 = get_op1(curr->instr);
	int32_t op2 = get_op2(curr->instr, signals["AluSrc"], opcode);
	//cout << "op1: " << op1 << endl;
	//cout << "op2: " << op2 << endl;

	//compute alu 
	int32_t alu_product = alu(aluOp, op1, op2);
	//cout << "alu product: " << alu_product << endl;

	// compute immediate for branch 
	if(signals["Branch"])
	{
		if(alu_product < 0)
		{
			bitset<32> bmask1 = (1 << 31);
			bitset<32> bmask3 = (((1 << 6) - 1) << 25);
			bitset<32> bmask2 = (1 << 7);
			bitset<32> bmask4 = (((1 << 4) - 1) << 8);
			bitset<32> tempbit = (((bmask1 & curr->instr) >> 20) | ((bmask2 & curr->instr) << 3) | ((bmask3 & curr->instr) >> 21) | ((bmask4 & curr->instr) >> 7));
			PC += sign_extend(13, tempbit) - 4;
		}
	}

	//mem prep
	bitset<32> r2_mask = ((1 << 5) - 1) << 20;
	bitset<32> reg2 = (curr->instr & r2_mask) >> 20;
	//cout << reg2.to_ulong() << endl;
	//memory
	int32_t mem_product = mem(signals["MemWr"], signals["MemRe"], alu_product, reg[reg2.to_ulong()]);

	// emulating mux
	int32_t final_product;
	if(signals["LinkR"])
	{
		//cout << "PC: " <<  PC << endl;
		final_product = PC;
		PC = (alu_product >> 1) << 1;
		//cout << "final: " << final_product << endl;
	}
	else if (signals["MemtoReg"])
	{
		final_product = mem_product;
	}
	else
	{
		final_product = alu_product;
	}

	if(signals["RegWrite"])
	{
		//if sw, then rd = r2
		bitset<32> rd_mask;
		int32_t rd;
		rd_mask = (((1 << 5) - 1) << 7);
		rd = static_cast<int32_t>(((curr->instr & rd_mask) >> 7).to_ulong());

		//cout << "RD: " << rd << endl;
		reg[rd] = final_product;
		//cout << "reg " << rd << " changed to " << reg[rd] << endl;
	}
	
	reg[0] = 0;

	//cout << endl;

	return true;
}

// outputs control signals given opcode and func3
string CPU::controller(bitset<32> instr)
{
	bitset<32> opcode_mask = (1 << 7) - 1; //captures last 7 bits
	string opcode = (opcode_mask & instr).to_string().substr(25);

	bitset<32> tricode_mask = ((1 << 3) - 1) << 12;
	string tricode = ((tricode_mask & instr) >> 12).to_string().substr(29);
	// 000 = add/sub, 010 = add, 100 = xor, 101 = sra, 110 = or, 111 = and, 

	if (opcode == "0110011")
	{
		//cout << "R-type" << endl;
		signals["RegWrite"] = true;
		signals["AluSrc"] = false;
		signals["Branch"] = false;
		signals["MemRe"] = false;
		signals["MemWr"] = false;
		signals["MemtoReg"] = false;
		signals["LinkR"] = false;

		//figure out aluOp
		if (tricode == "000") // add, sub
		{
			if (instr[30] == 0)
			{
				aluOp = "add";
			} 
			else
			{
				aluOp = "sub";
			}
		}
		else if (tricode == "100")
		{
			aluOp = "xor";
		}
		else if (tricode == "101")
		{
			aluOp = "sra";
		}

		//cout << aluOp << endl;

		return opcode;
	} 
	else if (opcode == "0010011")
	{
		//cout << "I-type" << endl;
		signals["RegWrite"] = true;
		signals["AluSrc"] = true;
		signals["Branch"] = false;
		signals["MemRe"] = false;
		signals["MemWr"] = false;
		signals["MemtoReg"] = false;
		signals["LinkR"] = false;

		if (tricode == "000") // addi
		{
			aluOp = "add";
		}
		else if (tricode == "111")
		{
			aluOp = "and";
		}

		//cout << aluOp << endl;

		return opcode;
	}
	else if (opcode == "0000011")
	{
		//cout << "LW" << endl;
		signals["RegWrite"] = true;
		signals["AluSrc"] = true;
		signals["Branch"] = false;
		signals["MemRe"] = true;
		signals["MemWr"] = false;
		signals["MemtoReg"] = true;
		signals["LinkR"] = false;

		aluOp = "add";
	}
	else if (opcode == "0100011")
	{
		//cout << "SW" << endl;
		signals["RegWrite"] = false;
		signals["AluSrc"] = true;
		signals["Branch"] = false;
		signals["MemRe"] = false;
		signals["MemWr"] = true;
		signals["MemtoReg"] = false;
		signals["LinkR"] = false;

		aluOp = "add";
	}
	else if (opcode == "1100011")
	{
		//cout << "BLT" << endl;
		signals["RegWrite"] = false;
		signals["AluSrc"] = false;
		signals["Branch"] = true;
		signals["MemRe"] = false;
		signals["MemWr"] = false;
		signals["MemtoReg"] = false;
		signals["LinkR"] = false;

		aluOp = "sub";
	}
	else if (opcode == "1100111")
	{
		//cout << "JALR" << endl;
		signals["RegWrite"] = true;
		signals["AluSrc"] = true;
		signals["Branch"] = false;
		signals["MemRe"] = false;
		signals["MemWr"] = false;
		signals["MemtoReg"] = false;
		signals["LinkR"] = true;

		aluOp = "add";
	}
	else if (opcode == "0000000")
	{
		//cout << "Program End" << endl;
	}
	else
	{
		//cout << "unknown opcode" << endl;
	}
	return opcode;
}

//ALU
int32_t CPU::alu(string aluOp, int32_t op1, int32_t op2)
{
	if (aluOp == "add")
	{
		//cout << "add" << op1 << " " << op2<< endl;
		return op1 + op2;
	}
	else if (aluOp == "sub")
	{
		return op1 - op2;
	}
	else if (aluOp == "xor")
	{
		return op1 ^ op2;
	}
	else if (aluOp == "sra")
	{
		return op1 >> op2;
	}
	else if (aluOp == "and")
	{
		return op1 & op2;
	}
	return 0;
}

//always register 1
int32_t CPU::get_op1(bitset<32> instr)
{
	bitset<32> r1_mask = ((1 << 5) - 1) << 15;
	bitset<32> reg1 = (instr & r1_mask) >> 15;
	//cout << "op1: reg " << reg1.to_ulong() << endl;
	return reg[reg1.to_ulong()];
}

// either immediate or reg2
int32_t CPU::get_op2(bitset<32> instr, bool aluSrc, string opcode)
{
	if(aluSrc) 		// use immediate
	{
		if(opcode == "0010011" || opcode == "0000011" || opcode == "1100111") // i type or lw or jalr
		{
			bitset<32> imm_mask = ((1 << 12) - 1) << 20;
			bitset<32> imm = (instr & imm_mask) >> 20;
			//sign extend immediate for negatives
			//cout << "op2: " << static_cast<int32_t>(imm.to_ulong()) << endl;
			return sign_extend(12, imm);
		}
		else if(opcode == "0100011") // sw
		{
			bitset<32> imm_mask1 = ((1 << 7) - 1) << 25;
			bitset<32> imm_mask2 = ((1 << 4) - 1) << 7;
			bitset<32> imm = ((imm_mask1 & instr) >> 21) | ((imm_mask2 & instr) >> 7);
			//cout << "op2: " << static_cast<int32_t>(imm.to_ulong()) << endl; 
			return sign_extend(12, imm);

		}
	}
	else			// use reg2
	{
		bitset<32> r2_mask = ((1 << 5) - 1) << 20;
		bitset<32> reg2 = (instr & r2_mask) >> 20;
		//cout << "op2: reg " << reg2.to_ulong() << endl;
		return reg[reg2.to_ulong()];
	}
	return 0;
}

//read/write for memory
int32_t CPU::mem(bool memWr, bool memRe, int32_t addr, int32_t write_data)
{
	if(memWr)
	{
		dmemory[addr] = write_data;
		//cout << "changed mem at " << addr << " to " << write_data << endl;
	}
	else if(memRe)
	{
		return dmemory[addr];
	}
	return 0;
}

int32_t CPU::get_reg(int32_t reg_id)
{
	return reg[reg_id];
}

//to del with negative immediates
int32_t CPU::sign_extend(int32_t msb, bitset<32> imm)
{
	bitset<32> mask;
	bitset<32> ext_imm = imm;
	if(msb == 13)
	{
		mask = ((1 << 19) - 1) << 13;
		if(imm[12] == 1)
		{
			ext_imm = mask | ext_imm;
		}
	}
	else if(msb == 12)
	{
		mask = ((1 << 20) - 1) << 12;
		if(imm[11] == 1)
		{
			ext_imm = mask | ext_imm;
		}
	}
	return static_cast<int32_t>(ext_imm.to_ulong()); 
}

unsigned long CPU::readPC()
{
	return PC;
}



// Add other functions here ... 