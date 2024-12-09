#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
//buffer size
#define BUFFER_SIZE 5
//thread count
#define NUM_THREADS 4
pthread_mutex_t memory_mutex;
pthread_mutex_t buffer_mutex;
sem_t buffer_full, buffer_empty;

//buffer to array
int buffer[BUFFER_SIZE];
//buffer tracker
int buffer_index = 0;
//amount of dedicated ram
#define RAM_SIZE 1024
//L1 and L2 Caches
#define L1_SIZE 64
#define L2_SIZE 128
//max proccesses
#define MAX_PROCESSES 3
//memory size
#define TOTAL_MEM 1024
//time slices
#define TIME_SLICE 2
//blocks
#define NUM_BLOCKS 10

//levels of memory
//array for ram and L1, L2 caches
int RAM[RAM_SIZE];
int L1Cache[L1_SIZE];
int L2Cache[L2_SIZE];

//memory blocks for allocation
struct MemoryBlock {
    //process id for block
    int processID;
    //start of the block
    int memoryStart;
    //end of the block
    int memoryEnd;
    // 1 = free, 0 = used
    int isFree;};
//memory table
struct MemoryBlock memoryTable[NUM_BLOCKS];

//cpu registers and flags
int PC = 0;
int ACC = 0;
int IR = 0;
int ZF = 0;
int CF = 0;
int OF = 0;

//process control block
//struct PCB {
//    int pid;
//    int pc;
//    int acc;
//    int state;};
// Represents the Process Control Block
struct PCB {
    int pid;             // Process ID
    int pc;              // Program counter
    int acc;             // Accumulator
    int state;           // Process state: 0 not started, 1 running, 2 finished
    int priority;        // Priority for scheduling
    int timeRemaining;   // Time left for completion
    int arrivalTime;     // Arrival time for scheduling
    int burstTime;       // Total execution time
};


struct PCB processTable[MAX_PROCESSES];

//interrupt handling
void (*IVT[3])();
pthread_mutex_t interrupt_mutex;

//time tracking
double cpu_execution_time = 0;
double memory_execution_time = 0;
double scheduling_execution_time = 0;
double interrupt_execution_time = 0;
double dma_execution_time = 0;
double efficiency_analysis_time = 0;

//stuff - fetch decode execute
void fetch();
void decode();
void execute();
void updateStatusFlags(int result);
void loadProgram();

//memory management
void initMemoryTable();
void allocateMemory(int processID, int size);
void allocateBestFit(int processID, int size);

//process initialization
void initProcesses();
void scheduler();
void contextSwitch(int currentProcess, int nextProcess);

//interrupt handling
void initMutex();
void handleInterrupt(int interruptType); //basic
void timerInterrupt(); //timer
void ioInterrupt(); // I/O
void systemCallInterrupt(); //system call
void dispatcher(int currentProcess, int nextProcess);

//DMA
void dmaTransfer(int* source, int* destination, int size);
void initiateDMA(int* source, int* destination, int size);
//efficiency analysis
void analyzeEfficiency();
//fetch decode execute
void* cpuTask(void* arg) {
    clock_t start_time = clock();
    while (1) {
        pthread_mutex_lock(&memory_mutex);
        fetch(); //fetche
        decode(); //decode
        execute(); //execute
        //checks if it is a halt instruction
        if (IR == 0) {
            pthread_mutex_unlock(&memory_mutex);
            break;}
        pthread_mutex_unlock(&memory_mutex);
        usleep(1000);}
    clock_t end_time = clock();
    cpu_execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    return NULL;}
//memory allocation and cache managing
void* memoryTask(void* arg) {
    // Records start time for time measurement
    clock_t start_time = clock();
    int iterations = 0;

    //allocates memory 3 times
    while (iterations < 3) {
        pthread_mutex_lock(&memory_mutex);
        allocateBestFit(1, 50);
        pthread_mutex_unlock(&memory_mutex);
        sleep(5);
        iterations++;}
    //ends time of execution
    clock_t end_time = clock();
    memory_execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    return NULL;}

//process scheduling logic
void* schedulingTask(void* arg) {
    clock_t start_time = clock();
    while (1) {
        scheduler();
        sleep(1); //for 1 second
        int allFinished = 1;
        //uses loops to check the process
        for (int i = 0; i < MAX_PROCESSES; i++) {
            pthread_mutex_lock(&memory_mutex);
            //if the process is not finished
            if (processTable[i].state != 2) {
                allFinished = 0;
                pthread_mutex_unlock(&memory_mutex);
                break;}
            pthread_mutex_unlock(&memory_mutex);}

        //if processes are done
        if (allFinished) {
            break;}}
    //execution time records
    clock_t end_time = clock();
    scheduling_execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    return NULL;}
