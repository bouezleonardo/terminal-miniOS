#include "communication_unit.h"

int main() {  
  pthread_t receiver;
  int ret_code;
  
  ret_code = init_communication_unit();
  
  if(ret_code < 0) return -1;
  
  while(1){}
  
  close_communication_unit(); 
  return 0;  
}  
