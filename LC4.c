/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>
#define INSN_OP(I) ((I) >> 12)
#define INSN_11_9(I) (((I) >> 9) & 0x7)
#define INSN_5_3(I) (((I)>>3) & 0x7)
#define INSN_5(I) (((I)>>5) & 0x1)
#define INSN_8_6(I) (((I)>>6) & 0x7)
#define INSN_2_0(I) ((I) & 0x7)
#define INSN_3_0(I) ((I) & 0xF)
#define INSN_4_0(I) ((I) & 0x1F)
#define INSN_5_0(I) ((I) & 0x3F)
#define INSN_6_0(I) ((I) & 0x7F)
#define INSN_7_0(I) ((I) & 0xFF)
#define INSN_8_0(I) ((I) & 0x1FF)
#define INSN_10_0(I) ((I) & 0x7FF)
#define INSN_8_7(I) (((I)>>7) & 0x3)
#define INSN_4_3(I) (((I)>>3) & 0x3)
#define INSN_11(I) (((I)>>11) & 0x1)
#define INSN_5_4(I) (((I)>>4) & 0x3)

/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState* CPU)
{
    ClearSignals(CPU);
    CPU->PC = 0x8200; //default starting PC
    CPU->PSR = 0x8002;
    for(int i = 0; i < 8; i++){
        CPU->R[i] = 0;
    }
    CPU->regInputVal = 0;
    CPU->NZPVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
}


/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState* CPU)
{
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
}


/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    char instruct_binary[17]; // Allocate memory for 16 bits (+1 for null terminator)
    instruct_binary[16] = '\0'; // Null terminator at the end of the string
    unsigned short int dest_reg = 0;
    for (int i = 0; i < 16; i++){
        instruct_binary[15 - i] = (instruction >> i) & 1 ? '1' : '0'; // Extract bits from the instruction
    }
    if (CPU->regFile_WE == 1){
      if (CPU->rdMux_CTL == 0){
        dest_reg = INSN_11_9(instruction);
      }
      else{
        dest_reg = 7;
      }
    }
    fprintf(output, "%04X %s %01X %01X %04X %01X %01X %01X %04X %04X\n", 
        CPU->PC, instruct_binary, CPU->regFile_WE, dest_reg, CPU->regInputVal,
        CPU->NZP_WE, CPU->NZPVal, CPU->DATA_WE, CPU->dmemAddr, CPU->dmemValue);
}


void ConstOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int Rd = INSN_11_9(instruction);
    unsigned short int immediate = INSN_8_0(instruction);
    signed short int sign_bit = immediate >> 8; // Shift sign bit to the least significant bit
    signed short int IMM9 = (sign_bit ? 0xFE00 : 0x0000) | immediate;

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = IMM9;
    SetNZP(CPU, IMM9);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->R[Rd] = IMM9;
    CPU->PC = CPU->PC + 1;
    return;
}

void HiconstOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int Rd = INSN_11_9(instruction);

    unsigned short int UIMM8 = INSN_7_0(instruction);

    unsigned short int dest_val = (CPU->R[Rd] & 0xFF) | (UIMM8 << 8);

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = dest_val;
    SetNZP(CPU, dest_val);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->R[Rd] = dest_val;
    CPU->PC = CPU->PC + 1;
    return;
}

void LdrOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int Rd = INSN_11_9(instruction);
    unsigned short int Rs = INSN_8_6(instruction);

    unsigned short int immediate = INSN_5_0(instruction);
    signed short int sign_bit = immediate >> 5; // Shift sign bit to the least significant bit
    signed short int IMM6 = (sign_bit ? 0xFFC0 : 0x0000) | immediate;

    //check if privilege bit is 1 if CPU->R[Rs] is in OS code

    if (CPU->R[Rs] + IMM6 >= 0x8000 && (CPU->PSR & 0x8000) == 0){
        fprintf(stderr, "Access Denied: Privilege = 0\n");
        CPU->PC = CPU->PC + 1;
        return;
    }
    
    unsigned short int dest_val = CPU->memory[CPU->R[Rs] + IMM6];

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = dest_val;
    SetNZP(CPU, dest_val);
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->R[Rd] = dest_val;
    CPU->PC = CPU->PC + 1;
    return;

}

void StrOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int Rt = INSN_11_9(instruction);
    unsigned short int Rs = INSN_8_6(instruction);

    unsigned short int immediate = INSN_5_0(instruction);
    signed short int sign_bit = immediate >> 5; // Shift sign bit to the least significant bit
    signed short int IMM6 = (sign_bit ? 0xFFC0 : 0x0000) | immediate;
    
    unsigned short int d_address = CPU->R[Rs] + IMM6;

    //check if privilege bit is 1 if d_address is in OS code
    if (d_address >= 0x8000 && (CPU->PSR & 0x8000) == 0){
        fprintf(stderr, "Access Denied: Privilege = 0\n");
        CPU->PC = CPU->PC + 1;
        return;
    }

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 1;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->NZPVal = 0;
    CPU->DATA_WE = 1;
    CPU->regInputVal = 0;
    CPU->dmemAddr = d_address;
    CPU->dmemValue = CPU->R[Rt];
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->memory[d_address] = CPU->R[Rt];
    CPU->PC = CPU->PC + 1;
    return;
}

void TrapOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];

    unsigned short int UIMM8 = INSN_7_0(instruction);

    unsigned short int dest_val =  CPU->PC + 1;
    unsigned short int newPC =   (0x8000 | UIMM8);

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 1;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = dest_val;
    SetNZP(CPU, dest_val);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & (0x0000); //clearing NZP bits in PSR and status bit
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->PSR = CPU->PSR | (0x8000); //setting status bit to 1
    CPU->R[7] = dest_val;
    CPU->PC = newPC;
    return;
}

void RtiOp(MachineState* CPU, FILE* output)
{
    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 1;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
    CPU->NZPVal = 0;
    CPU->regInputVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x8000); //clearing status bit to 0
    CPU->PC = CPU->R[7];
    return;
}


/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState* CPU, FILE* output)
{
    //if PC for instruction is in data section, then return error. 
    if ((CPU->PC >= 0x2000 && CPU->PC <=0x7FFF) || (CPU->PC >= 0xA000 && CPU->PC <=0xFFFF)){
        fprintf(stderr, "Cannot execute from data section\n");
        CPU->PC = CPU->PC + 1;
        return 1;
    }
    
    unsigned short int instruction = CPU->memory[CPU->PC];

    unsigned short int opcode = INSN_OP(instruction);

    unsigned short int Rs = INSN_8_6(instruction);
    unsigned short int immediate = INSN_5_0(instruction);
    signed short int sign_bit = immediate >> 5; // Shift sign bit to the least significant bit
    signed short int IMM6 = (sign_bit ? 0xFFC0 : 0x0000) | immediate;
    unsigned short int d_address = CPU->R[Rs] + IMM6;

    //before executing, check if you align with the privilege bit
    switch (opcode){
        case 0x0: //0000
            //handle branching
            BranchOp(CPU, output);
            break;
        case 0x1: //0001
            //handle arithmetic
            ArithmeticOp(CPU, output);
            break;
        case 0x2: //0010
            //handle COMPARE
            ComparativeOp(CPU, output);
            break;
        case 0x4:  //0100
            //handle JSR
            JSROp(CPU, output);
            break;
        case 0x5: //0101
            //handle logic
            LogicalOp(CPU, output);
            break;
        case 0xA: //1010
            //handle shifting
            ShiftModOp(CPU, output);
            break;
        case 0xC: //1100
            //handle JUMP
            JumpOp(CPU, output);
            break;
        case 0x9:
            ConstOp(CPU, output);
            break;
        case 0xD:
            HiconstOp(CPU, output);
            break;
        case 0x6:
            //LDR
            //check if privilege bit is 1 if d_address is in OS code
            if (CPU->R[Rs] + IMM6 >= 0x8000 && (CPU->PSR & 0x8000) == 0){
                fprintf(stderr, "Access Denied: Privilege = 0\n");
                CPU->PC = CPU->PC + 1;
                return 1;
            }
            //check that you are not loading from the code section
            if ((CPU->R[Rs] + IMM6 >= 0x0 && CPU->R[Rs] + IMM6 <= 0x1FFF) 
                    || (CPU->R[Rs] + IMM6 >= 0x8000 && CPU->R[Rs] + IMM6 <= 0x9FFF)){
                fprintf(stderr, "Cannot load data from code section\n");
                CPU->PC = CPU->PC + 1;
                return 1;
            }
            LdrOp(CPU, output);
            break;
        case 0x7:
            //STR
            if (d_address >= 0x8000 && (CPU->PSR & 0x8000) == 0){
                fprintf(stderr, "Access Denied: Privilege = 0\n");
                CPU->PC = CPU->PC + 1;
                return 1;
            }
            //check that you are not storing into the code section
            if ((d_address >= 0x0 && d_address <= 0x1FFF) 
                    || (d_address >= 0x8000 && d_address <= 0x9FFF)){
                fprintf(stderr, "Cannot write data to code section\n");
                CPU->PC = CPU->PC + 1;
                return 1;
            }
            StrOp(CPU, output);
            break;
        case 0xF:
            //TRAP
            TrapOp(CPU, output);
            break;
        case 0x8:
            //RTI
            RtiOp(CPU, output);
            break;
        //LDR, STR, TRAP, RTI

    }
    return 0;
}



