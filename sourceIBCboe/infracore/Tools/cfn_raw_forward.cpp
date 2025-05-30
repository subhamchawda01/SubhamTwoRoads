// =====================================================================================
//
//       Filename:  cfn_raw_forward.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  10/21/2014 12:59:24 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "dvccode/Utils/data_forwarder.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << " USAGE : < exec > < multicast-input-file ( ip port interface ) > < multicast-output-file ( ip port "
                 "interface )\n";
    exit(-1);
  }

  // A Utility Class To Send Forward the multicast data
  HFSAT::Utils::DataForwarder &data_forwarder = HFSAT::Utils::DataForwarder::GetUniqueInstance();

  std::ifstream multicast_input_file_stream;
  multicast_input_file_stream.open(
      argv[1], std::ifstream::in);  // THis is the input data file, just load up ip/port/interface from where to listen

  if (!multicast_input_file_stream.is_open()) {
    std::cerr << "Can't Open Multicast Input File For Reading : " << argv[1] << "\n";
    exit(-1);
  }

  char line_buffer[1024];
  std::string line_buffer_string = "";

  while (multicast_input_file_stream.good()) {
    memset((void *)line_buffer, 0, 1024);
    multicast_input_file_stream.getline(line_buffer, 1024);

    line_buffer_string = line_buffer;
    if (std::string::npos != line_buffer_string.find("#")) continue;

    HFSAT::PerishableStringTokenizer st(line_buffer, 1024);
    const std::vector<const char *> &tokens = st.GetTokens();

    if (tokens.size() >= 3) {
      // Add Multicast Group To The Input Subscription
      data_forwarder.AddMulticastChannelForInputSubscription(tokens[0], atoi(tokens[1]), tokens[2]);
    }
  }

  multicast_input_file_stream.close();

  multicast_input_file_stream.open(argv[2], std::ifstream::in);  // This one is the output mcast groups file

  if (!multicast_input_file_stream.is_open()) {
    std::cerr << "Can't Open Multicast Input File For Reading : " << argv[1] << "\n";
    exit(-1);
  }

  while (multicast_input_file_stream.good()) {
    memset((void *)line_buffer, 0, 1024);
    multicast_input_file_stream.getline(line_buffer, 1024);

    line_buffer_string = line_buffer;
    if (std::string::npos != line_buffer_string.find("#")) continue;

    HFSAT::PerishableStringTokenizer st(line_buffer, 1024);
    const std::vector<const char *> &tokens = st.GetTokens();

    if (tokens.size() >= 3) {
      data_forwarder.AddMulticastChannelForOutput(tokens[0], atoi(tokens[1]), tokens[2]);
    }
  }

  multicast_input_file_stream.close();

  // A wait loop here to process events indefinitely or until an error
  data_forwarder.Start();

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
