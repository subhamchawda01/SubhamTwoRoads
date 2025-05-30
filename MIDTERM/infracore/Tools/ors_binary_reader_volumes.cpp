// @ramkris: Hope there are no silly bugs, in hu
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#define LOGGED_DATA_PREFIX "/spare/local/ORSBCAST/"
#define MAX_SIZE_TO_READ_AT_ONCE 100

std::string exch_ = "";

// This class will read from the ORSBInary Log file

class ORSBinReader {
 public:
  ORSBinReader(std::string _filedir_, std::string _shortcode_, int _yyyymmdd_, int t_start_utc_time, int t_end_utc_time)
      : filedir_(_filedir_),
        shortcode_(_shortcode_),
        yyyymmdd_(_yyyymmdd_),
        start_utc_time(t_start_utc_time),
        end_utc_time(t_end_utc_time) {}

  /// Reader
  void processMsgRecvd() {
    //    time_t time_from_yyyy_midnight = HFSAT::DateTime::GetTimeMidnightUTC ( yyyymmdd_ );
    // HFSAT::ttime_t start_base_time (time_from_yyyy_midnight, 0 );
    // HFSAT::ttime_t end_base_time ( time_from_yyyy_midnight, 0 );

    HFSAT::ttime_t start_time(HFSAT::DateTime::GetTimeUTC(yyyymmdd_, start_utc_time), 0);
    HFSAT::ttime_t end_time(HFSAT::DateTime::GetTimeUTC(yyyymmdd_, end_utc_time), 0);

    // start_base_time.addmsecs ( start_secs_from_midnight*1000);
    // end_base_time.addmsecs ( end_secs_from_midnight * 1000);

    // std::cout << "Msecs: "<< start_secs_from_midnight* 1000 << " Msecs: "<< end_secs_from_midnight*1000 << std::endl;
    // std::cout << "Start: " << start_base_time << " End: "<< end_base_time << std::endl;

    std::cout << start_time << " " << end_time << std::endl;
    std::stringstream ff;
    ff << filedir_ << "/" << shortcode_ << "_" << yyyymmdd_;
    std::string filename_to_read = ff.str();
    std::ifstream filename_stream;
    filename_stream.open(filename_to_read.c_str(), std::ifstream::in);  // read binary

    int total_volume = 0;
    if (filename_stream.is_open()) {
      while (filename_stream.good() && !(filename_stream.eof())) {
        HFSAT::GenericORSReplyStruct reply_struct;
        filename_stream.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));
        char pp[6] = {'\0'};
        sprintf(pp, "%.6f", reply_struct.price_);

        if (reply_struct.orr_type_ == HFSAT::kORRType_Exec && start_time < reply_struct.time_set_by_server_ &&
            reply_struct.time_set_by_server_ < end_time) {
          // std::cout << "Time : "<< reply_struct.time_set_by_server_ << " "<<  reply_struct.size_executed_ <<
          // std::endl;
          total_volume += reply_struct.size_executed_;
        }
      }
    }
    std::cout << " TV: " << total_volume << std::endl;
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string filedir_;
  std::string shortcode_;
  int yyyymmdd_;
  int start_utc_time;
  int end_utc_time;
};

void sighandler(int signum) { exit(0); }
int main(int argc, char** argv) {
  std::string dir = "";
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  int start_time_from_midnight = 0;
  int end_time_from_midnight = 2359;

  if (argc < 4) {
    std::cout << "Usage : DIR_OF_FILE  SHORTCODE(ZNU1 ) YYYYMMDD " << std::endl;
    std::cout << "Usage : DIR_OF_FILE  SHORTCODE(ZNU1 ) YYYYMMDD UTC_StartTime UTC_Endtime" << std::endl;
    exit(0);
  } else if (argc == 4) {
    dir = std::string(argv[1]);
    shortcode_ = std::string(argv[2]);
    yyyymmdd_ = atoi(argv[3]);
  }

  else if (argc > 4 && argc <= 6) {
    dir = std::string(argv[1]);
    shortcode_ = std::string(argv[2]);
    yyyymmdd_ = atoi(argv[3]);
    start_time_from_midnight = atoi(argv[4]);
    end_time_from_midnight = atoi(argv[5]);
    // start_secs_from_midnight =  ( atoi( argv[ 4 ] ) /100 ) * 60*60 + ( atoi( argv[4] ) % 100 ) * 60;
    // end_secs_from_midnight =  ( atoi( argv[ 5 ] ) /100 ) * 60*60 + ( atoi( argv[5] ) % 100 ) * 60;

  } else {
    std::cout << "Usage : DIR_OF_FILE  SHORTCODE(ZNU1 ) YYYYMMDD UTC_StartTime UTC_Endtime" << std::endl;
    exit(0);
  }

  ORSBinReader common_logger(dir, shortcode_, yyyymmdd_, start_time_from_midnight, end_time_from_midnight);

  common_logger.processMsgRecvd();

  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);
}
