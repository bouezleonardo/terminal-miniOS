#ifndef __SERIAL_COMMUNICATION_H__
#define __SERIAL_COMMUNICATION_H__

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

/**
* @brief Open serial port's file
*
* Given a port name, opens the port in read-write mode
*
* @param port_name Port name string
*
* @return file descriptor if file is opened, -1 if it is not
*/
int open_serial_port(const char *port_name);

/**
* @brief Configure the termios terminal
*
* Given a file descriptor and a baud rate, configure the termios
* terminal
*
* @param fd Port's file descriptor
* @param baud_rate Transfer speed of the communication
*
* @return 0 if the settings are applied, -1 if not
*/
int configure_serial_port(int fd, speed_t baud_rate);

/**
* @brief Write ASCII characters to the port
*
* Given a file descriptor and a command string, write
* the command to the port
*
* @param fd Port's file descriptor
* @param command ASCII command to be written
*
* @return 0 if the command is written, -1 if not
*/
int send_ascii_command(int fd, const char *command);

/**
* @brief Read ASCII characters from the port
*
* Given a file descriptor, a buffer and the buffer length
* read characters from the port
*
* @param fd Port's file descriptor
* @param buffer Buffer where the data is stored
* @param buffer_size Buffer size
*
* @return 0 if the command is written, -1 if not
*/
int read_ascii_response(int fd, char *buffer, size_t buffer_size);

/**
* @brief Close the port
*
* Given a file descriptor, close the ports's file
*
* @param fd Port's file descriptor
*
* @return void
*/
void close_serial_port(int fd);

#endif
