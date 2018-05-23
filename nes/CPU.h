/*
6502 processor opcodes are written in the format aaabbbcc, where aaa and cc dictate the opcode (i.e. the instruction to be used), and bbb indicates the addressing mode
The look-up table consists of 32 entries (2^5) for all of the opcode instructions
The table is separated into 8 columns (aaa) and 3 rows (cc, no instructions of the form cc=11)
Each function will have a case statement for each of its possible addressing modes (2^3)
Once the proper opcode and addressing mode is located, the necessary bytes of information are read from the ROM to perform the instruction
Reference used: http://www.llx.com/~nparker/a2/opcodes.html
*/

//NOTE: $ implies a hex value, X is used to indicate an arbitrary 4-bit hex value

typedef int (*MyFunctionType)(void);

//functions for all opcodes using convential addressing modes
MyFunctionType OP_LUT[3][8] { {NULL, &BIT, &JMP_IND, &JMP_ABS, &STY, &LDY, &CPY, &CPX}, //cc=00
                              {&ORA, &AND, &EOR, &ADC, &STA, &LDA, &CMP, &SBC}, //cc=01
                              {&ASL, &ROL, &LSR, &ROR, &STX, &LDX, &DEC, &INC} //cc=10
                            };

//functions for all opcodes ending in 0x8
MyFunctionType X8_LUT[16] = {&PHP, &CLC, &PLP, &SEC, &PHA, &CLI, &PLA, &SEI,
                          &DEY, &TYA, &TAY, &CLV, &INY, &CLD, &INX, &SED};

MyFunctionType X0_LUT[4] = {&BRK, &JSR, &RTI, &RTS};  //four non-uniform instructions with opcodes in the $X0 format

MyFunctionType XA_LUT[5] = {&TXA, &TXS, &TAX, &TSX, &DEX};  //five non-uniform instructions with opcodes in the $XA format (excluding NOP, which is handled in startup.c)

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
int DEC();
int INC();

//leftover non-uniform opcodes
int BRANCH();

//$X8 functions
int PHP();
int CLC();
int PLP();
int SEC();
int PHA();
int CLI();
int PLA();
int SEI();
int DEY();
int TYA();
int TAY();
int CLV();
int INY();
int CLD();
int INX();
int SED();

//$XA functions
int TXS();
int TSX();
int TAX();
int TXA();
int DEX();
int NOP();

//$X0 functions
int BRK();
int JSR();
int RTI();
int RTS();
