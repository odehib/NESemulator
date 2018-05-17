/*
6502 processor opcodes are written in the format aaabbbcc, where aaa and cc dictate the opcode (i.e. the instruction to be used), and bbb indicates the adressing mode
The look-up table consists of 32 rows (2^5) for all of the opcode instructions and 8 (2^3) columns for the addressing modes assigned to each opcode
Once the proper opcode and adressing mode is located, the necessary bytes of information are read from the ROM to perform the instruction
Reference used: http://www.llx.com/~nparker/a2/opcodes.html
*/

typedef  int (*MyFunctionType)( int8_t , int8_t);

MyFunctionType OP_LUT[32][8] {}
