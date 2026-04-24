#include "message.h"
#include "string.h"
#include <stdio.h>

int init_message(Message *msg){
  int ret_code;
  
  // Initialize msg_received
  msg->msg_received = 0;
  
  // Initialize received condition
  ret_code = pthread_cond_init(&msg->cond_received, NULL);
  
  if(ret_code != 0) return -1;
  
  // Initialize mutex
  pthread_mutex_init(&msg->mutex_buffer, NULL);
  
  return 0;
}

//TODO: write a read function
int read_message(Message *msg, char *data){}

int write_message(Message *msg, char *data, int append){
  int ret_code, len = 0;
  
  // Check is append is set correctly
  if(append != 0 && append != 1) return -1;
  
  // Lock buffer mutex
  pthread_mutex_lock(&msg->mutex_buffer);
  
  // Get buffer string length
  if(append == 1) len = strlen(msg->buffer);
  
  // Check if buffer has space available for the data
  // Subtract 1 to account for the null terminator
  if(strlen(data) > MSG_BUFFER_SIZE - len - 1){
    pthread_mutex_unlock(&msg->mutex_buffer);
    return -1;
  }
  
  memcpy(msg->buffer+len, data, strlen(data));
  msg->buffer[strlen(data)+len] = '\0';

  pthread_mutex_unlock(&msg->mutex_buffer);
  
  return ret_code;
}

void close_message(Message *msg){
  // Destroy the condition variable
  (void) pthread_cond_destroy(&msg->cond_received);
  
  // Destroy the mutex
  pthread_mutex_destroy(&msg->mutex_buffer);
}
