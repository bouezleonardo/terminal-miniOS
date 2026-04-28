#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "message.h"

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

// Timeout in ms for acknowledge
#define ACK_TIMEOUT 1000

// Size of the receive buffer
#define RECEIVE_BUFFER_SIZE 4096

// Amount of bytes the terminals send at time to the microcontroller
#define MSG_SIZE 16

/**
* @brief Prepares the communication unit
*
* Initializes the port and the mutexes
*
* @return 0 if the initialization is sucessful, -1 if it is not
*/
int init_communication_unit();

/**
* @brief Set the message for the communication unit
*
* Set the message that the communication unit thread will put data into
*
* @return void
*/
void set_communication_msg(Message *msg);

/**
* @brief Set the PID pointer to indicate whats the destination PID of the message 
*
* Set the PID pointer to indicate whats the destination PID of the message
*
* @return void
*/
void set_communication_pid(int *pid);

/**
* @brief Get the maximum size of the data that can be transmitted with this PID 
*
* Get the maximum size of the data that can be transmitted with this PID based
* on MSG_SIZE.
*
* @return void
*/
int get_max_data_size(int pid);

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

/**
* @brief Free all resources of the communication unit
*
* Destroy mutexes, stop the receiver thread and free the port
*
* @return void
*/
void close_communication_unit();

#endif
