/*
6502 processor opcodes are written in the format aaabbbcc, where aaa and cc dictate the opcode (i.e. the instruction to be used), and bbb indicates the addressing mode
The look-up table consists of 32 entries (2^5) for all of the opcode instructions
The table is separated into 8 columns (aaa) and 3 rows (cc, no instructions of the form cc=11)
Each function will have a case statement for each of its possible addressing modes (2^3)
Once the proper opcode and addressing mode is located, the necessary bytes of information are read from the ROM to perform the instruction
Reference used: http://www.llx.com/~nparker/a2/opcodes.html
*/

typedef int (*MyFunctionType)( int8_t , int8_t);

MyFunctionType OP_LUT[3][8] { {NULL, &BIT, &JMP_IND, &JMP_ABS, &STY, &LDY, &CPY, &CPX}, //cc=00
                              {&ORA, &AND, &EOR, &ADC, &STA, &LDA, &CMP, &SBC}, //cc=01
                              {&ASL, &ROL, &LSR, &ROR, &STX, &LDX, &DEX, &INC} //cc=10
                            };


//cc=00 instructions
int BIT();
int JMP_IND();
int JMP_ABS();
int STY();
int LDY();
int CPY();
int CPX();

//cc=01 instructions
int ORA();
int AND();
int EOR();
int ADC();
int STA();
int LDA();
int CMP();
int SBC();

//cc=10 instructions
int ASL();
int ROL();
int LSR();
int ROR();
int STX();
int LDX();
int DEX();
int INC();