//interrupt handling
void* interruptTask(void* arg) {
    clock_t start_time = clock();
    int count = 0;
    //interrupts one time and 2 more times
    while (count < 3) {
        sleep(3);
        handleInterrupt(0);
        handleInterrupt(1);
        handleInterrupt(2);
        count++;}
    //records execution end time
    clock_t end_time = clock();
    interrupt_execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    return NULL;}

//producer consumer
void* producer(void* arg) {
    while (1) {
        //checks if buffer is full
        sem_wait(&buffer_empty);
        //locks buffer Mutex
        pthread_mutex_lock(&buffer_mutex);
        int data = rand() % 100;
        buffer[buffer_index] = data;
        printf("Produced data: %d\n", data);
        buffer_index = (buffer_index + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_mutex);
        sem_post(&buffer_full);
        sleep(1);}return NULL;}

//Consumer function
void* consumer(void* arg) {
    while (1) {
        sem_wait(&buffer_full);
        pthread_mutex_lock(&buffer_mutex);
        buffer_index = (buffer_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;
        int data = buffer[buffer_index];
        //prints data
        printf("Consumed data: %d\n", data);
        pthread_mutex_unlock(&buffer_mutex);
        //if buffer isn't empty
        sem_post(&buffer_empty);
        sleep(1); } return NULL;}

//gets instruction from RAM
void fetch() {
    IR = RAM[PC];
    printf("Fetched instruction %d at PC=%d\n", IR, PC);}
//decoding
void decode() {}
//updates the status flags
void updateStatusFlags(int result) {
    ZF = (result == 0) ? 1 : 0;
    CF = (result < 0) ? 1 : 0;
    OF = (result > ACC) ? 1 : 0;}

//executes the instructions
void execute() {
    int result;
    //instruction switching IR
     switch (IR) {
        case 1: //add
            printf("Executing ADD instruction at PC=%d\n", PC); //print execution
            result = ACC + RAM[PC + 1]; //does operation
            ACC = result; //Store result
            updateStatusFlags(result); //updates flag
            break;

        case 2: //sub
            printf("Executing SUB instruction at PC=%d\n", PC);
            result = ACC - RAM[PC + 1];
            ACC = result;
            updateStatusFlags(result);
            break;

        case 3: //mul
            printf("Executing MUL instruction at PC=%d\n", PC);
            result = ACC * RAM[PC + 1];
            ACC = result;
            updateStatusFlags(result);
            break;

        case 4: //load
            printf("Executing LOAD instruction at PC=%d\n", PC);
            ACC = RAM[PC + 1];
            updateStatusFlags(ACC);
            break;

        case 5: //store
            printf("Executing STORE instruction at PC=%d\n", PC);
            RAM[PC + 1] = ACC;
            break;

        case 6: //jmp
            printf("Executing JMP instruction to PC=%d\n", RAM[PC + 1]);
            PC = RAM[PC + 1];
            return;

        case 7: //div
            printf("Executing DIV instruction at PC=%d\n", PC);

            //checks if dividing by zer0
            if (RAM[PC + 1] != 0) {
                result = ACC / RAM[PC + 1];
                ACC = result;
                updateStatusFlags(result); } else {
                printf("Division by zero at PC = %d\n", PC);
                pthread_mutex_unlock(&memory_mutex);
                pthread_exit(NULL); }break;

        case 8: //and
            printf("Executing AND instruction at PC=%d\n", PC);
            result = ACC & RAM[PC + 1];
            ACC = result;
            updateStatusFlags(result);
            break;

        case 9: //or
            printf("Executing OR instruction at PC=%d\n", PC);
            result = ACC | RAM[PC + 1];
            ACC = result;
            updateStatusFlags(result);
            break;

        case 10: //jz
            printf("Executing JZ instruction to PC=%d if ZF=%d\n", RAM[PC + 1], ZF);
            if (ZF) {
                PC = RAM[PC + 1];
                return; }break;
        //halt
        case 0:
            //halt program!
            printf("Program halted.\n");
            //unlocks memory
            pthread_mutex_unlock(&memory_mutex);
            //exit
            pthread_exit(NULL);

        //unknown instructions
        default:
            printf("Unknown instruction at PC = %d\n", PC);
            pthread_mutex_unlock(&memory_mutex);
            pthread_exit(NULL);}
    PC += 2;}

//program instructions to RAM
void loadProgram() {
    RAM[0] = 1; //add
    RAM[1] = 5; //operand
    RAM[2] = 7; //div
    RAM[3] = 1; //operand
    RAM[4] = 8; //and
    RAM[5] = 3; //operand
    RAM[6] = 9; //or
    RAM[7] = 2; //operand
    RAM[8] = 10; //jz
    RAM[9] = 0; //Jump target
    RAM[10] = 0;} //halt!

//makes table with blocks
void initMemoryTable() {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        memoryTable[i].processID = -1;
        //start blocks
        memoryTable[i].memoryStart = i * (TOTAL_MEM / NUM_BLOCKS);
        //end blocks
        memoryTable[i].memoryEnd = (i + 1) * (TOTAL_MEM / NUM_BLOCKS) - 1;
        //marks blocks free
        memoryTable[i].isFree = 1;}}

