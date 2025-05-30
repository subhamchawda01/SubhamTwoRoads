#pragma once

#include <unordered_map>
#include <cstring>
#include <math.h>
#include <iterator>
#include <time.h>
#include <exanic/exanic.h>
#include <exanic/fifo_rx.h>
#include "dvccode/Utils/rdtsc_timer.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/shm_writer.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define NSE_FPGA2_DEBUG_MODE 0
#define CCPROFILING_NSEFPGA2 1
#define MAX_NSE_DATA_BUFFER 65536
#define MAXEVENTS 1024
#define NSE_TBT_DATA_START_OFFSET 0

// MESSAGE TYPES
// HEADER RELATED INFO
#define NSE_TBT_DATA_HEADER_MSGLENGTH_OFFSET NSE_TBT_DATA_START_OFFSET
#define NSE_TBT_DATA_HEADER_MSGLENGTH_LENGTH sizeof(int16_t)

#define NSE_TBT_DATA_HEADER_STREAMID_OFFSET \
  (NSE_TBT_DATA_HEADER_MSGLENGTH_OFFSET + NSE_TBT_DATA_HEADER_MSGLENGTH_LENGTH)
#define NSE_TBT_DATA_HEADER_STREAMID_LENGTH sizeof(int16_t)

#define NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET \
  (NSE_TBT_DATA_HEADER_STREAMID_OFFSET + NSE_TBT_DATA_HEADER_STREAMID_LENGTH)
#define NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_LENGTH sizeof(int32_t)

namespace NSE_Exanic {

class ExanicReader {
 public:

  ExanicReader(std::string interface_, std::string port_)
    : mkt_cstr(new FPGA_MDS::MktUpdateStruct()),
      mdsLogger("DATA_DUMP"),
      mode_(HFSAT::kLogger),
      shm_writer_(nullptr),
      socket_fd_to_multicast_receiver_sockets_(),
      clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
      using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()){
          
	  std::cout << "Inside constructor " << std::endl;
          for (int32_t ctr = 0; ctr < NSE_MAX_FD_SUPPORTED; ctr++) {
                    socket_fd_to_multicast_receiver_sockets_[ctr] = NULL;
          }

          std::ostringstream host_nse_tbt_filename;
          host_nse_tbt_filename << "/home/pengine/prod/live_configs/sdv-ind-srv21_nse-tbt-mcast.txt";
          std::string nse_tbt_filename = host_nse_tbt_filename.str();
          std::ifstream nse_tbt_channels_file;
          nse_tbt_channels_file.open(nse_tbt_filename.c_str());

          if (!nse_tbt_channels_file.is_open()) {
                    std::cout << "Failed To Load The TBT Multicast File : " << nse_tbt_filename
                                       << std::endl;
                    exit(-1);
          }
#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024         

          char buffer[1024];
          while (nse_tbt_channels_file.good()) {
                nse_tbt_channels_file.getline(buffer, MAX_LINE_SIZE);
                std::string line_buffer = buffer;

                // Comments
                HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
                std::vector<char const*> const& tokens = pst.GetTokens();

                // We expect to read StreamId, StreamIP, StreamPort
                if (tokens.size() != 3) continue;
                if (line_buffer.find("#") != std::string::npos) continue;
                HFSAT::MulticastReceiverSocket* new_multicast_receiver_socket = new HFSAT::MulticastReceiverSocket(
                                        tokens[1], atoi(tokens[2]),interface_);
                std::cout << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << interface_ << std::endl;
                new_multicast_receiver_socket->SetNonBlocking();
                if (new_multicast_receiver_socket->socket_file_descriptor() >= NSE_MAX_FD_SUPPORTED) {
                        std::cout << "SOMETHING HAS GONE WRONG, DIDN'T EXPECT TO SEE A SOCKET FD WITH DESCRIPTOR : "
       					<< new_multicast_receiver_socket->socket_file_descriptor()
                        << " MAX SUPPOERTED RANGE IS UPTO : " << NSE_MAX_FD_SUPPORTED << std::endl;
                        exit(-1);
                }
                socket_fd_to_multicast_receiver_sockets_[new_multicast_receiver_socket->socket_file_descriptor()] =
                    new_multicast_receiver_socket;

                std::cout << "SOCKET FD :" << new_multicast_receiver_socket->socket_file_descriptor()
                                  << " SETUP ON : " << tokens[0] << std::endl;
                std::cout << "CHANNEL : " << tokens[1] << " X " << tokens[2] << std::endl;
           }
           nse_tbt_channels_file.close();

#undef MAX_LINE_SIZE

      const char *device = "exanic0";
      exanic = exanic_acquire_handle(device);
      if (!exanic){
        fprintf(stderr, "exanic_acquire_handle: %s\n", exanic_get_last_error());
        exit(1);
      }
      int port = std::stoi(port_);
      std::cout<<"Exanic Port used 0/1 is " << port << std::endl;
      rx = exanic_acquire_rx_buffer(exanic, port, 0);
      if (!rx){
        fprintf(stderr, "exanic_acquire_rx_buffer: %s\n", exanic_get_last_error());
          exit(1);
       }

    }

 
  static ExanicReader &GetUniqueInstance(std::string interface, std::string port_) {
    static ExanicReader unique_instance(interface, port_);
    return unique_instance;
  }

