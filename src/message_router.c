#include "message_router.h"
#include "communication_unit.h"
#include "main.h"
#include <stdio.h>
#include <pthread.h>

//########################### STATIC VARIABLES #################################
static Message msg;
static pthread_t router_thr;
static int running;
//###################### STATIC FUNCTION DECLARATIONS ##########################
static void* route_message(void *arg);

//######################## FUNCTION DEFINITIONS ################################

int init_message_router(){
  // Return codes and pid
  int ret_code = 0;
  
  // Thread loop
  running = 1;
  
  ret_code = init_message(&msg);
  
  if(ret_code != 0){
    printf("\n[Router] Failed to initialize router message");
    return -1;
  }
  
  // Create router thread
  ret_code = pthread_create(&router_thr, NULL, route_message, NULL);
  
  if(ret_code != 0){
    printf("\n[Router] Failed to create router thread");
    return -1;
  }
  
  // Initialize communication unit
  ret_code = init_communication_unit();
  
  if(ret_code < 0) {
    printf("\n[Router] Failed to initialize communication unit");
    return -1;
  }
  
  return 0;
}


static void* route_message(void *arg){
  // Input from the user to the Main thread
  char data[MSG_BUFFER_SIZE];
  
  // Terminal message that will be written to
  Message *msg_terminal;
  
  // Return codes and pid
  int msg_read, ret_code, read_pid, index;
  
  set_communication_msg(&msg);
  set_communication_pid(&read_pid);
  
  msg_read = -1;
  while(running){
    // Wait for a message
    msg_read = sread_message(&msg, data, MSG_BUFFER_SIZE);

    // If a message was read
    if(msg_read != -1){
      index = get_terminal_index(read_pid);
      
      // If no terminal with this PID was found
      if(index == -1){
        // Request the Main thread to create the new terminal 
        index = request_new_terminal(read_pid);
        
        // If the terminal was not created
        if(index == -1) continue;
      }
      
      // Get terminal msg
      msg_terminal = get_terminal_message(index);
      
      // Set the message into the corresponding terminal
      if(msg_terminal != NULL) ret_code = write_message(msg_terminal, data);
   
      if(ret_code != 0) printf("\n[Router] Failed to write data to terminal. PID(%d)", read_pid);
      
    }
  }

  close_communication_unit(); 
  return NULL;  
}

void close_message_router(){
  running = 0;
}
