#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include "communication_unit.h"

// Max process count in the microcontroller
#define MAX_PROCESS_COUNT 10

/**
 * @struct TerminalArg
 * @brief Represents the arguments that are passed to the terminal threads functions
 */
typedef struct {
  Message msg;             // Message where the data from the microcontroller will be stored
  int has_tab;             // Indicates this terminal has the tab, i.e. is allowed to perform IO
  int running;             // Indicates this thread is still running
  int pid;                 // PID associated with this terminal
} TerminalArg; 

/**
 * @struct Terminal
 * @brief Represents a terminal for Main thread to control
 */
typedef struct {
  TerminalArg arg;         // Arguments that will be passed to the function
  pthread_t thr;           // Terminal thread
} Terminal; 

int init_terminal();
int close_terminal(Terminal *terminal);

#endif
