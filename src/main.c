#include "terminal.h"
#include "message_router.h"
#include "main.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>

//########################### STATIC VARIABLES #################################
// Terminal array to control the terminals
static Terminal terminals[MAX_PROCESS_COUNT];

// Mutex for protecting access to the terminals array
static pthread_mutex_t mutex_terminals;

// Variables for treating requests
static pthread_mutex_t mutex_requests;
static pthread_cond_t cond_requests;
static int new_pid, new_ret, destroy_pid, destroy_ret, tab_step, tab_ret;

// Number of current terminals
static int count;

// Index of the terminal with has the current IO
static int current_tab;

//###################### STATIC FUNCTION DECLARATIONS ##########################

static void treat_new_terminal_req();
static void treat_destroy_terminal_req();
static void treat_tab_switch_req();

/**
* @brief Get the index where the terminal data is stored
*
* Get Get the index where the terminal data is stored
*
* @return Index of the terminal in the terminals array, -1 if there is no terminal with this PID
*/
static int get_terminal_index(int pid);

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

static void switch_tab(int new_tab);

//######################## FUNCTION DEFINITIONS ################################

int main(){
  int ret_code = 0;
  
  WINDOW *win; // Main thread Window
  
  // Height and Width of the terminal and the Main thread Window
  int max_y, max_x, win_h, win_w;
  
  // Input from the user to the Main thread
  char input[50];
  
  // Initialize variables
  count = 0;
  current_tab = -1;
  tab_step = 0;
  new_pid = -1;
  destroy_pid = -1;
  
  win_h = 3;
  win_w = 25;
  
  // Initialize mutex for terminals array
  ret_code += pthread_mutex_init(&mutex_terminals, NULL);
  
  // Initialize mutex and cond request
  ret_code += pthread_mutex_init(&mutex_requests, NULL);
  ret_code += pthread_cond_init(&cond_requests, NULL);
  
  if(ret_code != 0){
   perror("\n[Main] Failed to initialize Main thread");
   return -1;
  }
 
  // Initialize message router
  ret_code = init_message_router();
  
  if(ret_code != 0) return -1;
  
  // Initialize ncurses
  initscr();
  
  // Enable special keys
  keypad(stdscr, 1);
  getmaxyx(stdscr, max_y, max_x);
  refresh();
  
  win = newwin(win_h, win_w, 1, (max_x-win_w)/2);
  while(strcmp(input, "exit") != 0){
    box(win, 0, 0);
    
    if(count == 0){
      mvwprintw(win, 1, 1, "Main Thread Waiting");
    }else{
      mvwprintw(win, 1, 1, "Thread %d", terminals[current_tab].arg.pid);
    }
    wrefresh(win);
    
    // Treat requests
    if(new_pid != -1) treat_new_terminal_req();
    if(destroy_pid != -1) treat_destroy_terminal_req();
    if(tab_step != 0) treat_tab_switch_req();
    
    // Switch for tab 0 when the first terminal receives a message
    if(count != 0 && current_tab == -1){
      // Muda a tab
      switch_tab(0);
      // Limpa a window do main
      wclear(win);
    }
    usleep(25000);
  }
  
  // End ncurses
  endwin();
  close_communication_unit(); 
  return 0;  
}

//########################### REQUEST FUNCTIONS #################################
int request_new_terminal(int pid){
  if(pid < 0) return -1;
  
  pthread_mutex_lock(&mutex_requests);
  
  new_pid = pid;
  while(new_pid == pid) pthread_cond_wait(&cond_requests, &mutex_requests);
  
  pthread_mutex_unlock(&mutex_requests);
  
  return new_ret;
}

int request_destroy_terminal(int pid){
  if(pid < 0) return -1;
  
  pthread_mutex_lock(&mutex_requests);
  
  destroy_pid = pid;
  while(destroy_pid == pid) pthread_cond_wait(&cond_requests, &mutex_requests);
  
  pthread_mutex_unlock(&mutex_requests);
  
  return destroy_ret;
}

int request_tab_switch(int tab){
  if(tab == 1 || tab == -1){
    pthread_mutex_lock(&mutex_requests);
    
    tab_step = tab;
    while(tab_step == tab) pthread_cond_wait(&cond_requests, &mutex_requests);
    
    pthread_mutex_unlock(&mutex_requests);
  }
  return tab_ret;
}

//####################### TREAT REQUEST FUNCTIONS ##############################
static void treat_new_terminal_req(){
  pthread_mutex_lock(&mutex_requests);
  pthread_mutex_lock(&mutex_terminals);
  
  // Add new terminal
  new_ret = add_terminal(new_pid);

  new_pid = -1;
  
  pthread_mutex_unlock(&mutex_terminals);
  pthread_mutex_unlock(&mutex_requests);
  
  // Broadcast to all waiting threads
  pthread_cond_broadcast(&cond_requests); 
}

static void treat_destroy_terminal_req(){
  int index;
  
  pthread_mutex_lock(&mutex_requests);
  pthread_mutex_lock(&mutex_terminals);
  
  index = get_terminal_index(destroy_pid);
  
  //Remove terminal
  destroy_ret = remove_terminal(destroy_pid);
  
  destroy_pid = -1;
  
  // If the there are no more terminals
  if(count == 0){
    current_tab = -1;
  }else if(index == current_tab){
    // If the removed terminal was the current one
    if(current_tab - 1 >= 0) switch_tab(current_tab - 1);
    else switch_tab(current_tab + 1);
  }
  
  pthread_mutex_unlock(&mutex_terminals);
  pthread_mutex_unlock(&mutex_requests);
  
  // Broadcast to all waiting threads
  pthread_cond_broadcast(&cond_requests); 
}

static void treat_tab_switch_req(){
  // New tab is current tab -1 or +1 to go left or right
  int new_tab = current_tab + tab_step;
  
  pthread_mutex_lock(&mutex_requests);
    
  if(new_tab < count && new_tab >= 0){
    switch_tab(new_tab);
    tab_ret = 0;
  }else{
    tab_ret = -1;
  }
  
  // Indicates there is no more tab switch requests
  tab_step = 0;
  
  pthread_mutex_unlock(&mutex_requests);
  
  // Broadcast to all waiting threads
  pthread_cond_broadcast(&cond_requests); 
}

//########################## AUXILIARY FUNCTIONS ################################
Message* get_terminal_message(int pid){
  Message *msg = NULL;
  int index = -1;
  
  pthread_mutex_lock(&mutex_terminals);
  
  // Get terminal index in the array
  if(count != 0) index = get_terminal_index(pid);

  // Get terminal msg
  if(index != -1) msg = &terminals[index].arg.msg;

  pthread_mutex_unlock(&mutex_terminals);
  
  return msg;
}

static int get_terminal_index(int pid){
  int index, found = 0;
  
  if(count <= 0) return -1;
  
  // Searches for the terminal associated with pid
  for(index = 0;index < count;++index){
    if(terminals[index].arg.pid == pid){
       found = 1;
       break;
    }
  }
  
  if(found == 0) return -1;
  
  return index;
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
  int index;
  
  if(count == 0) return 0;
  
  // Get index with this PID
  index = get_terminal_index(pid);
  
  // If PID is not found
  if(index == -1) return -1;
  
  close_terminal(&terminals[index]);
  
  // Overwrite positions
  memmove(terminals+index, terminals+index+1, count-index-1);
  count--;
  
  return 0;
}

static void switch_tab(int new_tab){
  terminals[current_tab].arg.has_tab = 0;
  terminals[new_tab].arg.has_tab = 1;
  current_tab = new_tab;
  // Clear the screen
  clear();
}
