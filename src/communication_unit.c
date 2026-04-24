#include "communication_unit.h"
#include "serial_communication.h"
#include <errno.h>
#include <stdlib.h>

//############################ STATIC VARIABLES ################################

// Buffer to hold incoming data
static char buffer[RECEIVE_BUFFER_SIZE];

// Receiver thread that receive the messages from the microcontroller
static pthread_t receiver_thr;

// mutex_send Protect send_data() critical section
// mutex_ack Protect ack to guarantee consistency when a time out occurs
static pthread_mutex_t mutex_send, mutex_ack; 

// Message struture to send data to Main Thread
static Message *msg_main;

// Indicates to the Main thread the destination of the received data
static int *pid_destination;

// File descriptor for USB device
static int fd;

// Indicates the microcontroller has received data
static int ack;
// Expected PID of the ack
static int pid_ack;
// Indicates if a timeout has occurred
static int timeout;

// Mantains the receiver thread listening to the port
static int listening;

//##################### STATIC FUNCTION DECLARATIONS ###########################

/**
* @brief Receives data from the microcontroller
*
* Listens to the port by reading from it periodically, extracts messagens
* from the bytes read and puts the data in the corresponding terminal buffer
*
* @return void*
*/
static void* receive_data();

/**
* @brief Extract the data from a message from the microcontroller
*
* Extracts the data from a message formatted like SOH <PID> STX <DATA> ETX or 
* or SOH <PID> ACK or SOH <PID> EOT. These messages are stored in the buffer
*
* @return number of bytes in data, -1 if there was an error
*/
static int extract_data(char *data, int *pid, int len);

/**
* @brief Builds a formatted message
*
* Builds a message with the data string to send to the microcontroller formatted
* like SOH <PID> STX <DATA> ETX. The size of the message is fixed by MSG_SIZE.
* If the formatted message does not acheive MSG_SIZE bytes, the rest of msg is
* filled with SUB character
* 
* @return 0 if the message is built, -1 if there was an error
*/
static int build_msg(char *data, char *msg, int pid);

/**
* @brief Put the received data into the Main thread buffer
*
* Given a string of data and a PID, put it into the Main thread
* buffer and set the semaphore to signal a receive.
* 
* @return 0 if the data is put into the buffer, -1 if there was an error
*/
static int set_main_message(char *data, int pid);

//######################## FUNCTION DEFINITIONS ################################

int init_communication_unit(Message *msg, int *pid){
  // Return code of the functions
  int ret_code = 0;
  
  // Open serial port
  fd = open_serial_port(PORT_NAME);
  
  if(fd == -1) return -1;
  
  // Configure serial port
  ret_code = configure_serial_port(fd, BAUD_RATE);
  
  if(ret_code == -1) return -1;
  
  buffer[RECEIVE_BUFFER_SIZE-1] = '\0';
  
  // Initialize mutexes
  pthread_mutex_init(&mutex_send, NULL);
  pthread_mutex_init(&mutex_ack, NULL);
  
  ack = 0;
  timeout = 0;
  pid_ack = -1;
  
  // Set the Main msg and pid destination
  msg_main = msg;
  pid_destination = pid;
  
  // Set the receiver thread to listen to the port
  listening = 1;
  
  // Initialize receiver thread
  pthread_create(&receiver_thr, NULL, receive_data, NULL);
  
  return 0;
}

