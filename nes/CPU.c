#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "CPU.h"

//enable status bits by ORing the proper constant defined below
//disable status bits by ANDing the inverse of the proper constant defined below
#define CARRY 0x01;
#define ZERO 0x02;
#define INT_DISABLE 0x04;
#define BCD 0x08;
#define BRK 0x10;
//bit 5 is unused - always 1
#define OVERFLOW 0x40;
#define NEGATIVE 0x80;
#define BBBFILTER 0x1C;


int8_t regSTAT;   //Processor's status register
uint16_t regPC;   //program counter
int8_t regSP;    //stack pointer register
int8_t regACC;   //accumulator register
int8_t regX;    //general purpose register
int8_t regY;    //general purpose register
int8_t curr_instruction;  //the current instruction to be parsed

BIT(int8_t byte1, int8_t byte2)
{
  

  return 0;
}