 void printHexString(const char *c, int len) {
  int i;
  unsigned char buff[17];
  unsigned char *pc = (unsigned char *)c;

  if (len == 0) {
    printf("  ZERO LENGTH\n");
    return;
  }
  if (len < 0) {
    printf("  NEGATIVE LENGTH: %i\n", len);
    return;
  }

  // Process every byte in the data.
  for (i = 0; i < len; i++) {
    // Multiple of 16 means new line (with line offset).

    if ((i % 16) == 0) {
      // Just don't print ASCII for the zeroth line.
      if (i != 0) printf("  %s\n", buff);

      // Output the offset.
      printf("  %04x ", i);
    }

    // Now the hex code for the specific character.
    printf(" %02x", pc[i]);

    // And store a printable ASCII character for later.
    if ((pc[i] < 0x20) || (pc[i] > 0x7e))
      buff[i % 16] = '.';
    else
      buff[i % 16] = pc[i];
    buff[(i % 16) + 1] = '\0';
  }

  // Pad out last line if not exactly 16 characters.
  while ((i % 16) != 0) {
    printf("   ");
    i++;
  }

  // And print the final ASCII bit.
  printf("  %s\n", buff);

  printf("\n");
  fflush(stdout);
 }
  
 // Dump data to file , shm or both
  inline void FlushData() {
#if CCPROFILING_NSEFPGA2
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(4);
#endif
    switch (mode_) {
      case HFSAT::kLogger:
        mdsLogger.log(*mkt_cstr);
        break;
      case HFSAT::kProShm:
//        shm_writer_->Write(mkt_cstr);
        break;
      case HFSAT::kRaw:
//          shm_writer_->Write(mkt_cstr);
          mdsLogger.log(*mkt_cstr);
        break;
      case HFSAT::kLiveConsumer:
//        fpga_event_timeout_listener_->OnFPGAMarketEventDispatch(mkt_cstr);
//      queue_->push(*mkt_cstr);
      default:
        break;
    }
#if CCPROFILING_NSEFPGA2
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
#endif
  }
  
  void RunLive(){
        exanic_cycles32_t timestamp;

  	while (1){
            ssize_t sz = exanic_receive_frame(rx, data_buffer, MAX_NSE_DATA_BUFFER, &timestamp);
            if (sz > 0){
	    	char* msg_ptr = data_buffer;
		uint32_t msg_seq_no = *((int32_t*)((char*)(msg_ptr + 42 + 4)));
//           	std::cout<< "Got a valid frame " << sz << " Seq " << msg_seq_no <<std::endl;
		if ( sz == 84 ) continue; 
		memcpy(mkt_cstr->contract_, "NSE_TMPORE_DATA" , 13);
//		printHexString(msg_ptr,sz);
	        timespec_get(&(mkt_cstr->time), TIME_UTC);
      		mkt_cstr->seq_no_ = msg_seq_no;
      		mkt_cstr->fd_id_ = 0;
      		mkt_cstr->no_of_cycles_ = HFSAT::GetReadTimeStampCounter();
		mdsLogger.log(*mkt_cstr);
            }
    	}

	exanic_release_rx_buffer(rx);
        exanic_release_handle(exanic);
  }


bool Initialize(HFSAT::FastMdConsumerMode_t mode) {
  // Initialize the application.
  std::cout<<"InitializeFunct "<<std::endl;
  mode_ = mode;
  switch (mode_) {
    case HFSAT::kLogger:
      mdsLogger.EnableAffinity("MDS_LOGGER");
      mdsLogger.run();
      break;
    case HFSAT::kProShm:
//      shm_writer_ = new SHM::ShmWriter<FPGA_MDS::MdUpdateFpga2>(SHM_KEY_NSE_FPGA, NSE_FPGA_SHM_QUEUE_SIZE);
      break;
    // Using Raw as hybrid mode as a hack
    case HFSAT::kRaw:
//        shm_writer_ = new SHM::ShmWriter<FPGA_MDS::MdUpdateFpga2>(SHM_KEY_NSE_FPGA, NSE_FPGA_SHM_QUEUE_SIZE);
        mdsLogger.EnableAffinity("MDS_LOGGER");
        mdsLogger.run();
      break;
    case HFSAT::kLiveConsumer:
      // queue Strat read
      break;
    default:
      std::cerr << "Exiting...Invalid mode provided " << (int)mode_ << std::endl;
      exit(0);
      break;
  }
  return true;
}

 private:
//  DebugLogger& dbglogger_;
  FPGA_MDS::MktUpdateStruct* mkt_cstr;
  MDSLogger<FPGA_MDS::MktUpdateStruct> mdsLogger;
  HFSAT::FastMdConsumerMode_t mode_;
  SHM::ShmWriter<FPGA_MDS::MktUpdateStruct>* shm_writer_;
  HFSAT::MulticastReceiverSocket* socket_fd_to_multicast_receiver_sockets_[NSE_MAX_FD_SUPPORTED];
  HFSAT::ClockSource& clock_source_;
  bool using_simulated_clocksource_;
  char data_buffer[MAX_NSE_DATA_BUFFER];
  exanic_t *exanic;
  exanic_rx_t *rx;
};

}  // namespace NSE_FPGA
