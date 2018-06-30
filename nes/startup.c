#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "startup.h"

//TODO: get rid of goto statements and replace them with functions
//NOTE: goto's are considered bad style and should be avoided to make code \
more readable

#define CCFILTER 0x03  //filter only the last two bits of the instruction
#define BBBFILTER 0x1C  //filter bits 2-4 of the instruction
#define AAAFILTER 0xE0  //filter bits 5-7 of the instruction
#define SIZE 0x10000  //size of the memory map

int8_t * ROM;
int8_t * input; //address holding input for instruction
int8_t * stack;
int8_t * nes_file;
int8_t header; //header of the nes_file
FILE * fp;

int main()
{
  //TODO: fix fgets since it stops at EOF
  fp = fopen("[filename].NES", r); //temporary placeholder for compilation FIXME
  nes_file = malloc(SIZE); //create heap memory for the contents of the nes file
  if(!fgets(nes_file, SIZE+1, fp))  //copy SIZE+1 data from the file into memory (plus one for EOF)
  {
    fclose("[filename].NES"); //FIXME (name)
    free(NES_file); //deallocate the dynamic memory
    return -1;
  }

  //close the file when done copying the memory
  fclose("[filename].NES"); //FIXME (name)


  regSTAT = 0x20; //start the status register with only the unused bit enabled, as is standard



  cleanup:

  return 0;
}

