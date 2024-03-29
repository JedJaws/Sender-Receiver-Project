#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include "msg.h"    /* For the message struct */

using namespace std;

/* The size of the shared memory segment */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr = NULL;


/**
 * The function for receiving the name of the file
 * @return - the name of the file received from the sender
 */
string recvFileName()
{
	/* The file name received from the sender */
	string fileName;
        
	/* TODO: declare an instance of the fileNameMsg struct to be
	 * used for holding the message received from the sender.

	 Done
         */
	fileNameMsg fileObj;

        /* TODO: Receive the file name using msgrcv()
			Done
		 */
	msgrcv(msqid, &fileObj, sizeof(fileNameMsg) - sizeof(long), FILE_NAME_TRANSFER_TYPE, 0);
	/* TODO: return the received file name */
	fileName = fileObj.fileName;
    return fileName;
}
 /**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */
void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	
	/* TODO: 
        1. Create a file called keyfile.txt containing string "Hello world" (you may do
 	    so manually or from the code).
	2. Use ftok("keyfile.txt", 'a') in order to generate the key.
	3. Use will use this key in the TODO's below. Use the same key for the queue
	   and the shared memory segment. This also serves to illustrate the difference
 	   between the key and the id used in message queues and shared memory. The key is
	   like the file name and the id is like the file object.  Every System V object 
	   on the system has a unique id, but different objects may have the same key.
	*/
	
	key_t key = ftok("keyfile.txt", 'a');

	/* TODO: Allocate a shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
	if((shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, S_IRUSR | S_IWUSR | IPC_CREAT)) >= 0) {
		std::cout << "Allocated shared memory segment!" << std::endl;
	} else {
		std::cout << "Error allocating memory segment: " << errno << std::endl;
		exit(-1);
	}
	/* TODO: Attach to the shared memory */
	sharedMemPtr = (char*)shmat(shmid, NULL, 0);
	/* TODO: Create a message queue */
	msqid = msgget(key, S_IRUSR | S_IWUSR | IPC_CREAT);
	/* TODO: Store the IDs and the pointer to the shared memory region in the corresponding parameters */
}
 

/**
 * The main loop
 * @param fileName - the name of the file received from the sender.
 * @return - the number of bytes received
 */
unsigned long mainLoop(const char* fileName)
{
	/* The size of the message received from the sender */
	int msgSize = -1;
	
	/* The number of bytes received */
	int numBytesRecv = 0;
	
	/* The string representing the file name received from the sender */
	string recvFileNameStr = fileName;
	
	/* TODO: append __recv to the end of file name */
	recvFileNameStr += "__recv";

	/* Open the file for writing */
	FILE* fp = fopen(recvFileNameStr.c_str(), "w");
			
	/* Error checks */
	if(!fp)
	{
		perror("fopen");	
		exit(-1);
	}
	message msg;
	message recMsg;	

	/* Keep receiving until the sender sets the size to 0, indicating that
 	 * there is no more data to send.
 	 */	
	while(msgSize != 0)
	{	

		/* TODO: Receive the message and get the value of the size field. The message will be of 
		 * of type SENDER_DATA_TYPE. That is, a message that is an instance of the message struct with 
		 * mtype field set to SENDER_DATA_TYPE (the macro SENDER_DATA_TYPE is defined in 
		 * msg.h).  If the size field of the message is not 0, then we copy that many bytes from 
		 * the shared memory segment to the file. Otherwise, if 0, then we close the file 
		 * and exit.
		 *
		 * NOTE: the received file will always be saved into the file called
		 * <ORIGINAL FILENAME__recv>. For example, if the name of the original
		 * file is song.mp3, the name of the received file is going to be song.mp3__recv.
		 */
		
		/* If the sender is not telling us that we are done, then get to work */
		if((msgSize = msgrcv(msqid, &msg, SHARED_MEMORY_CHUNK_SIZE, SENDER_DATA_TYPE, 0)) >= 0) {
			std::cout << "Chunk size is: " << msgSize << std::endl;	
		} else {
			std::cout << "Error receiving chunk size: " << errno << std::endl;
			exit(-1);
		}

		if(msgSize != 0)
		{
			/* TODO: count the number of bytes received */
			numBytesRecv += msgSize;
			/* Save the shared memory to file */
			if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0)
			{
				perror("fwrite");
				exit(-1);
			}
			
			/* TODO: Tell the sender that we are ready for the next set of bytes. 
 			 * I.e., send a message of type RECV_DONE_TYPE. That is, a message
			 * of type ackMessage with mtype field set to RECV_DONE_TYPE. 
 			 */
			ackMessage.mtype = RECV_DONE_TYPE;
			ackMessage.size = 0;
			if((msgsnd(msqid, &ackMessage, 0, 0) >= 0)) {
				std::cout << "Ready for next set of bytes!" << std::endl;
			} else {
				std::cout << "Error with readying for next set: " << errno << std::endl;
				exit(-1);
			}
		}
		/* We are done */
		else
		{
			/* Close the file */
			fclose(fp);
		}
	}
	return numBytesRecv;
}



/**
 * Performs cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	if(shmdt(sharedMemPtr) >= 0) {
		std::cout << "Pointer detached from shared memory!" << std::endl;
	} else {
		std::cout << "Error attempting to detach shared memory:" << errno << std::endl;
		exit(-1);
	}
	/* TODO: Deallocate the shared memory segment */
	if(shmctl(shmid, IPC_RMID, NULL) >= 0) {
		std::cout << "Removing shmid and destroying shared memory!" << std::endl;
	} else {
		std::cout << "Error attempting to destroy shared memory: " << errno << std::endl;
		exit(-1);
	}
	/* TODO: Deallocate the message queue */
	if(msgctl(msqid, IPC_RMID, NULL) >= 0) {
		std::cout << "Removing the message queue!" << std::endl;
	} else {
		std::cout << "Error attempting to remove the message queue: " << errno << std::endl;
		exit(-1);
	}
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */
void ctrlCSignal(int signal)
{
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv)
{
	
	/* TODO: Install a signal handler (see signaldemo.cpp sample file).
 	 * If user presses Ctrl-c, your program should delete the message
 	 * queue and the shared memory segment before exiting. You may add 
	 * the cleaning functionality in ctrlCSignal().
 	 */
	signal(SIGINT, ctrlCSignal);
				
	/* Initialize */
	init(shmid, msqid, sharedMemPtr);
	
	/* Receive the file name from the sender */
	string fileName = recvFileName();
	
	/* Go to the main loop */
	fprintf(stderr, "The number of bytes received is: %lu\n", mainLoop(fileName.c_str()));

	/* TODO: Detach from shared memory segment, and deallocate shared memory 
	 * and message queue (i.e. call cleanup) 

	 	Done
	 */
	cleanUp(shmid, msqid, sharedMemPtr);
	return 0;
}
