/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"
#include <stdio.h>
// memory array location
unsigned short memoryAddress;
//fprintf
/*
 * Read an object file and modify the machine state as described in the writeup
 */
int ReadObjectFile(char* filename, MachineState* CPU)
{
  FILE *src_file = fopen(filename, "rb");
  if (src_file == NULL){
    fclose(src_file);
    return 1;
  }
  while (1){
    //reading little endian method
    int first_byte_header = fgetc(src_file); //reading a byte is equivalent to reading a word (2 hexadecimal units)
    if (first_byte_header == EOF){
      break;
    }
    int second_byte_header = fgetc(src_file);

    // fprintf(stdout, "%x\n", first_byte_header);
    // fprintf(stdout, "%x\n", second_byte_header);
    
    if ((first_byte_header == 0xCA && second_byte_header == 0xDE) || (first_byte_header == 0xDA && second_byte_header == 0xDA)){
      int first_byte_address = fgetc(src_file);
      // fprintf(stdout, "%x\n", first_byte_address);
      int second_byte_address = fgetc(src_file);
      // fprintf(stdout, "%x\n", second_byte_address);
      int first_byte_n = fgetc(src_file);
      int second_byte_n = fgetc(src_file);
      // fprintf(stdout, "%x\n", first_byte_n);
      // fprintf(stdout, "%x\n", second_byte_n);
      unsigned short int address_combined = (first_byte_address << 8) | second_byte_address;
      unsigned short int n_combined = (first_byte_n << 8) | second_byte_n;
      while(n_combined > 0){
        int first_byte_instruction = fgetc(src_file);
        int second_byte_instruction = fgetc(src_file);

        unsigned short int instruction_combined = (first_byte_instruction << 8) | second_byte_instruction;
        // CPU->memory[address_combined] = instruction_combined;
        *((*CPU).memory + address_combined) = instruction_combined;
        n_combined = n_combined - 1;
        address_combined = address_combined + 1;
      }
    }
    else if (first_byte_header == 0xC3 && second_byte_header == 0xB7){
      int first_byte_address = fgetc(src_file);
      int second_byte_address = fgetc(src_file);

      int first_byte_n = fgetc(src_file);
      int second_byte_n = fgetc(src_file);

      unsigned short int address_combined = (first_byte_address << 8) | second_byte_address;
      unsigned short int n_combined = (first_byte_n << 8) | second_byte_n;

      while (n_combined > 0){
        int x = fgetc(src_file);
        n_combined = n_combined - 1;
      }
    }
    else if (first_byte_header == 0xF1 && second_byte_header == 0x7E){
        int first_byte_n = fgetc(src_file);
        int second_byte_n = fgetc(src_file);
        unsigned short int n_combined = (first_byte_n << 8) | second_byte_n;
    }
    else if (first_byte_header == 0x71 && second_byte_header == 0x5E){
        int first_byte_address = fgetc(src_file);
        int second_byte_address = fgetc(src_file);

        int first_byte_line = fgetc(src_file);
        int second_byte_line = fgetc(src_file);

        int first_byte_filei = fgetc(src_file);
        int second_byte_filei = fgetc(src_file);
    }
  }
  fclose(src_file);
  return 0;
}