//////////////// PARSING HELPER FUNCTIONS ///////////////////////////



/*
 * Parses rest of branch operation and updates state of machine.
 */
 // updates PC, regFile_WE
void BranchOp(MachineState* CPU, FILE* output) 
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int sub_opcode = INSN_11_9(instruction);
    unsigned short int immediate = INSN_8_0(instruction);
    signed short int sign_bit = immediate >> 8; // Shift sign bit to the least significant bit
    signed short int IMM9 = (sign_bit ? 0xFE00 : 0x0000) | immediate;
    unsigned short int PSR = CPU->PSR;

    unsigned short int newPC = 0;

    switch (sub_opcode){
        case 0x4:
            if ((PSR & 0x4) == 0x4){ //negative
                newPC = CPU->PC + 1 + IMM9;
            }
            else{
                newPC = CPU->PC + 1;
            }
            break;
        case 0x6:
            if (((PSR & 0x4) == 0x4) || ((PSR & 0x2) == 0x2)){ //negative or zero
                newPC = CPU->PC + 1 + IMM9;
            }
            else{
                newPC = CPU->PC + 1;
            }
            break;
        case 0x5:
            if (((PSR & 0x4) == 0x4) || ((PSR & 0x1) == 0x1)){ //negative or positive
                newPC = CPU->PC + 1 + IMM9;
            }
            else{
                newPC = CPU->PC + 1;
            }
            break;
        case 0x2:
            if ((PSR & 0x2) == 0x2){ //zero
                newPC = CPU->PC + 1 + IMM9;
            }
            else{
                newPC = CPU->PC + 1;
            }
            break;
        case 0x3: 
            if (((PSR & 0x2) == 0x2) || ((PSR & 0x1) == 0x1)){ //zero or positive
                newPC = CPU->PC + 1 + IMM9;
            }
            else{
                newPC = CPU->PC + 1;
            }
            break;
        case 0x1:
            if ((PSR & 0x1) == 0x1){ //positive
                newPC = CPU->PC + 1 + IMM9;
            }
            else{
                newPC = CPU->PC + 1;
            }
            break;
        case 0x7:
            newPC = CPU->PC + 1 + IMM9;
            break;
        default:
            newPC = CPU->PC + 1;
            break;
    }

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->NZPVal = 0;
    CPU->DATA_WE = 0;
    CPU->regInputVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);
    //update 
    CPU->PC = newPC;
    return;
}

