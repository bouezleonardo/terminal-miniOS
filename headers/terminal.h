#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include "communication_unit.h"

#define MAX_PROCESS_COUNT 10

/**
 * @struct Terminal
 * @brief Represents a terminal for Main thread to control
 */
typedef struct {
  Message msg;             // Message where the data from the microcontroller will be stored
  int pid;                 // PID associated with this terminal
} Terminal; 

int init_terminal();
void activate_terminal(Terminal *terminal);
int init_terminal_struct(Terminal *terminal, int pid);
void close_terminal();

#endif
