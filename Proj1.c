#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//Program forward declerations

void LoadValue(int* AC, int val);
void GetRandNum(int* AC);
void PutToScreen(int AC, int port);
void AddX(int X, int* AC);
void AddY(int Y, int* AC);
void CopyToX(int AC, int* X);
void CopyToY(int AC, int* Y);



struct MemoryRequest{
	int io; //0 is read, 1 is write
	int addr; //Addr to read or write
	int data; //data to send if its a write
};

int CPUReadMem(int readfd, int writefd, int* data, struct MemoryRequest mr, int mode);


int main(int argc, char *argv[]){
	srand(time(NULL));	
	char* inputFile;
	int interruptTime = 0;

	if(argc == 3){
		inputFile = argv[1];
		interruptTime = atoi(argv[2]);
	}
	else{
		printf("Too few arguments.\n");
		return 1;
	}


	//fd[0] for read    fd[1] for write
	int fd1[2]; //pipe 1 parent writes, child reads
	int fd2[2]; //pipe 2 child writes, parent reads

	if(pipe(fd1) == -1){
		printf("Error creating pipe.");
		return 1;
	}
	if(pipe(fd2) == -1){
		printf("Error creating pipe.");
		return 1;
	}
	
	pid_t pid;
	pid = fork();

	if(pid > 0){ //Parent- CPU
		int mode = 0; //0 is user, 1 is system
		int timer = 0;
		int running = 1;
		int currentlyInterrupted = 0;
		int timerInterrupt = 0;
		int PC = 0, SP = 1000, IR = 0, AC = 0, X = 0, Y = 0;
		

		//Close write end of pipe 2 since Ill never be writing on that pipe from this process- only reading.
		close(fd2[1]);
		//Close read end of pipe 1 since Ill never be reading on that pipe from this process- only writing.
		close(fd1[0]);

		//switch on IR for what to execute
		while(running == 1){
			struct MemoryRequest mr;

			if(timer > interruptTime && currentlyInterrupted == 0){
				mode = 1; //Set system mode
				currentlyInterrupted = 1;
				timerInterrupt = 1;
				int oldSP = SP;
				SP = 1999; //Switch to new stack
				mr.io = 1;
				mr.addr = SP;
				mr.data = oldSP;
				write(fd1[1], &mr, sizeof(struct MemoryRequest)); //Write SP to new stack

				SP -= 1;
				mr.addr = SP;
				mr.data = PC - 1;
				write(fd1[1], &mr, sizeof(struct MemoryRequest)); //Write PC addr to new stack
				//printf("Interrupt");	
				PC = 1000; //Timer interupt cause execution at 1000

			}



			mr.io = 0;
			mr.addr = PC;

			//Read instruction into IR
			CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);
			//printf("\nIR %i\n", IR);
			//printf("\nPC %i\n-----------------\n", PC);
			//printf("%i\n", mode);
			
			switch(IR){
				case 1:
					PC++; //Increase program counter to get the put method which comes next in the instructions
					mr.io = 0;
					mr.addr = PC;
					
				
				
					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);
					
					LoadValue(&AC, IR);
					break;
				case 2:
					PC++;
					mr.io = 0;
					mr.addr = PC;

					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);

					mr.addr = IR;

					CPUReadMem(fd2[0], fd1[1], &AC, mr, mode);


					break;
				case 3:
					PC++;
					mr.io = 0;
					mr.addr = PC;

					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode); //Read address value
					
					mr.addr = IR;

					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode); //Read value at value addr

					mr.addr = IR;

					CPUReadMem(fd2[0], fd1[1], &AC, mr, mode); //Read value from the value addr

					break;
                                case 4:
					PC++;
					mr.io = 0;
					mr.addr = PC;

					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);

					mr.addr = IR + X;
					

					CPUReadMem(fd2[0], fd1[1], &AC, mr, mode);

					//printf("%i", AC);
				        break;
				case 5:
					PC++;
					mr.io = 0;
					mr.addr = PC;
					
					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);

					mr.addr = IR + Y;

					CPUReadMem(fd2[0], fd1[1], &AC, mr, mode);
					break;
				case 6:
					mr.io = 0;
					mr.addr = SP + X;
					CPUReadMem(fd2[0], fd1[1], &AC, mr, mode);
					break;
				case 7:
					PC++;
					mr.io = 0;
					mr.addr = PC;

					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);
		
					if(mode != 1 && IR > 999){
						mr.io = -1;
						write(fd1[1], &mr, sizeof(struct MemoryRequest));
						running = 0;
						break;
						//_exit(0);
					}

					
					mr.io = 1;
					mr.addr = IR;
					mr.data = AC;
					//printf("AC %i\n", AC);
					write(fd1[1], &mr, sizeof(struct MemoryRequest));
					break;
				case 8:
					GetRandNum(&AC);
					break;
				case 9:
					//Logic for gettign the next instruction here
					PC++; //Increase program counter to get the put method which comes next in the instructions
					mr.io = 0;
					mr.addr = PC;
					
						
					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);
					//printf("%i", AC);	
					PutToScreen(AC, IR);
					break;
				case 10:
					AddX(X, &AC);
					break;
				case 11:
					AddY(Y, &AC);
					break;
				case 12:
					AC = AC - X;
					break;
				case 13:
					AC = AC - Y;
					break;
				case 14:
					CopyToX(AC, &X);;
					break;
				case 15:
					AC = X;
					break;
				case 16:
					CopyToY(AC, &Y);
					break;
				case 17:
					AC = Y;
					break;
				case 18:
					SP = AC;
					break;
				case 19:
					AC = SP;
					break;
				case 20:
					PC++;
					mr.io = 0;
					mr.addr = PC;
					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);

					PC = IR-1;
					break;
				case 21:
					if(AC == 0){
						PC++; //Get next mem instruction to see where to jump to.
						mr.io = 0;
						mr.addr = PC;
						CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);

						PC = IR-1; //-1 since PC will get incremented at the end of this
					}
					else{
						PC++;
					}
					break;
				case 22:
					if(AC != 0){
						PC++; //Get next mem instruction to see where to jump to.
						mr.io = 0;
						mr.addr = PC;
						CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);

						PC = IR-1; //-1 since PC will get incremented at the end of this

					}
					else{
						PC++;
					}
					break;
				case 23:
					//Load the current PC into the stack- this acts as the return address for the function call
					PC++; //Increase pc in order to find the mem addr to jump to
					int oldPC = PC;		
					mr.io = 0;
					mr.addr = PC;


					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);

					PC = IR-1; //Set to IR since that the new location we are jumping to for the function Subtract 1 since the PC will increment by 1 after the switch statement ends
						
					SP -= 1; //Move stack pointer 1 down
					
					if(mode != 1 && SP > 999){
						mr.io = -1;
						write(fd1[1], &mr, sizeof(struct MemoryRequest));
						running = 0;
						break;
						
					}

					mr.io = 1; //Write operation
					mr.addr = SP; //Write to the current stack pointer.
					mr.data = oldPC;
					write(fd1[1], &mr, sizeof(struct MemoryRequest)); //Write return addr to stack
					
					
					break;
				case 24:
					mr.io = 0;
					mr.addr = SP; //Get last addr in stack
					CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);
					
					SP++;
					PC = IR; //Subract 2 since PC will increment by 1 after the switch
					
					break;
				case 25:
					X++;
					break;
				case 26:
					X--;
					break;
				case 27:
					SP -= 1; //Move stack pointer down 1
					mr.io = 1; //Set up for write
					mr.addr = SP;
					mr.data = AC;
					if(mode != 1 && mr.addr > 999){
						running = 0;
						break;
					}
					write(fd1[1], &mr, sizeof(struct MemoryRequest)); //Write AC to stack
					break;
				case 28:
					//Dont have to add to the SP or anything since its already at the spot where the last push was
					mr.io = 0;
					mr.addr = SP;

					CPUReadMem(fd2[0], fd1[1], &AC, mr, mode);
					SP++; //add 1 to stack to set it at the prev mem addr
					break;
				case 29:
					if(currentlyInterrupted == 0){
						//printf("Current PC: %i\n", PC);
						mode = 1; //Set to system mode
						currentlyInterrupted = 1;
						int oldSP = SP;
						SP = 1999;
						
						mr.io = 1;
						mr.addr = SP;
						mr.data = oldSP;
						write(fd1[1], &mr, sizeof(struct MemoryRequest)); //Write SP to new stack
						
						SP -= 1;
						mr.addr = SP;
						mr.data = PC;
						write(fd1[1], &mr, sizeof(struct MemoryRequest)); //Write PC addr to new stack

						PC = 1499; //System call interupt cause execution at 1500

					}
					break;
				case 30:
					if(currentlyInterrupted == 1){
						//Get PC and then get SP
						mr.io = 0;
						mr.addr = SP;
						CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);
						PC = IR; //Get last PC - 1 since it will be incremented at the end of the loop
	
						SP++;
						mr.addr = SP;
						CPUReadMem(fd2[0], fd1[1], &IR, mr, mode);
						SP = IR;
						
						currentlyInterrupted = 0;
						
						if(timerInterrupt == 1){
							timer = 0;
						}
						timerInterrupt = 0;
						mode = 0; //Set to user mode

					}
					break;


				case 50:
					mr.io = -1;
					write(fd1[1], &mr, sizeof(struct MemoryRequest));
					running = 0;
					break;
				default:
					mr.io = -1;
					write(fd1[1], &mr, sizeof(struct MemoryRequest));
					printf("%i is not a valid instruction.\n", IR);
					running = 0;
					break;
			}

			PC += 1; //Increment program counter after every instruction cycle
			timer += 1; //Increment timer
		}


		close(fd1[1]); //Close write
		close(fd2[0]); //Close read

	}
	else if(pid == 0) { //Child- Main Memory

		int memory[2000]; //0-999 user program, 1000-1999 System code. Programs get loaded into memory starting from 0. Stack is in the memory and starts at 999 and 1999 and
				  //grows to 0.
		
		//Close write end of pipe 1 since child isnt writing to that pipe ever.
		close(fd1[1]);
		//Close read end of pipe 2 since child isnt reading from pipe 2 ever.
		close(fd2[0]);
		
		//Read input file into memory
		FILE* fptr;
	        fptr = fopen(inputFile, "r");
	        
		if(fptr == NULL){
	                printf("Failed to open file");
	                return 1;
		}

		char str[100];
		int i = 0;
		while(fgets(str, 100, fptr) != NULL){
			if(str[0] != '\n' && (isdigit(str[0]) || str[0] == '.')){
				
				if(str[0] == '.'){
					char* tok1 = strtok(str, ".");
					//printf("%s\n", tok1);
					//char* tok2 = strtok(str, " ");
					//printf("tok2 %s\n", tok2);
					i = atoi(tok1);
					//printf("I: %i\n", i);
					continue;
				}

				//fprintf(stderr, "%c\n", str[0]);
				char* token = strtok(str, " ");
				
				//if(atoi(token) == 0){
				//	continue;
				//}

				memory[i] = atoi(token);		
				i++;
			}
			
		}

		

		fclose(fptr);
		
		while(1){ //Do mem read writes here i guess
			
			struct MemoryRequest mr;
			read(fd1[0], &mr, sizeof(struct MemoryRequest)); //Read next memory request
			
			if(mr.io == 0) { //Read request
				write(fd2[1], &memory[mr.addr], sizeof(int));		
			}
			else if(mr.io == 1){ //Write request
				memory[mr.addr] = mr.data;
			}
			else if(mr.io == -1){
				//_exit(0);
				break;
			}
		}



		close(fd1[0]); //Close read 
		close(fd2[1]); //Close write
	}
	else{
		printf("Error forking.");
		return 1;
	}
	
	
	return 0;
}