/*
 * Parses rest of arithmetic operation and prints out.
 */
 //updates Rd, RegFileWE to 1, NZP WE to 1, NZPval to Rd, reginputval, R, CTLs, PC
void ArithmeticOp(MachineState* CPU, FILE* output)
{
    //get the instruction and opcode (add, mult, etc)
    //parse instruction to get rd, rs, rt and update CPU accordingly
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int sub_opcode = INSN_5(instruction);
    unsigned short int sub_sub_opcode = INSN_4_3(instruction);

    unsigned short int Rd = INSN_11_9(instruction);
    unsigned short int Rs = INSN_8_6(instruction);
    unsigned short int Rt = INSN_2_0(instruction);
    unsigned short int immediate = INSN_4_0(instruction);
    signed short int sign_bit = immediate >> 4; // Shift sign bit to the least significant bit
    signed short int IMM5 = (sign_bit ? 0xFFE0 : 0x0000) | immediate;

    signed short int dest_val = 0;

    switch(sub_opcode){
        case 0x0:
            switch(sub_sub_opcode){
                case 0x0:
                    dest_val = CPU->R[Rs] + CPU->R[Rt];
                    break;
                case 0x1:
                    dest_val = CPU->R[Rs] * CPU->R[Rt];
                    break;
                case 0x2:
                    dest_val = CPU->R[Rs] - CPU->R[Rt];
                    // fprintf(stdout, "%04X\n%04X\n%04X\n", CPU->R[Rs], CPU->R[Rt], dest_val);
                    break;
                case 0x3:
                    dest_val = CPU->R[Rs] / CPU->R[Rt];
                    break;
            }
            break;
        case 0x1:
            dest_val = CPU->R[Rs] + IMM5;
            break;
    }

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = dest_val;
    SetNZP(CPU, dest_val);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->R[Rd] = dest_val;
    CPU->PC = CPU->PC + 1;
    return;
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int sub_opcode = INSN_8_7(instruction);
    unsigned short int Rs = INSN_11_9(instruction);
    unsigned short int Rt = INSN_2_0(instruction);
    unsigned short int UIMM7 = INSN_6_0(instruction);
    signed short int sign_bit = UIMM7 >> 6; // Shift sign bit to the least significant bit
    signed short int IMM7 = (sign_bit ? 0xFF80 : 0x0000) | UIMM7;
    signed short int dest_val = 0;

    switch(sub_opcode){
        case 0x0:
            dest_val = (signed short) CPU->R[Rs] - (signed short) CPU->R[Rt];
            break;
        case 0x1:
            dest_val = (unsigned short) CPU->R[Rs] - CPU->R[Rt];
            break;
        case 0x2:
            dest_val = (signed short) CPU->R[Rs] - IMM7;
            break;
        case 0x3:
            dest_val = (unsigned short) CPU->R[Rs] - (unsigned short) UIMM7;
            break;
    }
    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 2;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = 0;
    SetNZP(CPU, dest_val);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->PC = CPU->PC + 1;
    return;

}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int sub_opcode = INSN_5_3(instruction);

    unsigned short int Rd = INSN_11_9(instruction);
    unsigned short int Rs = INSN_8_6(instruction);
    unsigned short int Rt = INSN_2_0(instruction);
    unsigned short int immediate = INSN_4_0(instruction);
    signed short int sign_bit = immediate >> 4; // Shift sign bit to the least significant bit
    signed short int IMM5 = (sign_bit ? 0xFFE0 : 0x0000) | immediate;

    short int dest_val = 0;

    switch(sub_opcode){
        case 0x0:
            dest_val = CPU->R[Rs] & CPU->R[Rt];
            break;
        case 0x1:
            dest_val = ~CPU->R[Rs];
            break;
        case 0x2:
            dest_val = CPU->R[Rs] | CPU->R[Rt];
            break;
        case 0x3:
            dest_val = CPU->R[Rs] ^ CPU->R[Rt];
            break;
    }
    if (INSN_5(instruction) == 0x1){
        //addition with intermediate
        dest_val = CPU->R[Rs] & IMM5;
    }


    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = dest_val;
    SetNZP(CPU, dest_val);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->R[Rd] = dest_val;
    CPU->PC = CPU->PC + 1;
    return;
}