static void* receive_data(){
  int bytes_received, bytes_read, bytes_extracted, pid, len;
  char str_read[RECEIVE_BUFFER_SIZE], data[RECEIVE_BUFFER_SIZE];

  // Wait for data
  len = 0;
  while(listening){
    usleep(100000);
    
    // Reset buffer
    if(len >= RECEIVE_BUFFER_SIZE-1){
      len = 0;
      buffer[0] = '\0';
    }
    // Read from port
    bytes_read = read_ascii_response(fd, str_read, RECEIVE_BUFFER_SIZE);
    
    if(bytes_read != -1){  
      if(bytes_read < RECEIVE_BUFFER_SIZE-len-1) bytes_received = bytes_read;
      else bytes_received = RECEIVE_BUFFER_SIZE-len-1;
      
      memcpy(buffer+len, str_read, bytes_received);
      buffer[bytes_received + len]='\0';
      len = strlen(buffer);
    }
    
    str_read[0] = '\0';
    
    printf("\nLEN(%d) BUFF:%s",len, buffer);
    
    // If no bytes were received
    if(bytes_read <= 0) continue;
    
    // Try to extract data from the bytes received
    bytes_extracted = extract_data(data, &pid, len);
    len = strlen(buffer);
    
    // If no data was extracted
    if(bytes_extracted <= 0) continue;
    
    // Check for ack
    pthread_mutex_lock(&mutex_ack);
    if(bytes_extracted == 1 && data[0] == ACK && pid == pid_ack && timeout == 0){
      ack = 1;
      pthread_mutex_unlock(&mutex_ack);
      continue;
    }
    pthread_mutex_unlock(&mutex_ack);
    
    // Send the data to main thread
    set_main_message(data, pid);
    
    data[0] = '\0';
  }
  
  return NULL;
}

int send_data(char *data, int pid){
  int ret_code;
  
  char msg[MSG_SIZE+1];
  
  build_msg(data, msg, pid);
  
  pthread_mutex_lock(&mutex_send);
  
  pthread_mutex_lock(&mutex_ack);
  pid_ack = pid;
  timeout = 0;
  ack = 0;
  pthread_mutex_unlock(&mutex_ack);
  
  // Send msg to the microcontroller
  ret_code = send_ascii_command(fd, msg);
  
  // Only waits for acknowledge if data is sucessfully sent
  if(ret_code != -1){
    int time_count = 0;
    
    // Wait for acknowledge
    while(ack != 1 && time_count < ACK_TIMEOUT/100){
      usleep(100);
      time_count++;
    }
    
    pthread_mutex_lock(&mutex_ack);
    timeout = 1;
    
    // Check if acknowledge has arrived
    if(ack != 1){
      ret_code = -2;
    }
    pthread_mutex_unlock(&mutex_ack);
  }
  pthread_mutex_unlock(&mutex_send);
  
  return ret_code;
}

static int extract_data(char *data, int *pid, int len){
  // Used to know where sections of the message start and end
  int soh_index = -1, stx_index = -1, etx_index = -1;
  
  // Used to find the PID
  int pid_start = -1, pid_end = -1, pid_length;
  long int aux;
  
  // Amount of bytes extracted (size of data that will go to the terminal)
  // Total message size (bytes extracted + delimiters + pid)
  int bytes_extracted = 0, msg_size = 1;
  
  char pid_string[16];
  
  // Searches the buffer for SOH
  for(int i = 0;i < len;++i){
    if(buffer[i] == SOH){
      soh_index = i;
      break;
    }
  }
  
  // If there is no SOH
  if(soh_index == -1) return -1;
  
  pid_start = soh_index + 1;
  
  // Searches the buffer for STX, ACK or EOT
  for(int i = soh_index + 1;i < len;++i){
    if(buffer[i] == ACK || buffer[i] == EOT) {
      data[0] = buffer[i];
    
      bytes_extracted = 1;
      
      pid_end = i - 1;
      break;
    }else if(buffer[i] == STX){
      stx_index = i;
      pid_end = i - 1;
      break;
    }else if(buffer[i] == SOH){
      break; // If there is another SOH in sequence
    }
  }
  
  if(pid_end == -1) return -1;
  
  // If there is a start of text
  if(stx_index != -1){
    // Searches the buffer for ETX
    for(int i = stx_index+1;i < len;++i){
      if(buffer[i] == ETX){
        etx_index = i;
        break;
      }else if(buffer[i] == STX){
        break; // If there is another STX in sequence
      }
    }
    if(etx_index == -1) return -1;
  }
  
  // Find PID
  pid_length = pid_end - pid_start + 1;
  
  // Check if it is possible to extract a PID
  if(pid_length > 15) return -1;
  
  // Get PID string
  strncpy(pid_string, buffer + pid_start, pid_length);
  pid_string[pid_length] = '\0';
  
  // Reset errno to check for erros in strtol
  errno = 0;
  aux = strtol(pid_string, NULL, 10);
  
  // Check if there was no error in strtol
  if(errno != 0) return -1;
  
  // Check if strtol has returned a valid PID integer
  if(aux > 2147483647 || aux < 0) return -1; 
  
  // Get PID integer
  *pid = (int)aux; 

  // If there is text
  if(stx_index != -1 && etx_index != -1){
    bytes_extracted =  (etx_index-1) - (stx_index+1) + 1;
    
    strncpy(data, buffer + stx_index + 1, bytes_extracted);
    
    msg_size += 2;
  }
  
  data[bytes_extracted] = '\0';
  
  msg_size += pid_length + bytes_extracted;
  
  // Overwrite the message from the buffer
  int data_len = len - msg_size - soh_index;
  char buffer_string[data_len+1];
  
  memcpy(buffer_string, buffer + soh_index + msg_size, data_len);
  buffer_string[data_len] = '\0';
  
  memcpy(buffer, buffer_string, data_len);
  buffer[data_len] = '\0';
  
  return bytes_extracted;
}

