/**
   file Tools/send_slack_notification.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fstream>
#include "dvccode/Utils/slack_utils.hpp"

// This exec posts a message present in a file to a given slack channel
int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << " USAGE : <exec> <slack-channel-name> <FILE/DATA> "
                 "<file-name/data>\n";
    exit(-1);
  }

  std::string slack_channel = argv[1];  // Name of the slack channel
  bool is_using_file_or_data = (std::string("FILE") == std::string(argv[2]));
  std::string file_name = argv[3];

  if (true == is_using_file_or_data) {
    std::ifstream fin(file_name);
    std::string message;

    // Loop over the lines in the file and prepare the slack message to be sent
    std::stringstream temp_ss;

    while (fin.good()) {
      getline(fin, message);
      if (message.length() <= 0) continue;

      size_t pos = 0;
      while ((pos = message.find("\"", pos)) != std::string::npos) {
        message.replace(pos, 1, "\\\"");
        pos += 2;  // since we have added 1 additional character '\'
      }
      temp_ss << message << "\\n";
    }

    if (temp_ss.str().length() > 0) {
      HFSAT::SlackManager slack_manager(slack_channel);
      slack_manager.sendNotification(temp_ss.str());
      std::cout << "Slack notification sent\n";
    } else {
      std::cout << "No message to send\n";
    }

  } else {
    std::string data = "";

    for (int32_t data_counter = 3; data_counter < argc; data_counter++) {
      data += std::string(argv[data_counter]);
    }

    HFSAT::SlackManager slack_manager(slack_channel);
    slack_manager.sendNotification(data);
  }

  return 0;
}
