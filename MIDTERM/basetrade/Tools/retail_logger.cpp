// =====================================================================================
//
//       Filename:  retail_logger.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Friday 16 May 2014 12:49:28  IST
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

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include <boost/filesystem.hpp>

#include <map>
#include <stdlib.h>

typedef std::map<std::string, std::ofstream *> secfilemap;

// making this global to pass it to termination handler
secfilemap sec_to_outfile_map_;

void ParseCommandLineParams(const int argc, const char **argv, int &_date_, std::string &_infile_,
                            std::string &_outfile_) {
  if (argc < 4) {
    std::cerr << "USAGE: " << argv[0] << " date_YYYYMMDD infile outdir\n";
    exit(0);
  } else {
    _date_ = atoi(argv[1]);

    if (_date_ < 20100101 || _date_ > 20201231) {
      std::cerr << "Enter valid date_YYYYMMDD(between 20100101 and 20201231)\n";
    }

    _infile_ = argv[2];
    _outfile_ = argv[3];
  }
}

void cleanup(secfilemap &_sec_to_outfile_map_) {
  for (secfilemap::iterator it = _sec_to_outfile_map_.begin(); it != _sec_to_outfile_map_.end(); it++) {
    it->second->close();
    delete it->second;
  }
}

void termination_handler(int signum) { cleanup(sec_to_outfile_map_); }

int main(int argc, char **argv) {
  int date_ = 0;
  std::string infile_ = "";
  std::string outdir_ = "";

  ParseCommandLineParams(argc, (const char **)argv, date_, infile_, outdir_);

  std::ostringstream t_date_stream_;
  t_date_stream_ << date_;
  std::string c_date_ = t_date_stream_.str();

  std::string outfile_dir_ =
      outdir_ + "/" + c_date_.substr(0, 4) + "/" + c_date_.substr(4, 2) + "/" + c_date_.substr(6, 2) + "/";

  boost::filesystem::create_directories(outfile_dir_);
  // HFSAT::FileUtils::Mkdir(outfile_dir_);

  std::ifstream t_infile_stream_;
  t_infile_stream_.open(infile_.c_str(), std::ios::in);

  if (t_infile_stream_.is_open()) {
    const int kbufferlength = 1024;
    char readlinebuffer_[kbufferlength];

    RETAIL_MDS::RETAILCommonStruct next_event_;
    const int kstructlength = sizeof(next_event_);
    char struct_buffer_[kstructlength];

    while (t_infile_stream_.good()) {
      bzero(readlinebuffer_, kbufferlength);
      t_infile_stream_.getline(readlinebuffer_, kbufferlength);

      HFSAT::PerishableStringTokenizer st_(readlinebuffer_, kbufferlength);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 5) {
        next_event_.msg_ = RETAIL_MDS::RETAIL_TRADE;

        char t_timestamp_[17];
        strcpy(t_timestamp_, tokens_[0]);
        next_event_.time_.tv_sec = atoi(strtok(t_timestamp_, "."));
        next_event_.time_.tv_usec = atoi(strtok(NULL, "."));

        strcpy(next_event_.data_.retail_trds_.contract_, tokens_[1]);
        next_event_.data_.retail_trds_.trd_qty_ = atoi(tokens_[2]);
        next_event_.data_.retail_trds_.agg_side_ = tokens_[3][0];
        next_event_.data_.retail_trds_.trd_px_ = atof(tokens_[4]);
        if (tokens_.size() >= 6) {
          next_event_.data_.retail_trds_.quoted_qty_ = atoi(tokens_[5]);
        } else {
          next_event_.data_.retail_trds_.quoted_qty_ = next_event_.data_.retail_trds_.trd_qty_;
        }
        next_event_.data_.retail_trds_.requested_qty_ =
            next_event_.data_.retail_trds_.trd_qty_;                      // currently no partial execution
        next_event_.data_.retail_trds_.trd_type_ = RETAIL_MDS::ACCEPTED;  // currently only accepted trades are logged

        // std::cout << next_event_.data_.retail_trds_.contract_ << " " << next_event_.ToString() << std::endl;

        if (sec_to_outfile_map_.find(next_event_.data_.retail_trds_.contract_) == sec_to_outfile_map_.end()) {
          std::ofstream *t_outfile_ = new std::ofstream();
          std::string t_filename_ =
              outfile_dir_ + std::string(next_event_.data_.retail_trds_.contract_) + "_" + c_date_;

          t_outfile_->open(t_filename_.c_str(), std::ios::out | std::ios::binary);
          sec_to_outfile_map_[next_event_.data_.retail_trds_.contract_] = t_outfile_;
        }

        std::ofstream *t_outfile_ = sec_to_outfile_map_[next_event_.data_.retail_trds_.contract_];
        if (t_outfile_->is_open() && t_outfile_->good()) {
          bzero(struct_buffer_, kstructlength);
          memcpy(struct_buffer_, &next_event_, kstructlength);
          t_outfile_->write((char *)(&next_event_), kstructlength);
          t_outfile_->flush();

          // std::cout << next_event_.ToString() <<std::endl;
        } else {
          std::cerr << "ofstream not good for sec: " << next_event_.data_.retail_trds_.contract_ << std::endl;
        }
      }
    }
    cleanup(sec_to_outfile_map_);
  }
}
