#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "CPU.h"

//TODO: figure out a way to remove if-statements if possible

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

//tools to be used for branching
#define XXFILTER 0xC0
#define YFILTER 0x20
int16_t BRVALS[4] = {NEGATIVE, OVERFLOW, CARRY, ZERO};

#define INTERRUPT_VECTOR 0xFFFE

//TODO: check for all of the flag changes necessary
//NOTE: S and N are equivalent labels for the same flag (sign/negative)

int8_t regSTAT;   //Processor's status register
uint16_t regPC;   //program counter
uint8_t regSP;    //stack pointer register
int8_t regACC;   //accumulator register, stores the output of artithmetic and logic operations
int8_t regX;    //general purpose register
int8_t regY;    //general purpose register
int8_t curr_instruction;  //the current instruction to be parsed

/*
NOTE: number ^= (-x ^ number) & (1 << n) sets the nth bit of "number" to x
For any value x, !!x sets the value to a boolean, as does !x
*/

/*BIT
sets the Z flag as though the value in the address tested were ANDed with the accumulator.
The N and V flags are set to match bits 7 and 6 respectively in the value stored at the tested address.
The output of AND is not stored in the accumulator

*/
int BIT()
{
  int8_t temp;
  temp = *input;
  regSTAT ^= (!(!(temp&regACC)) ^ regSTAT) & ZERO; //set the zero flag high if temp&regACC=0, low otherwise
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
    //check the address for page boundary condition
    //CPU memory map corresponding to these boundaries is depicted on page 11 of NESDoc.pdf
    int8_t * temp;
    temp_ptr = input - ROM; //use temp as a relative address to the start of the memory map
    if(temp<0x2000) //RAM page
    {
      temp = temp%0x800;  //the memory is mirrored every 0x800 bytes
      temp++;
      if((temp==0x800)||(temp==0x200)||(temp==0x100)) return -1;  //these values indicate the end of a page
    }
    else if(temp<0x4000)  //  IO regs 0-7
    {
      temp = temp%0x8;  //mirrored every 0x8 bytes
      if(temp==0x7) return -1;  //indicates the end of a page mirror
    }
    else if (temp==0x401F) return -1; //end of IO reg page
    else
    {
      temp++;
      if(temp==0x6000 ||  temp==0x8000  ||  temp==0xC000  ||  temp==0x10000) return -1; //all indictae end of a page
    }
    // if(!((temp+1)%0x2000)) return -1; //0x2000, 0x4000, 0x6000, 0x8000, and 0x10000 all mark new pages

    //TODO: CHECK IF THIS IS THE CORRECT JMP_IND IMPLEMENTATION (I think so, will unit test)
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
    regSTAT ^= (!(!regY) ^ regSTAT) & ZERO; //set the zero flag high if regY is zero, low otherwise
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
    regSTAT ^= (!(!temp) ^ regSTAT) & ZERO; //set the zero flag high if the output is zero, low otherwise
    regSTAT ^= (!(temp&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the subtraction output
    regSTAT ^= (!(!(temp&NEGATIVE)) ^ regSTAT) & CARRY; //set the carry flag to the opposite of the negative flag (since carry is set for any non-negative value)
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
    regSTAT ^= (!(!temp) ^ regSTAT) & ZERO; //set the zero flag high if the output is zero, low otherwise
    regSTAT ^= (!(temp&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the subtraction output
    regSTAT ^= (!(!(temp&NEGATIVE)) ^ regSTAT) & CARRY; //set the carry flag to the opposite of the negative flag (since carry is set for any non-negative value)
    return 0;
  }

  /* ORA
    bitwise OR with accumulator
    sets the N and Z flags as necessary
  */
  int ORA()
  {
    regACC |= *input;
    regSTAT ^= (!(!regACC) ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
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
    regSTAT ^= (!(!regACC) ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
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
    regSTAT ^= (!(!regACC) ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the accumulator value
    return 0;
  }

  /*ADC
    add with carry
    adds the contents of a memory location to the accumulator
    affects flags S, V, Z, and C
  */
  int ADC()
  {
    int8_t temp;
    int8_t output;
    int16_t carry_check;
    bool C;
    bool V;
    temp = *input;
    output = temp + regACC;
    carry_check = (int16_t)temp + (int16_t)regACC;
    C = (carry_check!=(int16_t)output); //if the 16 bit addition is not equivalent to the 8 bit addition, then a carry must have occurred

    /*check for overflow
    overflow occurs iff the sign bit for both inputs is different from the sign bit of the output
    i.e. neg+neg=pos or pos+pos=neg
    no overflow otherwise because pos+neg always yields an output smaller in magnitude than the largest value*/
    V = (bool)((regACC^output) & (temp^output) & NEGATIVE);
    regSTAT ^= (!V ^ regSTAT) & OVERFLOW; //set the overflow flag to V
    regSTAT ^= (!C ^ regSTAT) & CARRY; //set the carry flag to C
    regSTAT ^= (!(!output) ^ regSTAT) & ZERO; //set the zero flag high if the result is zero, low otherwise
    regSTAT ^= (!(output&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the output

    regACC = output;
    return 0;
  }

  /* STA
    STA stores the accumulator in a specified address
  */
  int STA()
  {
    *input = regACC;
    return 0;
  }

  /* LDA
    LDA loads a value from memory into the accumulator
    alters the N and Z flags as necessary regarding this value
  */
  int LDA()
  {
    regACC = *input;
    regSTAT ^= (!(!regACC) ^ regSTAT) & ZERO; //set the zero flag high if the accumulator is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to the necessary value
    return 0;
  }

  /* CMP
    Compares the value at the input address to the accumulator
    sets the carry flag high if ACC>=val
    sets the zero flag high if ACC=val
    sets the negative flag high if ACC<val
    TODO: LOOK INTO POTENTIAL OVERFLOW CONDITIONS
  */
  int CMP()
  {
    int8_t temp;
    temp = regACC - *input;

    //set the carry, zero, and negative flags as necessary
    regSTAT ^= (!(!temp) ^ regSTAT) & ZERO; //set the zero flag high if the output is zero, low otherwise
    regSTAT ^= (!(temp&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the subtraction output
    regSTAT ^= (!(!(temp&NEGATIVE)) ^ regSTAT) & CARRY; //set the carry flag to the opposite of the negative flag (since carry is set for any non-negative value)
    return 0;
  }

  /*SBC
    subtract with carry
    subtracts the contents of a memory location from the accumulator
    affects flags S, V, Z, and C
    C is set high if a borrow does NOT occur, low otherwise
    EXACTLY THE SAME AS ADC EXCEPT FOR "temp*=-1";
  */
  int SBC()
  {
    int8_t temp;
    int8_t output;
    int16_t carry_check;
    bool C;
    bool V;
    temp = *input;
    temp *= -1; //SBC EXACTLY THE SAME AS ADC EXCEPT FOR THIS LINE
    output = temp + regACC;
    carry_check = (int16_t)temp + (int16_t)regACC;
    C = (carry_check!=(int16_t)output); //if the 16 bit addition is not equivalent to the 8 bit addition, then a carry must have occurred

    /*check for overflow
    overflow occurs iff the sign bit for both inputs is different from the sign bit of the output
    i.e. neg+neg=pos or pos+pos=neg
    no overflow otherwise because pos+neg always yields an output smaller in magnitude than the largest value*/
    V = (bool)((regACC^output) & (temp^output) & NEGATIVE);
    regSTAT ^= (!V ^ regSTAT) & OVERFLOW; //set the overflow flag to V
    regSTAT ^= (!C ^ regSTAT) & CARRY; //set the carry flag to C
    regSTAT ^= (!(!output) ^ regSTAT) & ZERO; //set the zero flag high if the result is zero, low otherwise
    regSTAT ^= (!(output&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the output

    regACC = output;
    return 0;
  }

  /*ASL
    arithmetic shift left
    logically shift all bits left, storing bit 7 in the status register's carry flag, and the output in the accumulator
    bit 0 becomes 0
    alters S, Z, and C
  */
  int ASL()
  {
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & CARRY; //set the carry flag to the 7th bit of the given value
    *input = (*input)<<1; //left shift by one bit
    regSTAT ^= (!(!(*input)) ^ regSTAT) & ZERO; //set the zero flag high if the new value is zero, low otherwise
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to the necessary value
    return 0;
  }

  /*ROL
    rotate left
    logically shift all bits left, store the carry flag in bit 0 and store bit 7 in the carry flag
    same as ASL but with bit 0 carrying the value of C instead of 0
    alters S, C, Z
  */
  int ROL()
  {
    int8_t carry_original;
    carry_original = regSTAT & CARRY; //store the original carry before it is altered
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & CARRY; //set the carry flag to the 7th bit of the given value
    *input = ((*input)<<1) | (carry_original); //left shift by one bit and store the carry in bit 0 (CARRY is 0x01, storing the value in the correct bit)
    regSTAT ^= (!(!(*input)) ^ regSTAT) & ZERO; //set the zero flag high if the new value is zero, low otherwise
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to the necessary value
    return 0;
  }

  /*LSR
    logical shift right
    right shifts the value by one bit and stores bit 0 in the carry flag
    bit 7 becomes 0
    alters S, C, and Z
  */
  int LSR()
  {
    regSTAT ^= (!((*input)&(0x01)) ^ regSTAT) & CARRY; //set the carry flag to the 0th bit of the given value (0x01)
    *input = (*input)>>1; //right shift by one bit
    regSTAT ^= (!(!(*input)) ^ regSTAT) & ZERO; //set the zero flag high if the new value is zero, low otherwise
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to the necessary value
    return 0;
  }

  /*ROR
    rotate right
    logically shift all bits right, store the carry flag in bit 7 and store bit 0 in the carry flag
    same as LSR but with bit 7 carrying the value of C instead of 0
    alters S, C, Z
  */
  int ROR()
  {
    int8_t carry_original;
    carry_original = regSTAT & CARRY; //store the original carry before it is altered
    regSTAT ^= (!((*input)&(0x01)) ^ regSTAT) & CARRY; //set the carry flag to the 0th bit of the given value (0x01)
    *input = ((*input)>>1) | (carry_original<<7); //right shift by one bit and store the original carry status in bit 7 (hence the left shift of the carry by 7)
    regSTAT ^= (!(!(*input)) ^ regSTAT) & ZERO; //set the zero flag high if the new value is zero, low otherwise
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to the necessary value
    return 0;
  }

  /* STX
    STX stores the X register in a specified address
  */
  int STX()
  {
    *input = regX;
    return 0;
  }

  /*LDX
    LDX loads a value from memory into the X register
    alters the N and Z flags as necessary regarding this value
  */
  int LDY()
  {
    regX = *input;
    regSTAT ^= (!(!regX) ^ regSTAT) & ZERO; //set the zero flag high if regY is zero, low otherwise
    regSTAT ^= (!(regX&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to the necessary value
    return 0;
  }

  /*DEC
    decrement the value stored in memory
    update S and Z flags
  */
  int DEC()
  {
    (*input) = (*input) - 1;  //decrement the value by one
    regSTAT ^= (!(!(*input)) ^ regSTAT) & ZERO; //set the zero flag high if (*input)=0, low otherwise
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the new value in memory
  }

  /*INC
    increment the value stored in memory
    update S and Z flags
  */
  int INC()
  {
    (*input) = (*input) + 1;  //increment the value by one
    regSTAT ^= (!(!(*input)) ^ regSTAT) & ZERO; //set the zero flag high if (*input)=0, low otherwise
    regSTAT ^= (!((*input)&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the new value in memory
  }

  /*BRANCH
    All branch instructions defined in one function
    branch instructions are of the form xxy10000 followed by the displacement
  */
  int BRANCH()
  {
    int8_t xx, y;
    int32_t newPC;
    xx = curr_instruction & XXFILTER;
    y = curr_instruction & YFILTER;
    newPC = (int32_t)regPC + (int32_t)(*input); //cast as 32 bits to deal with subtraction from an unsigned number
    if((regSTAT & 1<<BRVALS[xx])==y) regPC = (uint16_t)newPC; //if the status value to be checked matches the y value, perform the branch
    return 0;
  }

  /* STACK INSTRUCTIONS */

  //TXS - store X in the stack pointer
  int TXS()
  {
    regSP = regX;
    return 0;
  }

  //store the stack pointer in X
  int TSX()
  {
    regX = regSP;
    regSTAT ^= (!(!regX) ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
    regSTAT ^= (!(regX&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the accumulator value
    return 0;
  }

  //PHA - push accumulator to stack
  int PHA()
  {
    stack[regSP] = regACC;
    regSP++;
    return 0;
  }

  //PLA - pull (pop) accumulator from stack
  //Set the N and Z flags to match the new accumulator value
  int PLA()
  {
    regSP--;
    regACC = stack[regSP];
    regSTAT ^= (!(!regACC) ^ regSTAT) & ZERO; //set the zero flag high if regACC is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the accumulator value
    return 0;
  }

  //PHP - push status to stack
  int PHP()
  {
    stack[regSP] = regSTAT;
    regSP++;
    return 0;
  }

  //PLP - pull (pop) status from stack
  int PLP()
  {
    regSP--;
    regSTAT = stack[regSP];
    return 0;
  }

  /* STATUS FLAG INSTRUCTIONS */

  //CLC - clear the carry flag
  int CLC()
  {
    regSTAT &= ~CARRY;
    return 0;
  }

  //SEC - set the carry flag
  int SEC()
  {
    regSTAT |= CARRY;
    return 0;
  }

  //CLI - clears the interrupt disable flag
  int CLI()
  {
    regSTAT &= ~INT_DISABLE;
    return 0;
  }

  //SEI - sets the interrupt disable flag
  int SEI()
  {
    regSTAT |= INT_DISABLE;
    return 0;
  }

  //CLV - clears the overflow flag
  int CLV()
  {
    regSTAT &= ~OVERFLOW;
    return 0;
  }

  //NOTE: probably should set the BCD fxns to return -1 since NES games shouldn't be calling it
  //CLD - clears the BCD flag
  int CLD()
  {
    regSTAT &= ~BCD;
    return 0;
  }

  //SED - sets the BCD flag
  int SED()
  {
    regSTAT |= BCD;
    return 0;
  }

  //DEY - decrement Y and update the S and Z flags
  int DEY()
  {
    regY--;
    regSTAT ^= (!(!regY) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regY&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //TAY - transfer accumulator to Y
  int TAY()
  {
    regY = regACC;
    regSTAT ^= (!(!regY) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regY&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //TYA - transfer Y to accumulator
  int TYA()
  {
    regACC = regY;
    regSTAT ^= (!(!regACC) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //INY - increment Y
  int INY()
  {
    regY++;
    regSTAT ^= (!(!regY) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regY&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //INX - increment X
  int INX()
  {
    regX++;
    regSTAT ^= (!(!regX) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regX&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //TAX - transfer accumulator to X
  int TAX()
  {
    regX = regACC;
    regSTAT ^= (!(!regX) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regX&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //TXA - transfer X to the accumulator
  int TXA()
  {
    regACC = regX;
    regSTAT ^= (!(!regACC) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regACC&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //DEX - decrement X
  int DEX()
  {
    regX--;
    regSTAT ^= (!(!regX) ^ regSTAT) & ZERO; //set the zero flag high if Y is zero, low otherwise
    regSTAT ^= (!(regX&NEGATIVE) ^ regSTAT) & NEGATIVE; //set the negative flag to that of the Y value
    return 0;
  }

  //NOP - no operation
  int NOP()
  {
    //does nothing besides moving on to the next instruction
    return 0;
  }

  /*JSR
  Jump to subroutine
  pushes the address of the next instruction minus 1 to the stack
  sets the program to the target memory address
  only uses absolute addressing and is the only instruction that does so in a different format than those in the OP_LUT
  */
  int JSR()
  {
    *((uint16_t*)(stack + (int8_t*)regSP)) = regPC;   //push the two byte PC to the stack
    regSP+=2; //two bytes stored in stack
    regPC = *((uint16_t*)input);  //jump to the subroutine by changing PC to the target address
    return 0;
  }

  /*RTS
  Return from subroutine
  pulls the value of PC-1 stored on the stack to return to previous operation
  implied addressing mode
  */
  int RTS()
  {
    regSP-=2;
    regPC = *((uint16_t*)(stack + (int8_t*)regSP)); //pop the two byte PC register from the stack
    regPC++;
    return 0;
  }

  /*BRK
  Force interrupt
  generates an interrupt request
  pushes the PC and status onto the stack
  the interrupt vector at $FFFE - $FFFF is loaded into PC
  the BRK flag is set to one
  pushes the PC first
  */
  int BRK()
  {
    *((uint16_t*)(stack + (int8_t*)regSP)) = regPC; //push the two byte PC to the stack
    regSP += 2;
    stack[regSP] = regSTAT; //push the single byte stack pointer to the stack
    regSP++;
    regPC = *((uint16_t*)(ROM + INTERRUPT_VECTOR)); //sets the PC value to the interrupt vector
    regSTAT |= BRK; //enable the break command flag
    return 0;
  }

  /*RTI
  Return from interrupt
  pops the status then the PC from the stack
  */
  int RTI()
  {
    regSP--;
    regSTAT = stack[regSP];
    regSP-=2;
    regPC = *((uint16_t*)(stack + (int8_t*)regSP)); //pop the two byte PC register from the stack
    return 0;
  }