//allocates memory to a process
void allocateMemory(int processID, int size) {
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (memoryTable[i].isFree && (memoryTable[i].memoryEnd - memoryTable[i].memoryStart + 1) >= size) {
            //assigns block to the process
            memoryTable[i].processID = processID;
            //checks off block as allocated
            memoryTable[i].isFree = 0;
            printf("Allocated %d bytes to process %d in block %d\n", size, processID, i);
            return;}}
    printf("Memory allocation failed for process %d\n", processID);}

//best fit allocation
void allocateBestFit(int processID, int size) {
    //start point of -1
    int bestIdx = -1;
    int bestCapacity = TOTAL_MEM;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (memoryTable[i].isFree) {
            int blockCapacity = memoryTable[i].memoryEnd - memoryTable[i].memoryStart + 1;
            if (blockCapacity >= size && blockCapacity < bestCapacity) {
                bestCapacity = blockCapacity;
                bestIdx = i;}}}

    if (bestIdx != -1) {
        memoryTable[bestIdx].processID = processID;
        memoryTable[bestIdx].isFree = 0;
        printf("Best-fit allocated %d bytes to process %d in block %d\n", size, processID, bestIdx);
    } else {printf("Best-fit allocation failed for process %d\n", processID); }}

//starts the process
void initProcesses() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processTable[i].pid = i + 1;
        processTable[i].pc = 0;
        processTable[i].acc = 0;
        processTable[i].state = 0;}}

//scheduler function
void scheduler() {
    static int currentProcess = 0;
    pthread_mutex_lock(&memory_mutex);
    if (processTable[currentProcess].state == 0 || processTable [currentProcess].state == 1) {
        processTable[currentProcess].state = 1;
        printf("Process %d is running.\n", processTable[currentProcess].pid);
        processTable[currentProcess].pc += 1;
        if (processTable[currentProcess].pc >= 5) {
            processTable[currentProcess].state = 2;
            printf("Process %d has finished execution.\n", processTable[currentProcess].pid);}}
    //unlocks memory
    pthread_mutex_unlock(&memory_mutex);
    currentProcess = (currentProcess + 1) % MAX_PROCESSES;}

//does context switching
void contextSwitch(int currentProcess, int nextProcess) {
    // Saves the state of the current process before switching
    processTable[currentProcess].pc = PC;
    processTable[currentProcess].acc = ACC;
    processTable[currentProcess].state = 0;

    //loads the state of the next process to switch into
    PC = processTable[nextProcess].pc;
    ACC = processTable[nextProcess].acc;
    processTable[nextProcess].state = 1;}

//Mutax start
void initMutex() {
    pthread_mutex_init(&interrupt_mutex, NULL);}

//handles interrupt depending which one is raised
void handleInterrupt(int interruptType) {
    pthread_mutex_lock(&interrupt_mutex);
    //checks if the interrupt is valid
    if (interruptType >= 0 && interruptType < 3) {
        IVT[interruptType]();}
    pthread_mutex_unlock(&interrupt_mutex);}
void timerInterrupt() {
    printf("Timer interrupt handled.\n");}
//interrupt handler for I/O interrupts
void ioInterrupt() {printf("I/O interrupt handled.\n");}


void systemCallInterrupt() {
    printf("System call interrupt handled.\n");}

//dispatches processes through context switching
void dispatcher(int currentProcess, int nextProcess) {
    //calls the context switch to switch the processes
    contextSwitch(currentProcess, nextProcess);}

//DMA transfer function
void dmaTransfer(int* source, int* destination, int size) {
    for (int i = 0; i < size; i++) {
        destination[i] = source[i];}}

