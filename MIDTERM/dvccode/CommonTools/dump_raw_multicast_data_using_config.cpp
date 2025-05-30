// =====================================================================================
//
//       Filename:  dump_raw_multicast_data_using_config.cpp
//
//    Description:  Listenes to given config file based network sockets and dumps raw data
//
//        Version:  1.0
//        Created:  09/09/2015 06:40:15 AM
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
#include <cerrno>
#include <sys/time.h>
#include <csignal>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#define EXPECTED_NO_OF_CONFIG_PARAMS 3
#define RAWDATA_OUTPUT_DIR "/spare/local/RawData/"

#define MAX_UDP_BUFFER_SIZE 8192

std::map<int32_t, HFSAT::MulticastReceiverSocket *> socket_fd_to_mcast_socket_map;
HFSAT::SimpleLiveDispatcher *global_event_dispatch = NULL;

class RawDataListener : public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::BulkFileWriter bulk_file_writer_;
  struct timeval current_time_;

 public:
  // File is opened for open mode only, as we also attaching the timestamp at which it started
  RawDataListener(std::string dumpfilename)
      : bulk_file_writer_(),
        current_time_()

  {
    // get current time to append to filename
    gettimeofday(&current_time_, NULL);

    // Modify filename
    std::ostringstream filename;
    filename << RAWDATA_OUTPUT_DIR << dumpfilename << "_" << current_time_.tv_sec;

    bulk_file_writer_.Open(filename.str().c_str());
    if (!bulk_file_writer_.is_open()) {
      std::cerr << "Failed To Open File : " << filename.str()
                << " For Writing RawDump, System Error : " << strerror(errno) << std::endl;
      exit(-1);
    }
  }

  void ProcessAllEvents(int32_t socket_fd) final {
    if (socket_fd_to_mcast_socket_map.end() == socket_fd_to_mcast_socket_map.find(socket_fd)) {
      std::cerr << " Error Occured While Searching For MulticastSocket From Socket FD : " << socket_fd << std::endl;
      std::cerr << " List Of Available SocketFds :";

      for (auto &itr : socket_fd_to_mcast_socket_map) {
        std::cerr << " " << itr.first;
      }

      std::cerr << std::endl;
      exit(-1);
    }

    char buffer[MAX_UDP_BUFFER_SIZE];

    while (true) {
      // TODO - currently not handling speacial cases like partial buffer filled etc
      int32_t read_length = socket_fd_to_mcast_socket_map[socket_fd]->ReadN(MAX_UDP_BUFFER_SIZE, buffer);
      if (read_length <= 0) break;

      char socket_ip_addr[16];
      memset((void *)socket_ip_addr, 0, 16);
      memcpy((void *)socket_ip_addr, socket_fd_to_mcast_socket_map[socket_fd]->socket_ip_addr(),
             strlen(socket_fd_to_mcast_socket_map[socket_fd]->socket_ip_addr()));

      int32_t socket_port = socket_fd_to_mcast_socket_map[socket_fd]->socket_port();

      gettimeofday(&current_time_, NULL);

      // Metadata - Can be moved to a struct
      bulk_file_writer_.Write((void *)socket_ip_addr, 16);
      bulk_file_writer_.Write((void *)&socket_port, sizeof(int32_t));
      bulk_file_writer_.Write((void *)&read_length, sizeof(int32_t));
      bulk_file_writer_.Write((void *)&current_time_, sizeof(struct timeval));

      // Actual Content
      bulk_file_writer_.Write((void *)buffer, read_length);
      bulk_file_writer_.DumpCurrentBuffer();
    }
  }
};

RawDataListener *global_raw_data_listener = NULL;

void termination_handler(int32_t signal) {
  global_event_dispatch->CleanUp();

  // To allow in-processing sockets for cleanup
  sleep(1);

  for (auto &itr : socket_fd_to_mcast_socket_map) {
    if (NULL != itr.second) {
      delete itr.second;
      itr.second = NULL;
    }
  }

  if (NULL != global_raw_data_listener) {
    global_raw_data_listener->CleanUp();
    global_raw_data_listener = NULL;
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGPIPE, SIG_IGN);

  // Check to have config file
  if (argc < 2) {
    std::cerr << " Usage : " << argv[0] << " < config-file > " << std::endl;
    std::cerr << "============== Config File Template ==============" << std::endl;
    std::cerr << "<Multicast-IP> <Multicast-Port> <Interface>" << std::endl;
    exit(-1);
  }

  // Process config file and create sockets etc
  std::ifstream input_config_file_stream;
  input_config_file_stream.open(argv[1], std::ifstream::in);

  if (!input_config_file_stream.is_open()) {
    std::cerr << "Failed To Open Config File For Reading : " << argv[1] << " System Error : " << strerror(errno)
              << std::endl;
    exit(-1);
  }

  std::string raw_dump_file_name = argv[1];
  if (std::string::npos != raw_dump_file_name.find("/")) {
    raw_dump_file_name = raw_dump_file_name.substr(raw_dump_file_name.find_last_of("/") + 1);
  }

  std::string exec_name = argv[0];
  if (std::string::npos != exec_name.find("/")) {
    exec_name = exec_name.substr(exec_name.find_last_of("/") + 1);
  }

  HFSAT::SimpleLiveDispatcher event_dispatcher;

  // For signal handling
  global_event_dispatch = &event_dispatcher;
  RawDataListener raw_data_listener((exec_name + std::string("_") + raw_dump_file_name).c_str());
  global_raw_data_listener = &raw_data_listener;

#define MAX_LINE_LENGTH 1024

  char buffer[MAX_LINE_LENGTH];
  while (input_config_file_stream.good()) {
    input_config_file_stream.getline(buffer, MAX_LINE_LENGTH);

    // Linux comments
    if (std::string::npos != std::string(buffer).find("#")) continue;

    HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_LENGTH);
    std::vector<const char *> const &tokens = pst.GetTokens();

    if (EXPECTED_NO_OF_CONFIG_PARAMS != tokens.size()) continue;
    HFSAT::MulticastReceiverSocket *new_multicast_receiver_socket =
        new HFSAT::MulticastReceiverSocket(tokens[0], atoi(tokens[1]), tokens[2]);

    if (-1 == new_multicast_receiver_socket->socket_file_descriptor()) {
      std::cerr << "Failed To Open Multicast Receiver Socket For : " << tokens[0] << " X " << tokens[1]
                << " On : " << tokens[2] << " SystemError : " << strerror(errno) << std::endl;
      exit(-1);
    }

    socket_fd_to_mcast_socket_map[new_multicast_receiver_socket->socket_file_descriptor()] =
        new_multicast_receiver_socket;

    // So that we can loop over the socket and consume all available data
    new_multicast_receiver_socket->SetNonBlocking();
    event_dispatcher.AddSimpleExternalDataLiveListenerSocket(&raw_data_listener,
                                                             new_multicast_receiver_socket->socket_file_descriptor());
  }
#undef MAX_LINE_LENGTH

  event_dispatcher.RunLive();

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
