#include "terminal.h"
#include "main.h"
#include <pthread.h>
#include <stdio.h>

//########################### STATIC VARIABLES #################################

// Terminal thread
static pthread_t terminal_thr;

// Terminal the thread will run
static Terminal *current_terminal;

static int running;

//###################### STATIC FUNCTION DECLARATIONS ##########################
static void *run_terminal(void *arg);

//########################## FUNCTION DEFINITIONS ##############################
int init_terminal(){
  int ret_code;
  
  // Initialize variables
  running = 1;
  current_terminal = NULL;
  
  ret_code = pthread_create(&terminal_thr, NULL, run_terminal, NULL);
  
  if(ret_code != 0){
    printf("\n[Terminal] Failed to create terminal thread");
    return -1;
  }
  
  return 0;
}

static void *run_terminal(void *arg){
  // Input from the user to the Main thread
  char input[50];
  
  while(running){
    if(current_terminal != NULL){
      printf("\n\n|TERMINAL %d INTERFACE|\n", current_terminal->pid);
      
      // Print message buffer
      printf("\n%s\n", current_terminal->msg.buffer);
    }
  }
  
  return NULL;
}

void activate_terminal(Terminal *terminal){
  current_terminal = terminal;
}

int init_terminal_struct(Terminal *terminal, int pid){
  int ret_code;
  
  // Initialize Terminal message
  ret_code = init_message(&terminal->msg);
  
  if(ret_code != 0){
    printf("\n[Terminal] Failed to initialize message for struct Terminal. PID %d", pid);
    return -1;
  }
  
  // Initialize PID
  terminal->pid = pid;
  
  return 0;
}

void close_terminal(){
  running = 0;
}
