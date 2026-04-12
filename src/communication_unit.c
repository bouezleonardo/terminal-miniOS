#include "communication_unit.h"

// Buffer to hold incoming data
static char buffer[RECEIVE_BUFFER_SIZE];

// mutex_send Protect send_data() critical section
// mutex_arrays Protect add_terminal() and remove_terminal()
// mutex_ack Protect ack to guarantee consistency when a time out occurs
static pthread_mutex_t mutex_send, mutex_arrays, mutex_ack; 

// File descriptor for USB device
static int fd;

// Terminal semaphores array
static sem_t *semaphores[MAX_PROCESS_COUNT];

// Terminal buffer pointers array
static char *buffers[MAX_PROCESS_COUNT];

// Terminal buffer mutexes array
static pthread_mutex_t *terminal_mutexes[MAX_PROCESS_COUNT];

// Terminal PIDs array
static char pids[MAX_PROCESS_COUNT];

// Number of terminals
static size_t count;

// Indicates the microcontroller has received data
static int ack;
// Expected PID of the ack
static int pid_ack;

// Mantains the receiver thread listening to the port
static int listening;

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
  pthread_mutex_init(&mutex_arrays, NULL);
  pthread_mutex_init(&mutex_ack, NULL);
  
  count = 0;
  ack = 0;
  pid_ack = -1;
  
  // Set the receiver thread to listen to the port
  listening = 1;
  
  return 0;
}

int extract_data(char *data, int *pid, int len){
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

void* receive_data(){
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
    if(bytes_extracted == 1 && data[0] == ACK && pid == pid_ack){
      pthread_mutex_lock(&mutex_ack);
      ack = 1;
      pthread_mutex_unlock(&mutex_ack);
      continue;
    }
    
    // Put data in the corresponding terminal buffer
    //set_terminal_buffer(data, pid);
  }
}

int send_data(char *data, int pid){
  int ret_code;
  
  //char *formatted_data = format_data(data, pid);
  
  pthread_mutex_lock(&mutex_send);
  pid_ack = pid;
  
  // Make sure ack is 0 for this transmission
  pthread_mutex_lock(&mutex_ack);
  ack = 0;
  pthread_mutex_unlock(&mutex_ack);
  
  // Send data to microcontroller
  ret_code = send_ascii_command(fd, data);
  
  // Only waits for acknowledge if data is sucessfully sent
  if(ret_code != -1){
    int time_count = 0;
    
    // Wait for acknowledge
    while(ack != 1 && time_count < ACK_TIMEOUT/100){
      usleep(100);
      time_count++;
    }
    
    pthread_mutex_lock(&mutex_ack);
    // Check if acknowledge has arrived
    if(ack != 1){
      ret_code = -2;
    }
    pthread_mutex_unlock(&mutex_ack);
  }
  pthread_mutex_unlock(&mutex_send);
  
  return ret_code;
}

int add_terminal(char *buffer, sem_t *sem, pthread_mutex_t *mutex, int pid){
  if(count >= MAX_PROCESS_COUNT) return -1;
  
  pthread_mutex_lock(&mutex_arrays);
  // Store terminal information in the arrays
  buffers[count] = buffer; 
  semaphores[count] = sem;
  terminal_mutexes[count] = mutex;
  pids[count] = pid;
  
  // Increment count
  count++;
  pthread_mutex_unlock(&mutex_arrays);
  
  return 0;
}

int remove_terminal(int pid){
  size_t i;
  int found = 0;
  
  if(count <= 0) return -1;
  
  // Searches for the terminal associated with pid
  for(i = 0;i < count;++i){
    if(pids[i] == pid){
       found = 1;
       break;
    }
  }
  
  if(found == 0) return -1;
  
  pthread_mutex_lock(&mutex_arrays);
  // Overwrite positions
  for(size_t j = i;j < count-1;++j){
    buffers[j] = buffers[j+1]; 
    semaphores[j] = semaphores[j+1];
    terminal_mutexes[j] = terminal_mutexes[j+1];
    pids[j] = pids[j+1];
  }
  
  // Decrement count
  count--;
  pthread_mutex_unlock(&mutex_arrays);
  
  return 0;
}

size_t get_terminal_count(){return count;}

void close_communication_unit(){
  listening = 0;
  pthread_mutex_destroy(&mutex_send);
  pthread_mutex_destroy(&mutex_arrays);
  pthread_mutex_destroy(&mutex_ack);
  close_serial_port(fd);
}


