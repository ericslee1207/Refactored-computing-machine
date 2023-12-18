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
    if (CPU == NULL){
        return -1;
    }
    for(int i = 2; i < argc; i++){
        int res = ReadObjectFile(argv[i], CPU);
        if (res == 1){
            free(CPU);
            return -1;
        }
    }
    Reset(CPU);
    FILE *des_file = fopen(argv[1], "w");
    while(CPU->PC != 0x80FF){
        int update = UpdateMachineState(CPU, des_file);
        //updatemachinestate doesn't recognize pc, so it goes to writeout, which prints pc. 
        if (update == 1){
            break;
        }
    }
    fclose(des_file);
    free(CPU);
    return 0;
}