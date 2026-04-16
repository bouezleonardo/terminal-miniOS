#include "communication_unit.h"

//############################ STATIC VARIABLES ################################

// Buffer to hold incoming data
static char buffer[RECEIVE_BUFFER_SIZE];

// Receiver thread that receive the messages from the microcontroller
static pthread_t receiver_thr;

// mutex_send Protect send_data() critical section
// mutex_terminals Protect add_terminal() and remove_terminal()
// mutex_ack Protect ack to guarantee consistency when a time out occurs
static pthread_mutex_t mutex_send, mutex_terminals, mutex_ack; 

// File descriptor for USB device
static int fd;

// Terminal struct array
static Terminal *terminals[MAX_PROCESS_COUNT];

// Number of terminals
static int count;

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
* @brief Puts the received data into the corresponding terminal buffer
*
* Given a string of data and a PID, put it into the corresponding terminal
* buffer and set the semaphore to signal a receive to the terminal.
* 
* @return 0 if the data is put into the buffer, -1 if there was an error
*/
static int set_terminal_buffer(char *data, int pid);

//######################## FUNCTION DEFINITIONS ################################

int init_communication_unit(){
  // Return code of the functions
  int ret_code;
  
  // Open serial port
  fd = open_serial_port(PORT_NAME);
  
  if(fd == -1) return -1;
  
  // Configure serial port
  ret_code = configure_serial_port(fd, BAUD_RATE);
  
  if(ret_code == -1) return -1;
  
  // Initialize mutexes
  pthread_mutex_init(&mutex_send, NULL);
  pthread_mutex_init(&mutex_terminals, NULL);
  pthread_mutex_init(&mutex_ack, NULL);
  
  count = 0;
  ack = 0;
  timeout = 0;
  pid_ack = -1;
  
  // Set the receiver thread to listen to the port
  listening = 1;
  
  // Initialize receiver thread
  pthread_create(&receiver_thr, NULL, receive_data, NULL);
  
  return 0;
}

static void* receive_data(){
  int bytes_read, bytes_extracted, pid, len;
  char data[RECEIVE_BUFFER_SIZE];

  // Wait for data
  len = 0;
  while(listening){
    usleep(100);
    
    // Reset buffer
    if(len == RECEIVE_BUFFER_SIZE-1) len = 0;
    
    // Read from port
    bytes_read = read_ascii_response(fd, buffer+len, RECEIVE_BUFFER_SIZE-len);
    
    //printf("Buffer(%d): %s", len, buffer);
    
    // Length of the buffer
    len = strlen(buffer);
    
    // If no bytes were received
    if(bytes_read <= 0) continue;
    
    // Try to extract data from the bytes received
    bytes_extracted = extract_data(data, &pid, len);
    len = strlen(buffer);
    
    // If no data was extracted
    if(bytes_extracted <= 0) continue;

    printf("PID: %d, Data: %s", pid,data);
    
    // Check for ack
    pthread_mutex_lock(&mutex_ack);
    if(bytes_extracted == 1 && data[0] == ACK && pid == pid_ack && timeout == 0){
      ack = 1;
      continue;
    }
    pthread_mutex_unlock(&mutex_ack);
    
    //TODO Put data in the corresponding terminal buffer
    //set_terminal_buffer(data, pid);
  }
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
  
  if(soh_index == -1) return -1;
  
  pid_start = soh_index + 1;
  
  // Searches the buffer for STX, ACK or EOT
  for(int i = soh_index + 1;i < len;++i){
    if(buffer[i] == ACK || buffer[i] == EOT) {
      data[0] = buffer[i];
      data[1] = '\0';
    
      bytes_extracted = 1;
      
      pid_end = i - 1;
      break;
    }else if(buffer[i] == STX){
      stx_index = i;
      pid_end = i - 1;
      break;
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
    data[bytes_extracted] = '\0';
    
    msg_size += 2;
  }
  
  msg_size += pid_length + bytes_extracted;
  
  // Overwrite the message from the buffer
  memmove(buffer+soh_index, buffer+msg_size, len - msg_size + 1);
  
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

static int set_terminal_buffer(char *data, int pid){
  int ret_code, i, len;
  
  // Get index with this PID
  i = get_terminal_index(pid);
  
  // If PID is not found
  if(i == -1) return -1;
  
  // Lock terminal buffer mutex
  pthread_mutex_lock(&terminals[i]->mutex_buffer);
  
  // Get terminal buffer string length
  len = strlen(terminals[i]->buffer);
  
  // Check if terminal buffer has space available for the data
  // Subtract 1 to account for the null terminator
  if(strlen(data) > TERMINAL_BUFFER_SIZE - len - 1) return -1;
  
  memcpy(terminals[i]->buffer+len, data, strlen(data));
  
  len = strlen(terminals[i]->buffer);
  terminals[i]->buffer[len] = '\0';
  
  // Signals to the terminal that a message was received
  ret_code = sem_post(&terminals[i]->msg_received);

  pthread_mutex_unlock(&terminals[i]->mutex_buffer);
  
  return ret_code;
}

int add_terminal(Terminal *terminal){
  if(count >= MAX_PROCESS_COUNT) return -1;
  
  pthread_mutex_lock(&mutex_terminals);
  terminals[count] = terminal;
  count++;
  pthread_mutex_unlock(&mutex_terminals);
  
  return 0;
}

int remove_terminal(int pid){
  int i;
  
  // Get index with this PID
  i = get_terminal_index(pid);
  
  // If PID is not found
  if(i == -1) return -1;
  
  pthread_mutex_lock(&mutex_terminals);
  
  // Overwrite positions
  memmove(terminals+i, terminals+i+1, count-i-1);
  
  count--;
  pthread_mutex_unlock(&mutex_terminals);
  
  return 0;
}

int get_terminal_index(int pid){
  int index, found = 0;
  
  if(count <= 0) return -1;
  
  pthread_mutex_lock(&mutex_terminals);
  // Searches for the terminal associated with pid
  for(index = 0;index < count;++index){
    if(terminals[index]->pid == pid){
       found = 1;
       break;
    }
  }
  pthread_mutex_lock(&mutex_terminals);
  
  if(found == 0) return -1;
  
  return index;
}

int get_terminal_count(){return count;}

void close_communication_unit(){
  listening = 0;
  pthread_mutex_destroy(&mutex_send);
  pthread_mutex_destroy(&mutex_terminals);
  pthread_mutex_destroy(&mutex_ack);
  close_serial_port(fd);
}


