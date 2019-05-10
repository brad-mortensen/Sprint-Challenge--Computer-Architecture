#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include <string.h>

// #define DATA_LEN 6

unsigned char cpu_ram_read(struct cpu *cpu, unsigned char index)
{
  return cpu->ram[index];
}

void cpu_ram_write(struct cpu *cpu, unsigned char index, unsigned char val)
{
  cpu->ram[index] = val;
}

/**
 * Load the binary bytes from a .ls8 source file into a RAM array
 */
void cpu_load(struct cpu *cpu, char *fn_arg)
{
  // TODO: Replace this with something less hard-coded
  FILE *fp;
  char line[1024];
  int address = 0;

  fp = fopen(fn_arg, "r");

  if (fp == NULL)
  {
    fprintf(stderr, "comp: error opening file\n");
    exit(2);
  }
  while (fgets(line, 1024, fp) != NULL)
  {
    char *endptr;

    unsigned int val = strtoul(line, &endptr, 2);

    if (endptr == line)
    {
      //printf("Found no digits\n");
      continue;
    }
    // printf("%u\n", val);
    cpu->ram[address] = val;
    address++;
  }

  fclose(fp);
}

/**
 * ALU
 */
void alu(struct cpu *cpu, unsigned char op, unsigned char regA, unsigned char regB)
{
  switch (op)
  {
  case MUL:
    cpu->registers[regA] = cpu->registers[regA] * cpu->registers[regB];
    break;
  case ADD:
    cpu->registers[regA] = cpu->registers[regA] + cpu->registers[regB];
    break;
  case CMP:
    if (regA == regB)
    {
      // set the equal flag on
      cpu->FL |= 0b00000001;
    }
    else if (regA < regB)
    {
      // Set the L flag on
      cpu->FL |= 0b00000100;
    }
    else
    {
      // Set greater than flag on
      cpu->FL |= 0b00000010;
    }

    break;
  default:
    break;
    // TODO: implement more ALU ops
  }
}
//
/**
 * Run the CPU
 */
void cpu_run(struct cpu *cpu)
{
  int running = 1; // True until we get a HLT instruction
  unsigned char operandA;
  unsigned char operandB;
  cpu->registers[SP] = 0xf4; // Stack pointer initialized to ff
  cpu->FL = 0;
  while (running)
  {
    unsigned int ops;
    // TODO
    // 1. Get the value of the current instruction (in address PC).
    unsigned char ir = cpu_ram_read(cpu, cpu->PC);
    // 2. Figure out how many operands this next instruction requires
    ops = (ir >> 6) + 1; // number of operations are stored in bits #6-7
    // 3. Get the appropriate value(s) of the operands following this instruction
    operandA = cpu_ram_read(cpu, cpu->PC + 1);
    operandB = cpu_ram_read(cpu, cpu->PC + 2);
    // 4. switch() over it to decide on a course of action.
    if (((ir >> 5) & 0b11111001) == 1)
    {
      alu(cpu, ir, operandA, operandB);
    }
    else
    {
      switch (ir)
      {
        // 5. Do whatever the instruction should do according to the spec.
      case JNE:
        if ((cpu->FL & 0b00000001) != 1)
        {
          cpu->PC = cpu->registers[operandA];
        }
        break;
      case JEQ:
        if ((cpu->FL & 0b00000001) == 1)
        {
          cpu->PC = cpu->registers[operandA];
        }
        break;
      case JMP:
        cpu->PC = cpu->registers[operandA];
      case CALL:
        // Push return address to the stack
        cpu->registers[SP]--;
        cpu_ram_write(cpu, cpu->registers[SP], cpu->PC + ops);

        // set the pc
        cpu->PC = cpu->registers[operandA] - ops;
        break;
      case RET:
        cpu->PC = cpu_ram_read(cpu, cpu->registers[SP]) - ops;
        // 2. Increment `SP`.
        cpu->registers[SP]++;
        break;
      case PUSH:
        // 1. Decrement the `SP` held in R7.
        cpu->registers[SP]--;
        // 2. Copy the value in the given register to the address pointed to by `SP`.
        cpu_ram_write(cpu, cpu->registers[SP], cpu->registers[operandA]);
        break;
      case POP:
        // 1. Copy the value from the address pointed to by `SP` to the given register.
        cpu->registers[operandA] = cpu_ram_read(cpu, cpu->registers[SP]);
        // 2. Increment `SP`.
        cpu->registers[SP]++;
        break;
      case LDI: // 2 operands
        // set the value of a register to an integer
        cpu->registers[operandA] = operandB;

        break;
      case PRN: // PRN, 1 operands
        // Print to the console the decimal integer value stored in the given register
        printf("%d\n", cpu->registers[operandA]);
        break;
      case NOP: // NOP, Continue, no ops
        continue;
      case HLT: // HLT, no operands
        running = 0;
        break;
      default: // instruction not found
        printf("Unknown instruction at PC: %d", cpu->PC);
        exit(1);
      }
    }
    // 6. Move the PC to the next instruction.
    cpu->PC += ops;
  }
}

/**
 * Initialize a CPU struct
 */
void cpu_init(struct cpu *cpu)
{
  // TODO: Initialize the PC and other special registers
  cpu = malloc(sizeof(cpu));
  cpu->PC = 0;
  memset(cpu->registers, 0, 8);
  memset(cpu->ram, 0, sizeof(cpu->ram));
}
