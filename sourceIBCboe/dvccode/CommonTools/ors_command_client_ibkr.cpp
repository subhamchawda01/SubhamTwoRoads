#include <iostream>
#include <stdlib.h>
#include <vector>
#include <string>
#include "dvccode/IBUtils/ControlCommandClient.hpp"
int main(int argc, char** argv) {
  std::vector<std::string> control_command_text_;

  if (argc <= 1) return 0;

  std::string control_ip = "127.0.0.1";
  int control_port = 10003;

  if (argc > 1) control_port = atoi(argv[1]);

  for (int i = 2; i < argc; i++) {
    control_command_text_.push_back(argv[i]);
  }

  if (control_command_text_.size() <= 0) return 0;

  ControlCommandClient cmdRequestShortCode(control_ip,control_port);
  std::string response="";
  cmdRequestShortCode.sendControlCommand(control_command_text_,response);
  std::cout<<"The response received is: "<<response<<"\n";
}