//initiate DMA Transfer
void initiateDMA(int* source, int* destination, int size) {
    //records start time for DMA transfer
    clock_t start_time = clock();
    //calls the DMA transfer function
    dmaTransfer(source, destination, size);
    //records end time for DMA transfer
    clock_t end_time = clock();
    dma_execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;}

//efficiency analysis function
void analyzeEfficiency() {
    clock_t start_time = clock();

    printf("CPU Execution Time: %f seconds\n", cpu_execution_time);
    printf("Memory Management Execution Time: %f seconds\n", memory_execution_time);
    printf("Scheduling Execution Time: %f seconds\n", scheduling_execution_time);
    printf("Interrupt Handling Execution Time: %f seconds\n", interrupt_execution_time);
    printf("DMA Execution Time: %f seconds\n", dma_execution_time);
    //records end time for efficiency analysis
    clock_t end_time = clock();
    //calculates execution time
    efficiency_analysis_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;}

int main() {
    printf("==========================\n");
    printf("SYSTEM INITIALIZATION\n");
    printf("==========================\n");
    printf("System initialized: - - - - \n");
    printf("L1 Cache: %d entries\n", L1_SIZE);
    printf("L2 Cache: %d entries\n", L2_SIZE);
    printf("RAM: %d entries\n", RAM_SIZE);
    printf("Cores and Scheduler started successfully\n");
    printf("==========================\n");

    //load into memory
    loadProgram();
    //memory table
    initMemoryTable();
    //process table
    initProcesses();

    //initializes mutex and semaphores
    pthread_mutex_init(&memory_mutex, NULL);
    pthread_mutex_init(&buffer_mutex, NULL);
 sem_init(&buffer_full, 0, 0);
    sem_init(&buffer_empty, 0, BUFFER_SIZE);

    initMutex(); //interrupt mutex
    IVT[0] = timerInterrupt;//timer interrupt
    IVT[1] = ioInterrupt; //I/O interrupt
    IVT[2] = systemCallInterrupt; //system call

    //creates threads
    pthread_t threads[NUM_THREADS];
    pthread_create(&threads[0], NULL, cpuTask, NULL);//CPU task
    pthread_create(&threads[1], NULL, memoryTask, NULL);//memory management
    pthread_create(&threads[2], NULL, schedulingTask, NULL);//scheduling
    pthread_create(&threads[3], NULL, interruptTask, NULL); //interrupt handling

    //creates producer and consumer threads
    pthread_t producer_thread, consumer_thread;
    //thread producer
    pthread_create(&producer_thread, NULL, producer, NULL);
    //thread consumer
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    sleep(20);

    //cancels producer and consumer thread
    pthread_cancel(producer_thread);
    pthread_cancel(consumer_thread);
    //waits for producer and consumer to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    //waits for all the threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);}

    //does DMA transfer example
    int source[5] = {1, 2, 3, 4, 5};
    int destination[5];
    //initiates DMA transfer
    initiateDMA(source, destination, 5);

    //analyze efficiency
    //calls the efficiency analysis function
    analyzeEfficiency();

    //Mutex and Semaphore clean up
    pthread_mutex_destroy(&memory_mutex);
    pthread_mutex_destroy(&buffer_mutex);
    sem_destroy(&buffer_full);
    sem_destroy(&buffer_empty);
    pthread_mutex_destroy(&interrupt_mutex);

    //print execution times
    printf("\n--- Execution Times ---\n");
    printf("CPU Task Execution Time: %.6f seconds\n", cpu_execution_time);
    printf("Memory Management Task Execution Time: %.6f seconds\n", memory_execution_time);
    printf("Scheduling Task Execution Time: %.6f seconds\n", scheduling_execution_time);
    printf("Interrupt Handling Task Execution Time: %.6f seconds\n", interrupt_execution_time);
    printf("DMA Execution Time: %.6f seconds\n", dma_execution_time);
    printf("Efficiency Analysis Time: %.6f seconds\n", efficiency_analysis_time);

    return 0;
    }


//project 4 stuff(it fucking sucks)


struct MemoryBlock memoryTable[NUM_BLOCKS];





//function stuff
void enqueue(int pid);
int dequeue();
int getHighestPriority();
int getShortestRemainingTime();
void scheduler(const char *algorithm);
void loadProcessInstructions(int pid, int startAddress, int *instructions, int size);

//global variables(they work everywhere in the file)
int readyQueue[MAX_PROCESSES];
int queueFront = 0, queueRear = 0;
int activeProcesses = 0;

