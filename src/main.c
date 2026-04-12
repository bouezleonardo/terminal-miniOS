#include "communication_unit.h"
#include "serial_communication.h"

int main() {  
  pthread_t receiver;
  int ret_code;
  
  ret_code = init_communication_unit();
  
  if(ret_code < 0) return -1;
  
  pthread_create(&receiver, NULL, receive_data, NULL);
  
  pthread_join(receiver, NULL);
  
  close_communication_unit(); 
  return 0;  
}  
