#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include "communication_unit.h"
#include <pthread.h>
#include <semaphore.h>

/**
 * @struct Terminal
 * @brief Represents a terminal
 */
typedef struct {
  Message msg;           // Shared memory of the terminal
  pthread_t thr;         // Terminal thread
  int pid;               // PID of the microprocessor process
} Terminal;

int init_terminal(Terminal *terminal, int pid);

void close_terminal(Terminal *terminal);

#endif
