#include "terminal.h"

//########################### STATIC VARIABLES #################################


//###################### STATIC FUNCTION DECLARATIONS ##########################
static void *run_terminal(void *arg);

//########################## FUNCTION DEFINITIONS ##############################
int init_terminal(Terminal *terminal, int pid){
  int ret_code;
  
  // Init terminal message
  ret_code = init_message(&terminal->msg);
  
  if(ret_code != 0) return -1;
  
  // Assign PID
  terminal->pid = pid;
  
  // Create the terminal thread
  //ret_code = pthread_create(&terminal->thr, NULL, run_terminal, (void *)&terminal->msg);
  //if(ret_code != 0) return -1;
  
  return 0;
}


void close_terminal(Terminal *terminal){


}
