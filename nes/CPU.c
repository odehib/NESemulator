#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
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

/*BIT
sets the Z flag as though the value in the address tested were ANDed with the accumulator.
The N and V flags are set to match bits 7 and 6 respectively in the value stored at the tested address.
The output of AND is not stored in the accumulator
*/
int BIT()
{
  uint8_t temp;
  temp = regACC & (uint8_t)(*input);
  if(temp) regSTAT &= ~ZERO;  //set the zero flag high or low based on the logic output
  else regSTAT |= ZERO;

  temp = (uint8_t)(*input);

  //set the negative and overflow flags to that of the stored value
  if(temp&NEGATIVE) regSTAT |= NEGATIVE;
  else regSTAT &= ~NEGATIVE;

  if(temp&OVERFLOW) regSTAT |= OVERFLOW;
  else regSTAT &= ~OVERFLOW;

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
    regPC = (uint16_t*)input;
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
  }

  /* LDY
    LDY loads a value from memory into the Y register
    alters the N and Z flags as necessary regarding this value
  */
  int LDY()
  {
    regY = *input;
    if(!regY) regSTAT |= ZERO;  //set the zero flag to the necessary value
    else regSTAT &= ~ZERO;

    if(regY & NEGATIVE) regSTAT |= NEGATIVE;  //set the negative flag to the necessary value
    else regSTAT &= ~NEGATIVE;
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
    if(!temp) //zero case
    {
      regSTAT |= CARRY;
      regSTAT |= ZERO;
      regSTAT &= ~NEGATIVE;
    }

    else if(temp<0) //negative case
    {
      regSTAT |= NEGATIVE;
      regSTAT &= ~ZERO;
      regSTAT &= ~CARRY
    }

    else  //positive case
    {
       regSTAT &= ~ZERO;
       regSTAT &= ~NEGATIVE;
       regSTAT |= CARRY;
    }
  }

  /* CPX
    Compares the value at the input address to the X register
    sets the carry flag high if X>=val
    sets the zero flag high if X=val
    sets the negative flag high if X<val
  */
  int CPY()
  {
    int8_t temp;
    temp = regX - *input;

    //set the carry, zero, and negative flags as necessary
    if(!temp) //zero case
    {
      regSTAT |= CARRY;
      regSTAT |= ZERO;
      regSTAT &= ~NEGATIVE;
    }

    else if(temp<0) //negative case
    {
      regSTAT |= NEGATIVE;
      regSTAT &= ~ZERO;
      regSTAT &= ~CARRY
    }

    else  //positive case
    {
       regSTAT &= ~ZERO;
       regSTAT &= ~NEGATIVE;
       regSTAT |= CARRY;
    }
  }

  /* ORA
    bitwise OR with accumulator
    sets the N and Z flags as necessary
  */
  int ORA()
  {
    regACC |= *input;

    if(!regACC) regSTAT |= ZERO;
    else regSTAT &= ~ZERO;

    if(regACC & NEGATIVE) regSTAT |= NEGATIVE;
    else regSTAT &= ~NEGATIVE;
  }

  /* AND
    bitwise AND with accumulator
    sets the N and Z flags as necessary
  */
  int AND()
  {
    regACC &= *input;

    if(!regACC) regSTAT |= ZERO;
    else regSTAT &= ~ZERO;

    if(regACC & NEGATIVE) regSTAT |= NEGATIVE;
    else regSTAT &= ~NEGATIVE;
  }

  /* EOR
    bitwise XOR with accumulator
    set the N and Z flags as necessary
  */
  int EOR()
  {
    regACC ^= *input;

    if(!regACC) regSTAT |= ZERO;
    else regSTAT &= ~ZERO;

    if(regACC & NEGATIVE) regSTAT |= NEGATIVE;
    else regSTAT &= ~NEGATIVE;
  }

  /*ADC
    add with carry
    adds the contents of a memory location to the accumulator
  */
