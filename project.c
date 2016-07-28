#include "spimcore.h"

/*
 * add 100000
 * sub 100010
 * addi 001000
 * and 100100
 * or 100101
 * lw 100011
 * sw 101011
 * lui 001111
 * beq 000100
 * slt 101010
 * slti 001010
 * sltu 101001
 * sltiu 001001
 * j 000010
*/

#define CONTROL_SIGNALS(REGDST, JUMP, BRANCH, MEMREAD, MEMTOREG, ALUOP, MEMWRITE, ALUSRC, REGWRITE) \
			controls->RegDst = REGDST;\
			controls->RegWrite = REGWRITE;\
			controls-> ALUSrc = ALUSRC;\
			controls->ALUop = ALUOP;\
			controls->MemWrite = MEMWRITE;\
			controls->MemRead = MEMWRITE;\
			controls->MemtoReg = MEMTOREG;\
			controls->Jump = JUMP; \
			controls->Branch = BRANCH;\ 



enum {
	ADD = 0x20, // R
	SUB = 0x22, // R
	ADDI = 0x8, // I
	AND = 0x24, // R
	OR = 0x25, // R
	LW = 0x23, // I
	SW = 0x2B, // I
	LUI = 0xF, // I
	BEQ = 0x4, // J
	SLT = 0x2A, // R
	SLTI = 0xA, // I
	SLTU = 0x29, // R
	SLTIU = 0X9,  // I
	J = 0X2 // J
};

/* ALU */
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero) {
	switch (ALUControl) {
		case 0: *ALUresult = A + B; break;
		case 1: *ALUresult = A - B; break;
		case 2: *ALUresult = (A < B ? 1 : 0); break; // SIGNED
		case 3: *ALUresult = (A < B ? 1 : 0); break;
		case 4: *ALUresult = A & B; break;
		case 5: *ALUresult = A | B; break;
		case 6: *ALUresut = B << 16; break;
		case 7: *ALUresult = ~A; break;
	}

	*Zero = *ALUresult ? 0 : 1;
}

/* instruction fetch */
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction) {
	if ((PC % 4) || (PC < 0 || PC > 0xFFFF))
		   return 1;

	*instruction = Mem[PC / 4];
	return 0;
}


/* instruction partition */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec) {
	*op = instruction >> 26;
	char type;

	switch (*op) {
		case ADD:
		case SUB:
		case AND:
		case OR:
		case SLT:
		case SLTU:
			type = 'R';
			break;
		case ADDI:
		case LW:
		case SW:
		case LUI:
		case SLTI:
		case SLTIU:
		case BEQ:	
			type = 'I';
			break;
		case J:
			type = 'J';
		default:
			type = 'X';
	}

	switch (type) {
		case 'R':
			*r1 = (instruction >> 21) & 0x3F;
			*r2 = (instruction >> 16) & 0x1F;
			*r3 = (instruction >> 11) & 0x1F;
			*offset = (instruction >> 6) & 0x1F;
			function = instruction & 0x3F;
			break;
		case 'I':
			*r1 = (instruction >> 21) & 0x1F;
			*r2 = (instruction >> 16) & 0x1F;
			*offset = instruction & 0xFFFF;
			break;
		case 'J':
			*jsec = instruction & 0x3FFFFFF;
			break;
		default:
	}
}



/* instruction decode */
int instruction_decode(unsigned op,struct_controls *controls){
	/* CONTROL_SIGNALS args
	 * 	RegDst, Jump, Branch, MemRead, MemtoReg, ALUOp, MemWrite, ALUSrc, RegWrite
	 */
	switch (*op) {
		case ADD:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 2, 0, 0, 1);
			break;
		case SUB:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 6, 0, 0, 1);
			break;
		case AND:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 0, 0, 0, 1);
			break;
		case OR:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 1, 0, 0, 1);
			break;
		case SLT:
			
			break;
		case SLTU:

			break;
		case ADDI:

			break;
		case LW:
			CONTROL_SIGNALS(0, 0, 0, 1, 1, 0, 0, 1, 1);
			break;
		case SW:

			break;
		case LUI:

			break;
		case SLTI:

			break;
		case SLTIU:

			break;
		case BEQ:
			CONTROL_SIGNALS(2, 0, 1, 0, 2, 1, 0, 0, 0);
			break;
		case J:

			break;
		default:

	}


	
}

/* Read Register */
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{

}


/* Sign Extend */
void sign_extend(unsigned offset,unsigned *extended_value)
{

}

/* ALU operations */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{

}

/* Read / Write Memory */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{

}


/* Write Register */
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{

}

/* PC update */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{

}

