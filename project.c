#include "spimcore.h"

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
	ADD   = 0x20, // R
	SUB   = 0x22, // R
	ADDI  = 0x8, // I
	AND   = 0x24, // R
	OR    = 0x25, // R
	LW    = 0x23, // I
	SW    = 0x2B, // I
	LUI   = 0xF, // I
	BEQ   = 0x4, // J
	SLT   = 0x2A, // R
	SLTI  = 0xA, // I
	SLTU  = 0x29, // R
	SLTIU = 0X9,  // I
	J     = 0X2 // J
};

/* ALU */
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero) {
	switch (ALUControl) {
		case 0: *ALUresult = A + B; break;
		case 1: *ALUresult = A - B; break;
		case 2: *ALUresult = ((int) A < (inst) B ? 1 : 0); break; // SIGNED
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
	*r1 = (instruction >> 21) & 0x3F;
	*r2 = (instruction >> 16) & 0x1F;
	*r3 = (instruction >> 11) & 0x1F;
	*funct = instruction & 0x3F;
	*offset = instruction & 0xFFFF;
	*jsec = instruction & 0x3FFFFFF;
}


/* instruction decode */
int instruction_decode(unsigned op,struct_controls *controls){
	/* CONTROL_SIGNALS args
	 * 	RegDst, Jump, Branch, MemRead, MemtoReg, ALUOp, MemWrite, ALUSrc, RegWrite
	 */
	switch (*op) {
		case ADD:
		case SUB:
		case AND:
		case OR:
		case SLT:
		case SLTU:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 7, 0, 0, 1);
			break;
		case ADDI:
			CONTROL_SIGNALS(0, 0, 0, 0, 0, 0, 0, 1, 1);
			break;
		case LW:
			CONTROL_SIGNALS(0, 0, 0, 1, 1, 0, 0, 1, 1);
			break;
		case SW:
			CONTROL_SIGNALS(2, 0, 0, 0, 0, 0, 1, 1, 0);
			break;
		case LUI:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 6, 0, 1, 1);
			break;
		case SLTI:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 2, 0, 1, 1);
			break;
		case SLTIU:
			CONTROL_SIGNALS(1, 0, 0, 0, 0, 3, 0, 1, 1); 
			break;
		case BEQ:
			CONTROL_SIGNALS(2, 0, 1, 0, 2, 1, 0, 2, 0);
			break;
		case J:
			CONTROL_SIGNALS(2, 1, 2, 0, 2, 0, 0, 2, 0);
			break;
		default:
			return 1;
	}
	
	return 0;
}

/* Read Register */
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2) {
	*data1 = Reg[r1];
	*data2 = Reg[r2];
}


/* Sign Extend */
void sign_extend(unsigned offset,unsigned *extended_value) {
	*extended_value = offset >> 15 ? 0xffff0000 : 0x0000ffff;
}

/* ALU operations */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero) {
	// Immediate Source
	if (ALUSrc) {
		ALU(data1, extended_value, ALUOp, ALUresult, Zero);
	}
	// R-type
	else {
		switch (funct) {
			case  ADD: ALUOp = 0; break;
			case  SUB: ALUOp = 1; break;
			case  SLT: ALUOp = 2; break;
			case  AND: ALUOp = 3; break;
			case SLTU: ALUOp = 4; break;
			case   OR: ALUOp = 5; break;

			default: return 1;
		}
		ALU(data1, data2, ALUOp, ALUresult, Zero);
	}
}

/* Read / Write Memory */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem) {
	if (MemWrite) {
		if (ALUresult % 4 || ALUresult < 0 || ALUresult > 0xFFFF) return 1;
		Mem[ALUresult / 4] = data2;
	}
	if (MemRead) {
		if (ALUresult % 4 || ALUresult < 0 || ALUresult > 0xFFFF) return 1;
		*memdata = Mem[ALUresult / 4];
	}

	return 0;
}

/* Write Register */
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg) {
	unsigned data;

	if (MemtoReg == 1) {
		data = memdata;
	} else {
		data = ALUresult;
	}

	if (RegWrite) {
		if (RegDst) {
			Reg[r3] = r3 ? data : 0;
		} else {
			Reg[r2] = r2 ? data : 0;
		}
	}
}

/* PC update */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC) {
	*PC += 4;
	
	if (Branch && Zero) {
		*PC += extended_value * 4;
	}

	if (Jump) {
		*PC &= 0xF0000000;
		*PC |= jsec * 4;
	}
}

