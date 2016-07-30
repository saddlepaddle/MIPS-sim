/* 
 * Satya Patel, Sean Rapp, and Brody Nissen
 * CDA 3103
 * MYSPIM MIPS Simulator
 * 7/28/2016
 */

#include "spimcore.h"


/* 
 * Macro used for instruction decode
 * Fills in the control struct with the required 
 * values based on which opcode is given
 */
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


/* Does the required ALU function based on what signal is given to the ALU */
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero) {
	
	switch (ALUControl) {
		case 0: *ALUresult = A + B; break; //Addition
		case 1: *ALUresult = A - B; break; //Subtraction
		case 2: *ALUresult = ((int) A < (int) B ? 1 : 0); break; //Signed
		case 3: *ALUresult = (A < B ? 1 : 0); break; //Unsigned
		case 4: *ALUresult = A & B; break; //And
		case 5: *ALUresult = A | B; break; //Or
		case 6: *ALUresult = B << 16; break; //Shift Left
		case 7: *ALUresult = ~A; break; //Negate
	}

	//If the result of the ALU function is 0, sets the zero flag to true
	*Zero = *ALUresult ? 0 : 1;
}

/* Gets the instruction from the PC only if the instruction is grabbed from a valid location in MEM */
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction) {
	//If the PC counter is not word aligned or out of MEM signal exit
	if ((PC % 4) || (PC < 0 || PC > 0xFFFF))
		   return 1;

	//Otherwise take the instruction
	*instruction = Mem[PC / 4];
	return 0;
}


/* 
 * Takes the instruction given and separates the bits into all 
 * of the different values needed
 */
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec) {
	*op = instruction >> 26; //Opcode
	*r1 = (instruction >> 21) & 0x1F; //Register 1 value
	*r2 = (instruction >> 16) & 0x1F; //Register 2 value
	*r3 = (instruction >> 11) & 0x1F; //Register 3 value
	*funct = instruction & 0x3F; 
	*offset = instruction & 0xFFFF;
	*jsec = instruction & 0x3FFFFFF;
}


/* Based on the opcode given, sends the signals needed to the various other sections of the processor */
int instruction_decode(unsigned op,struct_controls *controls) {
	/* 
	 * Uses the Macro from before, with the Arguments:
	 * RegDst, Jump, Branch, MemRead, MemtoReg, ALUOp, MemWrite, ALUSrc, RegWrite
	 */
	
	switch(op){ //Finds which opcode is given
		case 0: //If it is R-type, then the following signal is given
			CONTROL_SIGNALS(1,0,0,0,0,7,0,0,1);
			break;
		case 8: //The opcode for addi
			CONTROL_SIGNALS(0,0,0,0,0,0,0,1,1);
			break;
		case 35: //lw
			CONTROL_SIGNALS(0,0,0,1,1,0,0,1,1);
			break;
		case 43: //sw
			CONTROL_SIGNALS(0,0,0,0,0,0,1,1,0);
			break;
		case 15: //sw
			CONTROL_SIGNALS(0,0,0,0,0,6,0,1,1);
			break;
		case 4: //beq
			CONTROL_SIGNALS(2,0,1,0,2,1,0,2,0);
			break;
		case 10: //slti
			CONTROL_SIGNALS(0,0,0,0,0,2,0,1,1);
			break;
		case 11: //sltui
			CONTROL_SIGNALS(0,0,0,0,0,3,0,1,1);
			break;
		case 2: //j
			CONTROL_SIGNALS(2,1,2,0,2,0,0,2,0);
			break;
		default: //if the opcode given is not valid, then the exit signal is given
			return 1;
        }
		
        return 0;
}


/* Reads the information held in data into the two registers that are defined */
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2) {
	*data1 = Reg[r1];
	*data2 = Reg[r2];
}


/* Sign Extends the value given */
void sign_extend(unsigned offset,unsigned *extended_value) {
	*extended_value = (offset >> 15 ? offset | 0xffff0000 : offset & 0x0000ffff);
}
/* Based on the signal given, determines what to tell the ALU and calls the ALU */
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero) {

	//temp variable used so ALUOp is unchanged
    unsigned char whichOP = ALUOp;

	//if ALUOp is an invalid number, the exit signal is given
	if (ALUOp < 0 || ALUOp > 7) return 1;
	
	//If the ALUOP signals R-type instructions, determine which r-type it is and assign whichOP
	if (ALUOp == 7) {
		switch(funct){
			case 32: //add
				whichOP = 0;
				break;
			case 36: //and
				whichOP = 4;
				break;
			case 37: // or
				whichOP = 5;
				break;
			case 42: //slt
				whichOP = 2;
				break;
			case 43: //sltu
				whichOP = 3;
				break;
			default: //if an invalid funct is given, exit
				return 1;
		}
	}

		/* Calls the ALU */
	//If ALUSrc is active, use the extended value instead of data2
	if (ALUSrc) ALU(data1, extended_value, whichOP, ALUresult, Zero);
	//Otherwise business as usual
	else ALU(data1, data2, whichOP, ALUresult, Zero);

	return 0;
}


/* If the signal is given to read or write memory, the memory is fetched or written to as told after checking if the location is valid */
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem) {
	// Checks if the location given is word aligned and in bounds, and then writes to the location
	if (MemWrite) {
		if (ALUresult % 4 || ALUresult < 0 || ALUresult > 0xFFFF) return 1; //Exit signal
		Mem[ALUresult / 4] = data2;
	}
	//Checks again, and then reads the memory
	if (MemRead) {
		if (ALUresult % 4 || ALUresult < 0 || ALUresult > 0xFFFF) return 1;
		*memdata = Mem[ALUresult / 4];
	}

	return 0;
}

/* Writes to the register if the register address is not 0 (i.e. given) and if the signal to write is given */
void write_register(unsigned r2,unsigned r3,unsigned memdata,unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg) {
	//Based on memtoreg, decides what data is going to be passed on
	unsigned data = MemtoReg ? memdata : ALUresult;
	//If you are supposed to write to a register, it does so
	if (RegWrite) {
		if (RegDst) {
			//if r3 is valid
			Reg[r3] = r3 ? data : 0;
		} else {
			//if r2 is valid
			Reg[r2] = r2 ? data : 0;
		}
	}
}

/* Updates the PC counter, based on what signals are given */
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC) {
	//Default
	//Jumps to next word in mem
	*PC += 4;
	
	//If you have used beq, then this is triggered
	if (Branch && Zero) *PC += extended_value * 4;
	
	//If you have used jump, this is triggered
	if (Jump) {
		//Jumps to location given 
		*PC &= 0xF0000000;
		*PC |= jsec * 4;
	}
}

