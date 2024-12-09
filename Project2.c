#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
//Main Memory(RAM)
#define RAM_SIZE 1024
//L1 Cache
#define L1_SIZE 64
//L2 Cache
#define L2_SIZE 128
//Max number of processes the system can have
#define MAX_PROCESSES 3
//total memory available
#define TOTAL_MEM 1024
//memory blocks available for us
#define NUM_BLOCKS 10
// Time slice for round-robin scheduling
#define TIME_SLICE 2
//variables for memory
//the int makes it global
int RAM[RAM_SIZE];
int L1Cache[L1_SIZE];
int L2Cache[L2_SIZE];
struct MemoryBlock {
int processID;
int memoryStart;
int memoryEnd;
//1 = Free, 0 = Allocated
int isFree; };
//example: divide ram into blocks
struct MemoryBlock memoryTable[NUM_BLOCKS];
//Module 1 - CPU Operations
//program counter
int PC = 0;
//Accumulator
int ACC = 0;
//instruction register
int IR = 0;
//zero flag
int ZF = 0;
//carry flag
int CF = 0;
//overflow flag
int OF = 0;
//fetches the next instructions
void fetch() {
IR = RAM[PC];}
//function decodes instructions in the instruction register
void decode() {}
//function to update the status flags
void updateStatusFlags(int result) {
ZF = (result == 0) ? 1 : 0;
CF = (result < 0) ? 1 : 0;
OF = (result > ACC) ? 1 : 0;}
void execute() {
int result;
switch (IR) {
case 1: //add
//adds the value in RAM to the accumulator
result = ACC + RAM[PC + 1];
ACC = result;
updateStatusFlags(result);
break;
case 2: //subtract
//subtracts the value in RAM at the address
result = ACC - RAM[PC + 1];
ACC = result;
updateStatusFlags(result);
break;
case 3: //multiply
//multiply the value in RAM at the address
result = ACC * RAM[PC + 1];
ACC = result;
updateStatusFlags(result);
break;
case 4: //load
//Loads the value from RAM at the address
ACC = RAM[PC + 1];
updateStatusFlags(ACC);
break;
case 5: //store
//stores the value in the accumulator
RAM[PC + 1] = ACC;
break;
case 6: //jump
PC = RAM[PC + 1];
return;
case 7: //divide
//divides the accumulator by the value in RAM
if (RAM[PC + 1] != 0) { // Check for division by zero
result = ACC / RAM[PC + 1];
ACC = result;
updateStatusFlags(result);
} else {
printf("Division by zero at PC = %d\n", PC);
exit(1);}
break;
case 8: //and
//bitwise AND operation
result = ACC & RAM[PC + 1];
ACC = result;
updateStatusFlags(result);
break;
case 9: //or
//bitwise OR operation
result = ACC | RAM[PC + 1];
ACC = result;
updateStatusFlags(result);
break;
case 10: //jump if zero
if (ZF) {
PC = RAM[PC + 1];
}
return;
case 0: //halt!
//Ends the program
exit(0);
default:
//Handles unknown instructions
printf("Unknown instruction at PC = %d\n", PC);
exit(1);}
PC += 2;}
void loadProgram() {
RAM[0] = 1; //add
RAM[1] = 5; //operand
RAM[2] = 7; //div
RAM[3] = 0; //operand
RAM[4] = 8; //and
RAM[5] = 3; //operand
RAM[6] = 9; //or
RAM[7] = 2; //operand
RAM[8] = 10; //jz
RAM[9] = 16; //jump
RAM[10] = 0; //halt
}
//Module 2: Memory system and OS Control Table
void initMemoryTable() {
for (int i = 0; i < NUM_BLOCKS; i++) {
memoryTable[i].processID = -1;
memoryTable[i].isFree = 1;}}
//first-fit allocation
void allocateMemory(int processID, int size) {
for (int i = 0; i < NUM_BLOCKS; i++) {
if (memoryTable[i].isFree && (memoryTable[i].memoryEnd -
memoryTable[i].memoryStart + 1) >= size) {
memoryTable[i].processID = processID;
memoryTable[i].isFree = 0; // Mark as allocated
printf("Allocated %d bytes to process %d in block %d\n", size,
processID, i);
return;}}
printf("Memory allocation process failed for process %d\n", processID);}
//finds the smallest memory block
void allocateBestFit(int processID, int size) {
int bestIdx = -1;
int bestCapacity = TOTAL_MEM;
for (int i = 0; i < NUM_BLOCKS; i++) {
if (memoryTable[i].isFree) {
int blockCapacity = memoryTable[i].memoryEnd -
memoryTable[i].memoryStart + 1;
//block is free than it is best found
if (blockCapacity >= size && blockCapacity < bestCapacity) {
bestCapacity = blockCapacity;
bestIdx = i;}}}
//if a suitable block has been found, assigns process id
if (bestIdx != -1) {
memoryTable[bestIdx].processID = processID;
memoryTable[bestIdx].isFree = 0; // Mark as allocated
printf("Best-fit allocated %d bytes to process %d in block %d\n", size,
processID, bestIdx);
} else {
printf("Best-fit failed for process %d\n", processID);}}
//Module 3 - process scheduling and multi-process handling
struct PCB {
int pid;
int pc;
int acc;
//Process state (0 = Ready, 1 = Running)
int state;};
struct PCB processTable[MAX_PROCESSES];
void initProcesses() {
for (int i = 0; i < MAX_PROCESSES; i++) {
processTable[i].pid = i + 1;
processTable[i].pc = 0;
processTable[i].acc = 0;
//Ready state
processTable[i].state = 0;}}
//Round-robin scheduler
//round-robin - "allocates each task an equal share of CPU time."
void scheduler() {
int currentProcess = 0;
while (1) {
//checks if its time to run
if (processTable[currentProcess].state == 0) {
processTable[currentProcess].state = 1; // Set to Running
printf("Process %d is running.\n", processTable[currentProcess].pid);
//makes the time slices for the round robin
sleep(TIME_SLICE);
processTable[currentProcess].state = 2;
printf("Process %d has finished running.\n",
processTable[currentProcess].pid);}
//moves to the next part of the process
currentProcess = (currentProcess + 1) % MAX_PROCESSES;
//Checks if all processes are done
int allFinished = 1;
for (int i = 0; i < MAX_PROCESSES; i++) {
if (processTable[i].state == 0) {
allFinished = 0;
break;}}
if (allFinished) {
printf("All processes have finished execution.\n");
break;}}}
void contextSwitch(int currentProcess, int nextProcess) {
//Saves current process state and load next process state
processTable[currentProcess].pc = PC;
processTable[currentProcess].acc = ACC;
//ready
processTable[currentProcess].state = 0;
PC = processTable[nextProcess].pc;
ACC = processTable[nextProcess].acc;
//running
processTable[nextProcess].state = 1;}
//Module 4: Interrupt Handling dispatcher
void (*IVT[3])();
//Handle timer interrupt, call dispatcher if needed
void timerInterrupt() {
printf("Timer interrupt occurred.\n");}
//Handle I/O interrupt
void ioInterrupt() {
printf("I/O interrupt occurred.\n");}
//Handle system call interrupt
void systemCallInterrupt() {
printf("System call interrupt occurred.\n");}
// Perform context switch
void dispatcher(int currentProcess, int nextProcess) {
contextSwitch(currentProcess, nextProcess);}
//Module 5 - Multithreading or Forking to handle the modules
void* cpuTask(void* arg) {
//handles CPU operations
while (1) {
fetch();
decode();
execute();}
return NULL;}
void* memoryTask(void* arg) {
//Handles memory management
return NULL;}
//Loads program into memory
int main() {
loadProgram();
//memory and processes
initMemoryTable();
initProcesses();
//memory for process
allocateBestFit(1, 128); // Example allocation
allocateMemory(2, 256); // Example first-fit allocation
//Setup interrupt handlers
IVT[0] = timerInterrupt;
IVT[1] = ioInterrupt;
IVT[2] = systemCallInterrupt;
//Creates threads for CPU and memory tasks
pthread_t thread1, thread2;
pthread_create(&thread1, NULL, cpuTask, NULL);
pthread_create(&thread2, NULL, memoryTask, NULL);
//Starts round-robin
scheduler();
IVT[0]();
pthread_join(thread1, NULL);
pthread_join(thread2, NULL);
return 0;}


