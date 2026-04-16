#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include "communication_unit.h"
#include <pthread.h>
#include <semaphore.h>

#define TERMINAL_BUFFER_SIZE 1024

typedef struct {
  char buffer[TERMINAL_BUFFER_SIZE]; // Where the data received is stored
  sem_t msg_received;                // Indicates if a message was received
  pthread_mutex_t mutex_buffer;      // Protects the access to buffer
  int pid;                           // PID of the microprocessor process
} Terminal;

int init_terminal(int pid);

void close_terminal();

#endif
