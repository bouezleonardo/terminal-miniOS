#include "message.h"

int init_message(Message *msg){
  int ret_code;
  
  // Initialize a binary semaphore
  ret_code = sem_init(&msg->msg_received, 0, 1);
  
  if(ret_code == -1) return -1;
  
  // Initial wait on semaphore
  ret_code = sem_wait(&msg->msg_received);
  
  if(ret_code == -1) return -1;
  
  // Initialize mutex
  pthread_mutex_init(&msg->mutex_buffer, NULL);
  
  return 0;
}

int read_message(Message *msg, char *data){}

void close_message(Message *msg){}
