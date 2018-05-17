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
int BIT(int8_t byte1, int8_t byte2);
int JMP_IND(int8_t byte1, int8_t byte2);
int JMP_ABS(int8_t byte1, int8_t byte2);
int STY(int8_t byte1, int8_t byte2);
int LDY(int8_t byte1, int8_t byte2);
int CPY(int8_t byte1, int8_t byte2);
int CPX(int8_t byte1, int8_t byte2);

//cc=01 instructions
int ORA(int8_t byte1, int8_t byte2);
int AND(int8_t byte1, int8_t byte2);
int EOR(int8_t byte1, int8_t byte2);
int ADC(int8_t byte1, int8_t byte2);
int STA(int8_t byte1, int8_t byte2);
int LDA(int8_t byte1, int8_t byte2);
int CMP(int8_t byte1, int8_t byte2);
int SBC(int8_t byte1, int8_t byte2);

//cc=10 instructions
int ASL(int8_t byte1, int8_t byte2);
int ROL(int8_t byte1, int8_t byte2);
int LSR(int8_t byte1, int8_t byte2);
int ROR(int8_t byte1, int8_t byte2);
int STX(int8_t byte1, int8_t byte2);
int LDX(int8_t byte1, int8_t byte2);
int DEX(int8_t byte1, int8_t byte2);
int INC(int8_t byte1, int8_t byte2);
