#include <stdio.h>
//Kyle Dunn presentation
//--------------------
//Module 1: CPU
//program counter - tracks memory
int PC = 0;
//Accumulator! - holds data in multi-step calculations
int ACC = 0;
//Instruction Register - stores the instruction
int IR = 0;
//Status flags - temp markers that CPU uses to remember certain conditions
int statusRegister = 0;
//for instructions and data
int memory[] = {1,2,3,4};
int dataMemory[1];
//Loads value to the ACC
#define LOAD  1
//Adds value to ACC
#define ADD   2
//Subtracts value from ACC
#define SUB   3
//Stores ACC value to memory
#define STORE 4
//Stops program execution
#define HALT  5
int fetchInstruction() {
    //Fetches the instructions and increments the PC
    return memory[PC++]; }
void executeInstruction(int opcode) {
    switch(opcode) {
        //ADD operation
        case ADD:ACC += 10;
            break;
            //SUB operation
        case SUB:ACC -= 9;
            break;}}
int main() {
    while (PC < sizeof(memory)/sizeof(memory[0])) {
        //Fetches instruction
        IR = fetchInstruction();
        // Execute instruction
        executeInstruction(IR);
        // Print current ACC value
        printf("ACC: %d\n", ACC);
    }return 0;}
//--------------------
// Module 2: Memory System
//RAM rams out
int RAM[100] = {0};
//Cache - the main memory for storing data and programs
//Level1 Cache
int cacheLevel1[10] = {0};
//Level2 Cache
int cacheLevel2[20] = {0};
//reads from memory
int readMemory(int address) {
    //Simulate cache logic
    if (address < 10) {
        return cacheLevel1[address];
    } else {
        return RAM[address]; }}
        //To write memory
void writeMemory(int address, int value) {
    if (address < 10) {
        cacheLevel1[address] = value;
    } else {
        RAM[address] = value; }}
//--------------------
//Module 3: Instruction Set Architecture (ISA)
//instruction Code
#define ADD_OP 1
#define SUB_OP 2
#define LOAD_OP 3
#define STORE_OP 4
//execute based on operation code
void executeInstructionISA(int opcode) {
    switch(opcode) {
        case ADD_OP:
            ACC += 10;
            break;
        case SUB_OP:
            ACC -= 5;
            break;
        default:
            printf("Undefined opcode: %d\n", opcode);
            break;}}
//--------------------
//Module 4: Interrupt Handling
int interruptFlag = 0; // Flag for interrupts
void interruptHandler() {
//Saves CPU state and Handles the interrupt
    printf("Interrupt handled.\n");}
void checkForInterrupt() {
    if (interruptFlag) {
        interruptHandler();}}
//--------------------
//Module 5: Direct Memory Access (DMA)
//DMA-allows for data transfer between memory and I/O devices
void dmaTransfer(int* source, int* destination, int size) {
    for (int i = 0; i < size; i++) {
        destination[i] = source[i]; }}
//starts the DMA transfer
void initiateDMA(int* source, int* destination, int size) {
    dmaTransfer(source, destination, size);
    printf("DMA transfer initiated.\n");
}

