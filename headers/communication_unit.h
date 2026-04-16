#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "serial_communication.h"
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <stdlib.h>

// Start of heading: start of metadata transmission
#define SOH 0x01
// Start of text: end of metadata transmission and start of data
#define STX 0x02 
// End of text: end of the data
#define ETX 0x03
// End of trasmission: the process that control the terminal was terminated
#define EOT 0x04
// Acknowledge: acknowledge that the message sent by the terminal was received
#define ACK 0x06
// Substitute: substitute character to fill the msg when the size is less than MSG_SIZE
#define SUB 0x1A

#define PORT_NAME "/dev/ttyUSB0"
#define BAUD_RATE 115200
#define MAX_PROCESS_COUNT 10

// Timeout in ms for acknowledge
#define ACK_TIMEOUT 1000

#define TERMINAL_BUFFER_SIZE 1024
#define RECEIVE_BUFFER_SIZE 1024

// Amount of bytes the terminals send at time to the microcontroller
#define MSG_SIZE 32

/**
* @brief Prepares the communication unit
*
* Initializes the port and the mutex
*
* @return 0 if the initialization is sucessful, -1 if it is not
*/
int init_communication_unit();

/**
* @brief Sends data to the microcontroller
*
* Given an array of data, sends it to the
* microcontroller
*
* @param data Pointer to the data that will be sent
* @param pid PID associated with this terminal
*
* @return 0 if data is sent sucessfully, -1 if it is not and -2 for ack timeout
*/
int send_data(char *data, int pid);

int add_terminal(char *buffer, sem_t *sem, pthread_mutex_t *mutex, int pid);

int remove_terminal(int pid);

/**
* @brief Get number of active terminals
*
* Get number of active terminals
*
* @return Number of active terminals
*/
int get_terminal_count();

/**
* @brief Get the index where the terminal data is stored
*
* Get Get the index where the terminal data is stored
*
* @return Index of the data, -1 if there is no terminal with this PID
*/
int get_terminal_index(int pid);

/**
* @brief Free all resources of the communication unit
*
* Destroy mutexes, stop the receiver thread and free the port
*
* @return 0 void
*/
void close_communication_unit();

#endif