int call_instruction()
{
  int8_t aaa;
  int8_t bbb;
  int8_t cc;
  int8_t * temp_ptr;  //temporary pointer
  int8_t temp;
  curr_instruction = ROM[regPC];
  aaa = (curr_instruction & AAAFILTER)>>5;  //shift by five bits to make aaa the least significant bits
  bbb = (curr_instruction & BBBFILTER)>>2;  //shift by two bits to make bbb the least significant bits
  cc = curr_instruction&CCFILTER; //no need to shift since cc are already the least significant bits

  //addressing modes described in detail at http://www.emulator101.com/6502-addressing-modes.html
  //opcode disambiguations described in detail at http://www.llx.com/~nparker/a2/opcodes.html#chart
  //addressing mode dictated by cc and bbb
  switch (cc) {
    case 0: //cc=00
      switch (bbb) {
        case 0:
          temp = curr_instruction>>4; //check the first four bits to parse this mode
          //bbb=000 guarantees an even 'temp' value, as is required for these instructions
          if (temp>8)  goto immediate; //immediate addressing modes all have a 'temp' value greater than 8
          else if (temp<8)  goto X0;  //all X0 instructions have a 'temp' value less than 8
          else return -1; //no instruction for 0x80
        case 1:
          if(aaa>>1 == 1) return -1;  //zp_abs is invalid for JMP and JMP_ABS in this section (aaa= 010 and 011 respectively)
          goto zp_abs;
        case 2:
          //ALL VALID HERE
          goto X8;  //non-convential mode
        case 3:
          //ALL VALID HERE
          goto absolute;
        case 4:
          //ALL VALID HERE
          goto branch;  //non-convential mode
        case 5:
          if(aaa>>1 == 2) goto zp_indexX;  //i.e. ZP, X
          return -1;  //zp_indexX only valid for STY and LDY in this section (aaa=100 and 101 respectively)
        case 6:
          //ALL VALID HERE
          goto X8;  //non-convential mode
        case 7:
          if(aaa==5)  goto abs_indX;  //i.e. absolute, X
          return -1;  //abs_indX only valid for LDY in this section (aaa=101)
      }
    case 1: //cc=01
      switch (bbb) {
        case 0:
          goto indirX;
        case 1:
          goto zp_abs;
        case 2:
          //STA imm8 (aaa=4, bbb=2) is the ONLY invalid instruction for cc=01
          if(aaa!=4)  goto immediate;
          return -1;
        case 3:
          goto absolute;
        case 4:
          goto indirY;
        case 5:
          goto zp_indexX;
        case 6:
          goto abs_indY;
        case 7:
          goto abs_indX;
    case 2:   //cc=10
      switch (bbb) {
        case 0:
          if(aaa==5) goto immediate;
          return -1;  //ldx (aaa=5) is the only valid instruction here
        case 1:
          //ALL VALID HERE
          goto zp_abs;
        case 2:
          if(aaa>>2) goto XA; //non-convential mode that's valid here if aaa>=4 (bitshift by two is division by 4, which yields zero if aaa<4)
          goto accumulator; //all other codes in this section are valid accumulator addressing instructions
        case 3:
          //ALL VALID HERE
          goto absolute;
        case 4:
          return -1;  //invalid instruction (for the original 6502)
        case 5:
          if(aaa>>1 == 2) goto zp_indexY; //for stx and ldx (aaa==100,101), use zp, Y instead of zp, X
          goto zp_indexX;
        case 6:
          //for bbb=110, cc=10, the only valid modes for aaa are 100 and 101 - this if statements checks for those values
          if(aaa>>1 == 2) goto XA;  //non-convential mode
          else return -1; //invalid instruction otherwise
        case 7:
          if(aaa=5) goto abs_indY;  //ldx (aaa=5) uses abs_indY instead of abs_indX
          else if(aaa!=4) goto abs_indX;  //stx (aaa=4) is invalid in this mode
          return -1;
    default:
      return -1;  //no instructions with cc=11
  }





  /*NECESSARY LABELS FOR ADDRESSING MODES*/

  /*IMMEDIATE ADDRESSING MODE
  value to be used is the byte immediately after the opcodes
  send a pointer to this value to the instruction
  */
immediate:
    input = ROM + (int8_t*)regPC + 1;   //#immediate addressing mode - 8 bit value
    regPC += 2;               //two bytes used by opcode and immediate byte, increment by two bytes
    goto call;

  /*ZERO PAGE ADDRESSING MODE
  value to be used is stored in the zero-page (i.e. one byte) address stored immediately after the opcode
  send this address to the instruction
  */
zp_abs:
    input = (int8_t*)(ROM[regPC + 1]);  //zero page absolute addressing
    regPC += 2;   //two byte instruction
    goto call;

  /*ABSOLUTE ADDRESSING MODE
  value to be used is stored in the absolute address (i.e. two bytes) stored immediately after the opcode
  send this address to the instruction
  NOTE: although the address is a two-byte address, it can still be stored as int8_t* since this just specifies the length of the value it points to
  (I have a common misconception that it must be int16_t* instead, which is NOT true)
  */
absolute:
    temp_ptr = ROM + (int8_t*)regPC + 1;  //use temp pointer to add one byte to the address and get the immediate two byte value
    input = (int8_t*)(*((int16_t*)temp_ptr));   //set input as two bytes at specified temp_ptr address and store it as a character pointer
    regPC+=3; //3 byte instruction
    goto call;

  /*ZERO PAGE INDEXED ADDRESSING MODE (regX)
  value to be used is stored in the address indicated by the immediate zero page address + X
  receive byte immediately following opcode and add it to the X register to get this address
  send this address to the instruction
  */
zp_indexX:
    input = (int8_t*)(ROM[regPC+1] + regX);
    regPC+=2; //two byte instruction
    goto call;

  /*ZERO PAGE INDEXED ADDRESSING MODE (regY)
  value to be used is stored in the address indicated by the immediate zero page address + Y
  receive byte immediately following opcode and add it to the Y register to get this address
  send this address to the instruction
  */
zp_indexY:
    input = (int8_t*)(ROM[regPC+1] + regY);
    regPC+=2; //two byte instruction
    goto call;


  /*ABSOLUTE X INDEXED ADDRESSING MODE
  address to be used is the two bytes immediately following the opcode added to the X register
  send this address to the instruction
  */
abs_indX:   //i.e. $1004, X
    temp_ptr = ROM + (int8_t*)regPC + 1;  //use temp pointer to add one byte to the address and get the immediate two byte value
    input = (int8_t*)(*((int16_t*)temp_ptr));   //set input as two bytes at specified temp_ptr address and store it as a character pointer
    input += (int8_t*)regX; //add X to the input value
    regPC+=3; //3 byte instruction
    goto call;

  /*ABSOLUTE Y INDEXED ADDRESSING MODE
  address to be used is the two bytes immediately following the opcode added to the Y register
  send this address to the instruction
  */
abs_indY: //i.e. $1004, Y
    temp_ptr = ROM + (int8_t*)regPC + 1;  //use temp pointer to add one byte to the address and get the immediate two byte value
    input = (int8_t*)(*((int16_t*)temp_ptr));   //set input as two bytes at specified temp_ptr address and store it as a character pointer
    input += (int8_t*)regY; //add Y to the input value
    regPC+=3; //3 byte instruction
    goto call;

  /*INDEXED INDIRECT ADDRESSING MODE (not to be confused with "indirect indexed")
  address to be used is stored in an address specified by the immediate byte plus the X register
  addition overflow is cyclical, ensuring the target is always stored in the zero page
  send the address stored in the zero page to the instruction
  */
indirX:  //indexed indirect i.e. ($20, X)
    temp_ptr = (int8_t*)(ROM[regPC + 1] + regX);  //receive immediate byte for zero page index and add it to X
    temp_ptr = temp_ptr & 0xFF; //bitmask to have circular address loop in the zero page
    input = (int8_t*)(*((int16_t*)temp_ptr)); //receive two bytes stored at temp_ptr value
    regPC += 2; //two byte instruction
    goto call;

  /*INDIRECT INDEXED ADDRESSING MODE (not to be confused with "indexed indirect")
  address to be used is a zero-page address' content added to the Y register. This zero-page address is specified by the immediate byte after the opcode
  mathematically: input = regY + zeropage[imm8]
  send this input to the instruction
  */
indirY: //indirect indexed i.e. ($86), Y
    temp = ROM[regPC + 1];
    input = (int8_t*)(*((int16_t*)temp)); //store the two bytes stored at the designated zero page address in input
    input += (int8_t*)regY; //add Y to the input value
    regPC += 2; //two byte instruction
    goto call;

  /*ACCUMULATOR ADDRESSING MODE
  corresponds to operations performed on the accumulator register value
  send a pointer to the accumulator register to the instruction
  */
accumulator:
    input = &regACC;
    goto call;

  /*BRANCH FUNCTIONS
  all branch instructions confined to one function
  offset to be used is the byte after the opcode
  pass this value to the branch function
  */
branch:
    input = ROM + (int8_t*)regPC + 1; //input to be branched by is the single byte after the opcode
    regPC+=2; //two byte instruction
    return BRANCH();

  /* all single byte opcodes that end in 0x8 are confined to their own array for simplicity */
X8:
    regPC++;  //single byte instructions
    return (*(X8_LUT[(curr_instruction>>4)]))();      //bitshift by 4 to get the 4 msb's to distinguish each function

X0:
    input = ROM + (int8_t*)regPC + 1; //input for absolute addressing mode for JSR
    regPC += 1 + ((temp==2) * 2); //temp=2 indicates an absolute JSR, which is a 3 byte instruction - all others are a single byte (implied addressing modes)
    temp>>2;  //divide temp by two to index into the X0 array
    return (*(X0_LUT[temp]))(); //call the function corresponsing to the first four bits

XA:
    regPC++;  //all instructions here are a single byte long
    temp = curr_instruction>>4;
    if(temp==0xE) return NOP(); //doesn't fit with the indexing scheme below and is therefore called independently
    temp-=(0x8);  //instructions start at 0x8A, so subtract 8 from 'temp' (the first four bits of the opcode)
    return (*(XA_LUT[temp]))();



    //change call to its own function that takes in a function pointer? TODO
    //instruction dictated by aaa and cc
call:
    if(OP_LUT[cc][aaa]) return (*(OP_LUT[cc][aaa]))();  //call the necessary instruction and return its value
    else return -1; //return -1 if an invalid instruction was called (i.e. a null value in the array)
    //some functions have a format that will point to the NULL entry in this array, but those calls should never reach this subroutine



}
