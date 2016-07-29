#include "spimcore.h"

#define CONTROL_SIGNALS(REGDST, JUMP, BRANCH, MEMREAD, MEMTOREG, ALUOP, MEMWRITE, ALUSRC, REGWRITE) \
			controls->RegDst = REGDST;\
			controls->RegWrite = REGWRITE;\
			controls-> ALUSrc = ALUSRC;\
			controls->ALUOp = ALUOP;\
			controls->MemWrite = MEMWRITE;\
			controls->MemRead = MEMWRITE;\
			controls->MemtoReg = MEMTOREG;\
			controls->Jump = JUMP; \
			controls->Branch = BRANCH;


/* ALU */
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero) {
	switch (ALUControl) {
		case 0: *ALUresult = A + B; break;
		case 1: *ALUresult = A - B; break;
		case 2: *ALUresult = ((int) A < (int) B ? 1 : 0); break; // SIGNED
		case 3: *ALUresult = (A < B ? 1 : 0); break;
		case 4: *ALUresult = A & B; break;
		case 5: *ALUresult = A | B; break;
		case 6: *ALUresult = B << 16; break;
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
	*r1 = (instruction >> 21) & 0x1F;
	*r2 = (instruction >> 16) & 0x1F;
	*r3 = (instruction >> 11) & 0x1F;
	*funct = instruction & 0x3F;
	*offset = instruction & 0xFFFF;
	*jsec = instruction & 0x3FFFFFF;
}

/* instruction decode */
/* 15 Points */
int instruction_decode(unsigned op,struct_controls *controls) {
        switch(op){

		/* Macro Arguments:
		 * RegDst, Jump, Branch, MemRead, MemtoReg, ALUOp, MemWrite, ALUSrc, RegWrite
		 */

                case 0:
                        CONTROL_SIGNALS(1,0,0,0,0,7,0,0,1);
                        break;
                case 8:
                        CONTROL_SIGNALS(0,0,0,0,0,0,0,1,1);
                        break;
                case 35:
                        CONTROL_SIGNALS(0,0,0,1,1,0,0,1,1);
                        break;
                case 43:
                        CONTROL_SIGNALS(0,0,0,0,0,0,1,1,0);
                        break;
                case 15:
                        CONTROL_SIGNALS(0,0,0,0,0,6,0,1,1);
                        break;
                case 4:
                        CONTROL_SIGNALS(2,0,1,0,2,1,0,2,0);
                        break;
                case 10:
                        CONTROL_SIGNALS(0,0,0,0,0,2,0,1,1);
                        break;
                case 11:
                        CONTROL_SIGNALS(0,0,0,0,0,3,0,1,1);
                        break;
                case 2:
                        CONTROL_SIGNALS(2,1,2,0,2,0,0,2,0);
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
	*extended_value = (offset >> 15 ? offset | 0xffff0000 : offset & 0x0000ffff);
}
/* ALU operations */
/* 10 Points */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero) {

        unsigned char ALUControl = ALUOp;

        switch(ALUOp){
                case 0x0:
                case 0x1:
                case 0x2:
                case 0x3:
                case 0x4:
                case 0x5:
                case 0x6:
                        break;
                case 0x7:
                        switch(funct){
                                case 0x20:
                                        ALUControl = 0x0;
		                        break;
                                case 0x24:
                                        ALUControl = 0x4;
		                        break;
                                case 0x25:      // or
                                        ALUControl = 0x5;
                    			break;
                                case 0x2a:
                                        ALUControl = 0x2;
                    			break;
                                case 0x2b:
                                        ALUControl = 0x3;
                    			break;
                                default:
                                        return 1;
                        }
            		break;
                default:
                	 return 1;

        }

        ALU(data1, (ALUSrc == 1)? extended_value : data2, ALUControl, ALUresult, Zero);

        return 0;

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
	/////////////////////////////////////////////////////////////////////
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

