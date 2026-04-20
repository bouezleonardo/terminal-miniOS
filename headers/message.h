#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <pthread.h>
#include <semaphore.h>

// Size of Message struct buffer
#define MSG_BUFFER_SIZE 1024

typedef struct {
  char buffer[MSG_BUFFER_SIZE]; // Where the data received is stored
  sem_t msg_received;                // Indicates if a message was received
  pthread_mutex_t mutex_buffer;      // Protects the access to buffer
} Message;


int init_message(Message *msg);

int read_message(Message *msg, char *data);

void close_message(Message *msg);

#endif
