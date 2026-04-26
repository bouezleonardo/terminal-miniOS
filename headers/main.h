#ifndef __MAIN_H__
#define __MAIN_H__

/**
* @brief Get the index where the terminal data is stored
*
* Get Get the index where the terminal data is stored
*
* @return Index of the data, -1 if there is no terminal with this PID
*/
int get_terminal_index(int pid);

Message* get_terminal_message(int index);

int request_new_terminal(int pid);

int request_destroy_terminal(int pid);

void request_tab_switch(int tab);

#endif
