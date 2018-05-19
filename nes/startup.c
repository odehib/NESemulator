#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "startup.h"

#define CCFILTER 0x03  //filter only the last two bits of the instruction
#define BBBFILTER 0x1C  //filter bits 2-4 of the instruction
#define AAAFILTER 0xE0  //filter bits 5-7 of the instruction

int8_t * ROM;
int8_t * input; //address holding input for instruction
bool twobytes;  //boolean to dictate whether to read one or two bytes from the given address

int main()
{
  ROM = NULL; //temporary placeholder for compilation
  regSTAT = 0x20; //start the status register with only the unused bit enabled, as is standard
  return 0;
}

//TODO: check for invalid addressing modes called
int call_instruction()
{
  int8_t aaa;
  int8_t bbb;
  int8_t cc;
  int8_t * temp_ptr;  //temporary pointer
  int16_t twobyte_temp; //pointer to data that is two bytes long
  int8_t temp;
  twobytes = false;
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
          goto immediate;
        case 1:
          goto zp_abs;
        case 2:
          return -1;  //invalid instruction
        case 3:
          goto absolute;
        case 4:
          return -1;  //invalid instruction
        case 5:
          goto zp_index;
        case 6:
          return -1;  //invalid instruction
        case 7:
          goto abs_indX;
      }
    case 1: //cc=01
      switch (bbb) {
        case 0:
          goto indirX;
        case 1:
          goto zp_abs;
        case 2:
          goto immediate;
        case 3:
          goto absolute;
        case 4:
          goto indirY;
        case 5:
          goto zp_index;
        case 6:
          goto abs_indY;
        case 7:
          goto abs_indX;
    case 2:   //cc=10
      switch (bbb) {
        case 0:
          goto immediate;
        case 1:
          goto zp_abs;
        case 2:
          goto accumulator;
        case 3:
          goto absolute;
        case 4:
          return -1;  //invalid instruction
        case 5:
          goto zp_ind;
        case 6:
          return -1; //invalid instruction
        case 7:
          goto abs_indX;
    default:
      return -1;  //no instructions with cc=11
  }


  //instruction dictated by aaa and cc
  call:
    if(OP_LUT[cc][aaa]) return (*(OP_LUT[cc][aaa]))();  //call the necessary instruction and return its value
    else return -1; //return -1 if an invalid instruction was called (i.e. a null value in the array)




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
    twobytes = true;
    regPC+=3; //3 byte instruction
    goto call;

  /*ZERO PAGE INDEXED ADDRESSING MODE
  value to be used is stored in the address indicated by the immediate zero page address + X
  receive byte immediately following opcode and add it to the X register to get this address
  send this address to the instruction
  */
  zp_index:
    input = (int8_t*)(ROM[regPC+1] + regX);
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
    twobytes = true;
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
    twobytes = true;
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
    twobytes = true;
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
    twobytes = true;
    regPC += 2; //two byte instruction
    goto call;

  /*ACCUMULATOR ADDRESSING MODE
  corresponds to operations performed on the accumulator register value
  send a pointer to the accumulator register to the instruction
  */
  accumulator:
    input = (int8_t*)(&regACC);
    goto call;


}
