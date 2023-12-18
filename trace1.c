/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

// Global variable defining the current state of the machine
MachineState* CPU;

int main(int argc, char** argv)
{
    if (argc < 2){
        fprintf(stderr, "Not enough arguments");
        return -1;
    }
    if (argc == 2){
        fprintf(stderr, "Need to specify at least one obj file");
        return -1;
    }
    
    CPU = malloc(sizeof(MachineState));
    for(int i = 2; i < argc; i++){
        int res = ReadObjectFile(argv[i], CPU);
        if (res == 1){
            free(CPU);
            return -1;
        }
    }
    FILE *des_file = fopen(argv[1], "w");
    if (des_file != NULL){
        for (int i = 0; i < 65536; i++){
            if (CPU->memory[i] != 0x0000){
                fprintf(des_file, "address: %05d contents: 0x%04X\n", i, CPU->memory[i]);
            }
        }
    }
    fclose(des_file);
    free(CPU);
    return 0;
}