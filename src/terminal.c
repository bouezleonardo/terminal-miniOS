#include "terminal.h"
#include "main.h"
#include "communication_unit.h"
#include <pthread.h>
#include <ncurses.h>

// Next tab key code
#define NEXT_TAB 261

// Previous tab key code
#define PREVIOUS_TAB 260

//########################### STATIC VARIABLES #################################


//###################### STATIC FUNCTION DECLARATIONS ##########################

static int init_terminal_arg(TerminalArg *arg, int pid);
static void *run_terminal(void *arg);
static void process_input(int max_input_size, int pid, int count, int input, int buffering, int *input_buffer);

//########################## FUNCTION DEFINITIONS ##############################

int init_terminal(Terminal *terminal, int pid){
  int ret_code;
  
  // Initialize arguments
  ret_code = init_terminal_arg(&terminal->arg, pid);

  if(ret_code != 0){
    printf("\n[Terminal] Failed to create terminal arguments. PID(%d)", pid);
    return -1;
  }
  
  ret_code = pthread_create(&terminal->thr, NULL, run_terminal, (void *)&terminal->arg);
  
  if(ret_code != 0){
    printf("\n[Terminal] Failed to create terminal thread. PID(%d)", pid);
    return -1;
  }
  
  return 0;
}

static void *run_terminal(void *arg){
  // Get the terminal arg
  TerminalArg *args = (TerminalArg *)arg;
  
  // Data received from the microcontroller
  char data_msg[MSG_BUFFER_SIZE];
  
  // Get the maximum data size that can be sent with this PID
  int max_input_size = get_max_data_size(args->pid);
  
  // Input from the user to the Main thread
  int input_buffer[max_input_size+1];

  /*
  count: counts the number of keys pressed
  input: stores the key pressed
  echo: enable(1)/disable(0) echoing the user input in the terminal
  buffering: enable(1)/disable(0) buffering user input before sending
  scroll: enable(1)/disable(0) scrolling of the window
  */
  int count, input, echo, buffering, scroll;
  
  // Auxiliary variables
  int msg_received;
  
  // Default initialization
  echo = 1;
  buffering = 1;
  scroll = 1;
  count = 0;
  
  // Enable continuous feed of data
  scrollok(stdscr, 1);
  while(args->running){
    msg_received = args->msg.msg_received;
    
    // If there is a message
    if(msg_received == 1) read_message(&args->msg, data_msg, MSG_BUFFER_SIZE);
    
    // If the thread is allowed to perform IO
    if(args->has_tab == 1){
      // Prints the data to the screen
      if(msg_received == 1) printw("\n%s", data_msg);
      
      // Blocks for 17ms
      timeout(17);
      input = getch();
      
      // If the user pressed a key
      if(input != ERR) process_input(max_input_size, args->pid, count, input, buffering, input_buffer);
    }else{
      timeout(200);
    }
  }
  
  return NULL;
}

static void process_input(int max_input_size, int pid, int count, int input, int buffering, int *input_buffer){
  int ret_code = -1;
  
  // Check if the user wants to change tabs
  if(input == PREVIOUS_TAB){
    ret_code = request_tab_switch(-1);
  }else if(input == NEXT_TAB){
    ret_code = request_tab_switch(1);
  }

  if(buffering && ret_code == -1){
    // If the user is typing more than the size of the buffer
    if(count == max_input_size && input != KEY_ENTER){
      printw("\nCharacter limit reached\n");
    }else if(input == KEY_ENTER){
      // Send data to the port
      send_data((char *)input_buffer, pid);
    }else{
      input_buffer[count] = input;
      count++;
    }
  }
}

static int init_terminal_arg(TerminalArg *arg, int pid){
  int ret_code;

  arg->running = 1;

  // Initialize Terminal message
  ret_code = init_message(&arg->msg);
  
  if(ret_code != 0){
    printf("\n[Terminal] Failed to initialize message for TerminalArg. PID %d", pid);
    return -1;
  }
  
  // Initialize PID
  arg->pid = pid;
  
  return 0;
}

int close_terminal(Terminal *terminal){
  terminal->arg.running = 0;
  return pthread_join(terminal->thr, NULL);
}
