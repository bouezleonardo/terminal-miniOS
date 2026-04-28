#ifndef __MAIN_H__
#define __MAIN_H__

Message* get_terminal_message(int pid);

int request_new_terminal(int pid);

int request_destroy_terminal(int pid);

int request_tab_switch(int tab);

#endif
