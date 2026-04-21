#include "serial_communication.h"

int open_serial_port(const char *port_name){
  // Opens serial port in read-write mode
  // O_RDWR: read-write mode
  // O_NOCTTY: prevents port from becoming the controlling terminal
  int fd = open(port_name, O_RDWR | O_NOCTTY);
  
  if(fd == -1){
    perror("Failed to open serial port");
    return -1;
  }
  return fd;
}

int configure_serial_port(int fd, speed_t baud_rate){
  // Terminal for controlling communications with the port
  struct termios tty;
  
  // Get terminal attributes
  if(tcgetattr(fd, &tty) != 0){
    perror("Failed to get terminal attributes");
    return -1;
  }
  
  // Set baud rate for I/O
  cfsetospeed(&tty, baud_rate);
  cfsetispeed(&tty, baud_rate);
  
  tty.c_cflag &= ~PARENB; // No parity
  tty.c_cflag &= ~CSTOPB; // 1 bit stop (signals end of caracter)
  tty.c_cflag &= ~CSIZE;  // Clear size bits
  tty.c_cflag |= CS8;     // 8 data bits
  
  // Disable hardware flow controls (no RTS/CTS)
  tty.c_cflag &= ~CRTSCTS;
  
  // Enable receiver and ignora modem control lines
  tty.c_cflag |= (CLOCAL | CREAD);
  
  // Canonical mode: read ASCII line by line
  tty.c_cflag |= ICANON;  // Enable canonical mode
  tty.c_cflag &= ~ECHO;   // Disable echo (do not send input back)
  
  // Set read timeout (VMIN = 0, VTIME = 10 = 1 second)
  tty.c_cc[VMIN] = 0;     // Read returns immediately  
  tty.c_cc[VTIME] = 10;   // Wait 10 deciseconds (1s) for data
  
  // Apply settings immediately
  if(tcsetattr(fd, TCSANOW, &tty) != 0){
    perror("Failed to configure terminal immediately");
    return -1;
  }
  
  // Flush pending data
  tcflush(fd, TCIOFLUSH);
  
  return 0;
}

int send_ascii_command(int fd, const char *command){
  ssize_t bytes_written = write(fd, command, strlen(command));
  if(bytes_written == -1){
    perror("Failed to write to serial port");
    return -1;
  }
  return 0;
}

int read_ascii_response(int fd, char *buffer, size_t buffer_size){
  ssize_t bytes_read = read(fd, buffer, buffer_size - 1); // -1 for \0
  
  if(bytes_read == -1){
    perror("Failed to read from serial port");
    return -1;
  }
  if(bytes_read == 0) return 0;

  buffer[bytes_read] = '\0'; // Last data position
  //printf("\n");
  return bytes_read;  
}

void close_serial_port(int fd){
  close(fd);
}