//queue
void enqueue(int pid) {
    readyQueue[queueRear] = pid;
    queueRear = (queueRear + 1) % MAX_PROCESSES;}

int dequeue() {
    int pid = readyQueue[queueFront];
    queueFront = (queueFront + 1) % MAX_PROCESSES;
    return pid;}

//helps with highest prioity
int getHighestPriority() {
    int highestPriority = 10; //0-9
    int selectedProcess = -1;
    for (int i = 0; i < activeProcesses; i++) {
        int pid = readyQueue[(queueFront + i) % MAX_PROCESSES];
        if (processTable[pid].state == 0 && processTable[pid].priority < highestPriority) {
            highestPriority = processTable[pid].priority;
            selectedProcess = pid;
        }}return selectedProcess;}

//get process with shortest remaining time
int getShortestRemainingTime() {
    int shortestTime = 100000; //big number = big time
    int selectedProcess = -1;
    for (int i = 0; i < activeProcesses; i++) {
        int pid = readyQueue[(queueFront + i) % MAX_PROCESSES];
        if (processTable[pid].state == 0 && processTable[pid].timeRemaining < shortestTime) {
            shortestTime = processTable[pid].timeRemaining;
            selectedProcess = pid;
        }}return selectedProcess;}

        //with enough love this might work

// Scheduler
//void scheduler(const char *algorithm) {
 //   while (activeProcesses > 0) {
  //      int pid = -1;

    //    if (strcmp(algorithm, "Priority") == 0) {
      //      pid = getHighestPriority();
       // } else if (strcmp(algorithm, "SRT") == 0) {
   //         pid = getShortestRemainingTime();
    //    } else if (strcmp(algorithm, "RoundRobin") == 0) {
     //       pid = dequeue();
      //  }

       // if (pid == -1) continue; // No process selected

//        struct PCB *process = &processTable[pid];
//        if (process->state == 0) {
 //           process->state = 1; // Running
  //          printf("Executing process %d (%s)...\n", process->pid, algorithm);

    //        int timeToRun = (strcmp(algorithm, "RoundRobin") == 0) ?
      //          (process->timeRemaining < TIME_SLICE ? process->timeRemaining : TIME_SLICE) :
//                process->timeRemaining;

//            for (int i = 0; i < timeToRun; i++) {
//                int instruction = RAM[process->pc];
 //               switch (instruction) {
   //                 case 1: // ADD
     //                   process->acc += RAM[process->pc + 1];
       //                 process->pc += 2;
         //               break;
           //         case 2: // SUB
               //         process->acc -= RAM[process->pc + 1];
             //           process->pc += 2;
                   //     break;
                 //   case 0: // HALT
                     //   process->state = 2; // Blocked
                      //  break;
                   // default:
                    //    printf("Invalid instruction at PC: %d\n", process->pc);
                    //    process->state = 2;
            //            break;
              //  }
               // process->timeRemaining--;
              //  if (process->timeRemaining <= 0 || process->state == 2) break;
            //    usleep(100000); // Simulate execution delay
            //}

            //if (process->timeRemaining > 0 && process->state != 2) {
              //  process->state = 0; // Ready
               // if (strcmp(algorithm, "RoundRobin") == 0) enqueue(pid);
            //} else if (process->state == 2) {
              //  printf("Process %d completed with ACC = %d\n", process->pid, process->acc);
                //activeProcesses--;
            //}}}}

//loads instructions to RAM
void loadProcessInstructions(int pid, int startAddress, int *instructions, int size) {
    processTable[pid].pid = pid;
    processTable[pid].pc = startAddress;
    processTable[pid].state = 0; //its ready
    processTable[pid].priority = rand() % 5; //Random priority
    processTable[pid].timeRemaining = size;
    processTable[pid].arrivalTime = pid; //Simp arrival time
    processTable[pid].burstTime = size;

    for (int i = 0; i < size; i++) {
        RAM[startAddress + i] = instructions[i];}}

//starter
void initializeProcesses() {
    int instructions1[] = {1, 5, 2, 3, 0};
    int instructions2[] = {1, 10, 2, 2, 0};
    int instructions3[] = {2, 7, 1, 3, 0};

    loadProcessInstructions(0, 0x100, instructions1, 5);
    loadProcessInstructions(1, 0x200, instructions2, 5);
    loadProcessInstructions(2, 0x300, instructions3, 5);

    activeProcesses = MAX_PROCESSES;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        enqueue(i);}}