/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int sub_opcode = INSN_11(instruction);

    unsigned short int Rs = INSN_8_6(instruction);

    unsigned short int immediate = INSN_10_0(instruction);
    signed short int sign_bit = immediate >> 10; // Shift sign bit to the least significant bit
    signed short int IMM11 = (sign_bit ? 0xFC00 : 0x0000) | immediate;

    unsigned short int newPC = 0;

    switch (sub_opcode){
        case 0x0:
            newPC = CPU->R[Rs];
            break;
        case 0x1:
            newPC = CPU->PC + 1 + IMM11;
            break;
    }
    
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->NZPVal = 0;
    CPU->DATA_WE = 0;
    CPU->regInputVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PC = newPC;
    return;

}

/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int sub_opcode = INSN_11(instruction);

    unsigned short int Rs = INSN_8_6(instruction);

    unsigned short int immediate = INSN_10_0(instruction);
    signed short int sign_bit = immediate >> 10; // Shift sign bit to the least significant bit
    signed short int IMM11 = (sign_bit ? 0xFC00 : 0x0000) | immediate;

    unsigned short int newPC = 0;
    unsigned short int dest_val = 0;

    switch (sub_opcode){
        case 0x0:
            dest_val = CPU->PC + 1;
            newPC = CPU->R[Rs];
            break;
        case 0x1:
            dest_val = CPU->PC + 1; 
            newPC = (CPU->PC & 0x8000) | (IMM11 << 4);
            break;
    }

    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 1;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = dest_val;
    SetNZP(CPU, dest_val);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->R[7] = dest_val;
    CPU->PC = newPC;
    return;
}

/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState* CPU, FILE* output)
{
    unsigned short int instruction = CPU->memory[CPU->PC];
    unsigned short int sub_opcode = INSN_5_4(instruction);

    unsigned short int Rd = INSN_11_9(instruction);
    unsigned short int Rs = INSN_8_6(instruction);
    unsigned short int Rt = INSN_2_0(instruction);
    unsigned short int UIMM4 = INSN_3_0(instruction);

    short int dest_val = 0;

    switch(sub_opcode){
        case 0x0:
            dest_val = CPU->R[Rs] << UIMM4;
            break;
        case 0x1:
            dest_val = ((short)CPU->R[Rs]) >> UIMM4;
            break;
        case 0x2:
            dest_val = CPU->R[Rs] >> UIMM4;
            break;
        case 0x3:
            dest_val = CPU->R[Rs] % CPU->R[Rt];
            break;
    }
    
    //generating control signals
    ClearSignals(CPU);
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    CPU->regInputVal = dest_val;
    SetNZP(CPU, dest_val);
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
    //writing to output file
    WriteOut(CPU, output);

    //updating registers
    CPU->PSR = CPU->PSR & ~(0x7); //clearing NZP bits in PSR
    CPU->PSR = CPU->PSR | CPU->NZPVal; //setting NZP bits in PSR
    CPU->R[Rd] = dest_val;
    CPU->PC = CPU->PC + 1;
    return;

}
/*
 * Set the NZPVal
 */
void SetNZP(MachineState* CPU, short result)
{
    if (result > 0){
        CPU->NZPVal =  0x1;
    }
    else if (result < 0){
        CPU->NZPVal = 0x4;
    }
    else{
        CPU->NZPVal = 0x2;
    }
}
