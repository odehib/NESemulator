#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "CPU.h"

//enable status bits by ORing the proper constant defined below
//disable status bits by ANDing the inverse of the proper constant defined below
#define CARRY 0x01
#define ZERO 0x02
#define INT_DISABLE 0x04
#define BCD 0x08
#define BRK 0x10
//bit 5 is unused - always 1
#define OVERFLOW 0x40
#define NEGATIVE 0x80
#define BBBFILTER 0x1C

//TODO: check for all of the flag changes necessary

int8_t regSTAT;   //Processor's status register
uint16_t regPC;   //program counter
uint8_t regSP;    //stack pointer register
uint8_t regACC;   //accumulator register, stores the output of artithmetic and logic operations
int8_t regX;    //general purpose register
int8_t regY;    //general purpose register
int8_t curr_instruction;  //the current instruction to be parsed

/*
NOTE: number ^= (-x ^ number) & (1 << n) sets the nth bit of "number" to x
for any value x, !!x sets the value to a boolean, as does !x
*/

/*BIT
sets the Z flag as though the value in the address tested were ANDed with the accumulator.
The N and V flags are set to match bits 7 and 6 respectively in the value stored at the tested address.
The output of AND is not stored in the accumulator

*/
int BIT()
{
  uint8_t temp;
  temp = (uint8_t)(*input);
  regSTAT ^= ((temp&&regACC) ^ regSTAT) & ZERO; //set the zero flag high if temp=0, low otherwise
  //&& is a logical and and will output a single bit

  //set the negative and overflow flags to that of the stored value
  regSTAT ^= (!(temp&NEGATIVE) ^ regSTAT) & NEGATIVE;
  regSTAT ^= (!(temp&OVERFLOW) ^ regSTAT) & OVERFLOW;

  return 0;
}

  /*JMP_IND
  Transfers program execution to the location contained in the following address (indirect).
  NOTE: AN INDIRECT JUMP MUST NEVER USE A VECTOR BEGINNING ON THE LAST BYTE OF A PAGE
  This will loop around for the second byte rather than grabbing from the next page
  */
  int JMP_IND()
  {
    //TODO: CHECK THE PAGES FOR THE PROPER BOUNDARY CONDITIONS
    //TODO: CHECK IF THIS IS THE CORRECT JMP_IND IMPLEMENTATION
    regPC = (uint16_t*)(*((uint16_t*)input)); //indirect jump, so dereference to get the proper address
    return 0;
  }

  /*JMP_ABS
  JMP transfers program execution to the following address (absolute)
  NOTE: we can consider jmp_abs as a different opcode from jmp_ind instead of a different addressing modes
  just for simplicity sake, allowing us to parse it by its aaa value (they have different aaa values)
  NOTE: JMP_ABS and JMP_IND technically have the same addressing mode (same valid bbb and cc values)
  TODO: look more into this for potential bugs
  */
  int JMP_ABS()
  {
    regPC = (uint16_t*)input;
    return 0;
  }

  /* STY
    STY stores the Y register in a specified address
  */
  int STY()
  {
    *input = regY;
    return 0;
  }

  /* LDY
    LDY loads a value from memory into the Y register
    alters the N and Z flags as necessary regarding this value
  */
  int LDY()
  {
    regY = *input;
    regSTAT ^= (!!regY ^ regSTAT) & ZERO; //set the zero flag high if regY is zero, low otherwise
    regSTAT ^= (!(regY&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to the necessary value
    return 0;
  }

  /* CPY
    Compares the value at the input address to the Y register
    sets the carry flag high if Y>=val
    sets the zero flag high if Y=val
    sets the negative flag high if Y<val
  */
  int CPY()
  {
    int8_t temp;
    temp = regY - *input;

    //set the carry, zero, and negative flags as necessary
    regSTAT ^= (!!temp ^ regSTAT) & ZERO; //set the zero flag high if the output is zero, low otherwise
    regSTAT ^= (!(temp&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the subtraction output
    regSTAT ^= ((temp&&NEGATIVE) ^ regSTAT) & CARRY; //set the carry flag to the opposite of the negative flag (since carry is set for any non-negative value)
    return 0;
  }

  /* CPX
    Compares the value at the input address to the X register
    sets the carry flag high if X>=val
    sets the zero flag high if X=val
    sets the negative flag high if X<val
  */
  int CPX()
  {
    int8_t temp;
    temp = regX - *input;

    //set the carry, zero, and negative flags as necessary
    regSTAT ^= (!!temp ^ regSTAT) & ZERO; //set the zero flag high if the output is zero, low otherwise
    regSTAT ^= (!(temp&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the subtraction output
    regSTAT ^= ((temp&&NEGATIVE) ^ regSTAT) & CARRY; //set the carry flag to the opposite of the negative flag (since carry is set for any non-negative value)
    return 0;
  }

  /* ORA
    bitwise OR with accumulator
    sets the N and Z flags as necessary
  */
  int ORA()
  {
    regACC |= *input;
    regSTAT ^= (!!regACC ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the accumulator value
    return 0;
  }

  /* AND
    bitwise AND with accumulator
    sets the N and Z flags as necessary
  */
  int AND()
  {
    regACC &= *input;
    regSTAT ^= (!!regACC ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the accumulator value
    return 0;
  }

  /* EOR
    bitwise XOR with accumulator
    set the N and Z flags as necessary
  */
  int EOR()
  {
    regACC ^= *input;
    regSTAT ^= (!!regACC ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the accumulator value
    return 0;
  }

  /*ADC
    add with carry
    adds the contents of a memory location to the accumulator
  */
  int ADC()
  {

    return 0;
  }