//CPU functions
void LoadValue(int* AC, int val){ //1- Load value val into the AC
	*AC = val;
}

void GetRandNum(int* AC){ //8- Get a random int from 1 to 100 and put into AC
	*AC = rand() % 100 + 1;
	//printf("%i", *AC);
}

void PutToScreen(int AC, int port){ //9- Writes to screen if 1 then as num, if 2 then as char
	if(port == 1){
		printf("%i", AC);
	}
	else if(port == 2){
		printf("%c", AC);
	}
	else{
		printf("Incorrect value for put\n");
	}
}

void AddX(int X, int* AC){ //10- Adds the value of X to the AC
	*AC += X;
}

void AddY(int Y, int* AC){ //11- Adds the value of Y to the AC
	*AC += Y;
}

void CopyToX(int AC, int* X){ //14- Copy AC value to register X
	*X = AC;
}

void CopyToY(int AC, int* Y){ //16- Copy AC to register Y
	*Y = AC;
}

//Mem functions

int CPUReadMem(int readfd, int writefd, int* data, struct MemoryRequest mr, int mode){

	if(mr.addr > 999 && mode != 1){
		printf("Illegal memory acess.\n");
		mr.io = -1;
		write(writefd, &mr, sizeof(struct MemoryRequest));
		close(readfd);
		close(writefd);
		_exit(0);
	}
	write(writefd, &mr, sizeof(struct MemoryRequest)); //Write pc to mem

	read(readfd, &*data, sizeof(int)); //Read instruction at PC in mem
	

}
