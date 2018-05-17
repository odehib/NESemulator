#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "startup.h"

int8_t * ROM;

int main()
{
  ROM = NULL; //temporary placeholder for compilation
  regSTAT = 0x20; //start the status register with only the unused bit enabled, as is standard
  return 0;
}

void call_instruction()
{
  curr_instruction = ROM[regPC];
  regPC++;
}
