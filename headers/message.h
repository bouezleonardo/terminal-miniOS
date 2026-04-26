#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <pthread.h>

// Size of Message struct buffer
#define MSG_BUFFER_SIZE 1024

/**
 * @struct Message
 * @brief Used to pass data between threads. Can be used synchronously or asynchronously
 */
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

/**
* @brief Read an specified amount of data from the Message buffer 
*
* Given a Message, a char array and the size of the array, read size-1 bytes
* from the Message buffer
*
* @param msg Pointer to the Message
* @param data Pointer to the char array
* @param size Size of the char array
*
* @return amount of bytes read
*/
int read_message(Message *msg, char *data, int size);

/**
* @brief Write data to the buffer of a Message
*
* Given a Message and a string, writes the string to the Message buffer
*
* @param msg Pointer to the Message
* @param data String to be written in the buffer
*
* @return 0 if sucessful, -1 if it is not
*/
int write_message(Message *msg, char *data);

/**
* @brief Synchronously read an specified amount of data from the Message buffer
*
* Given a Message, a char array and the size of the array, read size-1 bytes
* from the Message buffer only if there is new data to be read. Blocks until
* there is new data
*
* @param msg Pointer to the Message
* @param data Pointer to the char array
* @param size Size of the char array
*
* @return amount of bytes read if sucessful, -1 if not
*/
int sread_message(Message *msg, char *data, int size);

/**
* @brief Synchronously write data to the buffer of a Message
*
* Given a Message and a string, writes the string to the Message buffer
* only if there is no new data in the buffer. Blocks until there is new
* data
*
* @param msg Pointer to the Message
* @param data String to be written in the buffer
*
* @return 0 if sucessful, -1 if it is not
*/
int swrite_message(Message *msg, char *data);

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
