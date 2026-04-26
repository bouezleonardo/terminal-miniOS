#include "communication_unit.h"
#include "terminal.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// Terminal array to control the terminals
static Terminal terminals[MAX_PROCESS_COUNT];

// Number of current terminals
int count;

/**
* @brief Adds a terminal to the terminals array
*
* Adds the terminal pointer to the terminals array
*
* @param terminal Pointer to the terminal that will be added
*
* @return 0 if terminal is added sucessfully, -1 if it is not
*/
static int add_terminal(int pid);

/**
* @brief Adds a terminal to the terminals array
*
* Removes the terminal pointer from the terminals array given a
* PID
*
* @param pid PID of the process associated with the terminal
*
* @return 0 if terminal is removed sucessfully, -1 if it is not
*/
static int remove_terminal(int pid);

/**
* @brief Get the index where the terminal data is stored
*
* Get Get the index where the terminal data is stored
*
* @return Index of the data, -1 if there is no terminal with this PID
*/
static int get_terminal_index(int pid);

/**
* @brief Puts the received data into the corresponding terminal buffer
*
* Given a string of data and a PID, put it into the corresponding terminal
* 
* @return 0 if the data is put into the buffer, -1 if there was an error
*/
static int set_terminal_message(char *data, int pid);

int main(){
  // Main thread message
  Message msg;
  
  // Input from the user to the Main thread
  char input[50], data[MSG_BUFFER_SIZE];
  
  // Return codes and pid
  int ret_code, pid;
  
  // Initialize variables
  count = 0;
  ret_code = init_message(&msg);
  
  if(ret_code != 0){
    perror("Failed to initialize Main thread");
    return -1;
  }
  
  // Initialize communication unit
  ret_code = init_communication_unit(&msg, &pid);
  
  if(ret_code < 0) {
    printf("Failed to initialize communication unit");
    return -1;
  }
  
  printf("\n|MAIN THREAD CONTROL|\n");
  while(strcmp(input, "exit") != 0){
    ret_code = sread_message(&msg, data, MSG_BUFFER_SIZE);
    
    if(ret_code != -1) printf("\nMSG(%d):%s\n", pid, data);
  }
  
  close_communication_unit(); 
  return 0;  
}

static int set_terminal_message(char *data, int pid){
  int ret_code, i, len;
  
  
  
  return ret_code;
}

static int add_terminal(int pid){
  int ret_code;
  
  if(count >= MAX_PROCESS_COUNT) return -1;
  
  // Initialize terminal
  ret_code = init_terminal(&terminals[count], pid);
  
  // Check if terminal was sucessfully initialized
  if(ret_code == -1) return -1;
  
  count++;
  
  return 0;
}

static int remove_terminal(int pid){
  int i;
  
  // Get index with this PID
  i = get_terminal_index(pid);
  
  // If PID is not found
  if(i == -1) return -1;
  
  // Overwrite positions
  memmove(terminals+i, terminals+i+1, count-i-1);
  count--;
  
  return 0;
}

static int get_terminal_index(int pid){
  int index, found = 0;
  
  if(count <= 0) return -1;
  
  // Searches for the terminal associated with pid
  for(index = 0;index < count;++index){
    if(terminals[index].pid == pid){
       found = 1;
       break;
    }
  }
  
  if(found == 0) return -1;
  
  return index;
}