static int build_msg(char *data, char *msg, int pid){
  // Variable to check the return values of functions
  int ret_code, len;
  char pid_string[10];
  
  // Get PID string
  ret_code = sprintf(pid_string, "%d", pid);
  
  if(ret_code < 0) return -1;
  
  // Check if the data will fit into the message
  // Subtract for to account for the 3 delimiters plus the null terminator
  if(strlen(data) > MSG_SIZE - ret_code - 4) return -1;
  
  // Reset ret_code
  ret_code = 0;
  
  // Build the message
  msg[0] = SOH;
  msg[1] = '\0';
  
  // Put PID into msg
  ret_code = sprintf(msg + 1, "%s", pid_string);
  if(ret_code < 0) return -1;
  
  len = strlen(msg);
  
  msg[len] = STX;
  msg[len+1] = '\0';
  
  // Put  into msg
  ret_code = sprintf(msg + strlen(msg), "%s", data);
  if(ret_code < 0) return -1;
  
  len = strlen(msg);
  
  msg[len] = ETX;
  msg[len+1] = '\0';
  
  // Fill the rest of the bytes with SUB characters if needed
  len = strlen(msg);
  if(len < MSG_SIZE) memset(msg + len, SUB, MSG_SIZE - len);
  
  msg[MSG_SIZE] = '\0';
  
  return 0;
}

static int set_main_message(char *data, int pid){
  int ret_code = 0;
   
  pthread_mutex_lock(&msg_main->mutex_buffer);
  
  // Wait if there there is still a message to be read
  if(msg_main->msg_received == 1){
    ret_code = pthread_cond_wait(&msg_main->cond_received, &msg_main->mutex_buffer);
  }
  
  if(ret_code != 0){
    pthread_mutex_unlock(&msg_main->mutex_buffer);
    return -1;
  }
  
  // PID of the destination
  *pid_destination = pid;
  pthread_mutex_unlock(&msg_main->mutex_buffer);
  
  // Overwrite Main thread buffer (append = 0)
  ret_code = write_message(msg_main, data, 0);
  
  // Change msg_received
  msg_main->msg_received = 1;
  
  // Signals that a message was received
  ret_code = pthread_cond_signal(&msg_main->cond_received);
  
  return ret_code;
}

void close_communication_unit(){
  listening = 0;
  pthread_mutex_destroy(&mutex_send);
  pthread_mutex_destroy(&mutex_ack);
  close_serial_port(fd);
}


