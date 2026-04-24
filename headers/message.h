#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <pthread.h>

// Size of Message struct buffer
#define MSG_BUFFER_SIZE 1024

typedef struct {
  char buffer[MSG_BUFFER_SIZE]; // Where the data received is stored
  int msg_received;             // Indicates if a message was received
  pthread_mutex_t mutex_buffer; // Protects the access to buffer
  pthread_cond_t cond_received; // Condition to signal a message was received
} Message;

/**
* @brief Initialize a Message
*
* Given a Message, initizalizes its buffer mutex and received condition variables
*
* @param msg Pointer to the Message to be initialized
*
* @return 0 if sucessful, -1 if it is not
*/
int init_message(Message *msg);

//TODO: write a read function
int read_message(Message *msg, char *data);

/**
* @brief Write data to the buffer of a Message
*
* Given a Message and a string, writes the string to the Messages buffer
* using the mutex buffer. If append is set to 0, overwrites the data in 
* the buffer, if append is set to 1, add the data to the end of the buffer
*
* @param msg Pointer to the Message
* @param data String to be written in the buffer
* @param append Integer to select the append option
*
* @return 0 if sucessful, -1 if it is not
*/
int write_message(Message *msg, char *data, int append);

/**
* @brief Close a Message
*
* Given a Message, destroy its buffer mutex and condition variable
*
* @param msg Pointer to the Message to be initialized
*
* @return void
*/
void close_message(Message *msg);

#endif
