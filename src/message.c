#include "message.h"
#include "string.h"
#include <stdio.h>

//######################## FUNCTION DEFINITIONS ################################
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

int read_message(Message *msg, char *data, int size){
  int len, bytes_read = 0;
  
  // Lock buffer mutex
  pthread_mutex_lock(&msg->mutex_buffer);
  
  len = strlen(msg->buffer);
  
  // Put the minimum amount of bytes between size-1 and all the data
  if(size-1 < MSG_BUFFER_SIZE-len-1) bytes_read = size-1;
  else bytes_read = len;
  
  // Copy the data in the buffer to data
  memcpy(data, msg->buffer, bytes_read);
  data[bytes_read] = '\0';
  
  // Overwrite read data in the buffer
  memcpy(msg->buffer, msg->buffer+bytes_read, len-bytes_read);
  msg->buffer[len-bytes_read] = '\0';

  pthread_mutex_unlock(&msg->mutex_buffer);
  
  return bytes_read;
}

int write_message(Message *msg, char *data){
  int ret_code = 0, len = 0;
  
  // Lock buffer mutex
  pthread_mutex_lock(&msg->mutex_buffer);
  
  // Get buffer string length
  len = strlen(msg->buffer);
  
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

int sread_message(Message *msg, char *data, int size){
  int ret_code = 0;
  
  pthread_mutex_lock(&msg->mutex_buffer);
  // Wait if there is no message
  if(msg->msg_received == 0) ret_code = pthread_cond_wait(&msg->cond_received, &msg->mutex_buffer);

  if(ret_code != 0) {
    pthread_mutex_unlock(&msg->mutex_buffer);
    pthread_cond_signal(&msg->cond_received);
    return -1;
  }
  
  // Incates the message was read
  msg->msg_received = 0;
  
  pthread_mutex_unlock(&msg->mutex_buffer);
  
  // Read message
  ret_code = read_message(msg, data, size);
  
  // Signals the message was read
  pthread_cond_signal(&msg->cond_received);
  
  return ret_code;
}

int swrite_message(Message *msg, char *data){
  int ret_code = 0;
  
  pthread_mutex_lock(&msg->mutex_buffer);
  
  // Wait until the message is read
  if(msg->msg_received == 1) ret_code = pthread_cond_wait(&msg->cond_received, &msg->mutex_buffer);
  
  if(ret_code != 0) {
    pthread_mutex_unlock(&msg->mutex_buffer);
    pthread_cond_signal(&msg->cond_received);
    return -1;
  }
  // Incates the data was written
  msg->msg_received = 1;
  
  pthread_mutex_unlock(&msg->mutex_buffer);
  
  // Write message
  ret_code = write_message(msg, data);

  // Signals to wake the reader thread
  pthread_cond_signal(&msg->cond_received);
  
  return ret_code;
}

void close_message(Message *msg){
  // Destroy the condition variable
  (void) pthread_cond_destroy(&msg->cond_received);
  
  // Destroy the mutex
  pthread_mutex_destroy(&msg->mutex_buffer);
}
