/**
   \file Tools/get_latency_stats.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717

*/
#include <iostream>
#include <stdlib.h>
#include <math.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &input_date_,
                            std::string &primary_location_, std::string &alternate_location_,
                            int &begin_secs_from_midnight_, int &end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0]
              << " shortcode input_date_YYYYMMDD pri_loc sec_loc [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
    primary_location_ = argv[3];
    alternate_location_ = argv[4];

    if (argc > 5) {
      begin_secs_from_midnight_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
    }
    if (argc > 6) {
      end_secs_from_midnight_ = (atoi(argv[6]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  std::string primary_location_ = "";
  std::string alternate_location_ = "";
  bool show_intra_day_stats_ = true;  // FIXME : Parse from command line

  ParseCommandLineParams(argc, (const char **)argv, shortcode_, input_date_, primary_location_, alternate_location_,
                         begin_secs_from_midnight_, end_secs_from_midnight_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  const char *t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::TradingLocation_t primary_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationFromLOC_NAME(primary_location_);
  HFSAT::TradingLocation_t alternate_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationFromLOC_NAME(alternate_location_);

  HFSAT::BulkFileReader bulk_file_reader_primary_;
  HFSAT::BulkFileReader bulk_file_reader_alternate_;

  std::map<uint32_t, HFSAT::ttime_t> primary_instseq_to_timestamp_;
  std::map<uint32_t, HFSAT::ttime_t> alternate_instseq_to_timestamp_;

  switch (HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_)) {
    case HFSAT::kExchSourceCME: {
      std::string t_cme_filename_primary_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, primary_location_file_read_);
      std::string t_cme_filename_alternate_ = "";

      if (alternate_location_file_read_ == HFSAT::kTLocNY4) {  // For custom runs only. TODO Remove after tests.
        t_cme_filename_alternate_ = "/home/sghosh/CME/ESH2_20120111";
      } else {
        t_cme_filename_alternate_ =
            HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, alternate_location_file_read_);
      }

      std::cerr << " Primary : " << t_cme_filename_primary_ << "\n";
      std::cerr << " Secondary : " << t_cme_filename_alternate_ << "\n";

      bulk_file_reader_primary_.open(t_cme_filename_primary_);
      bulk_file_reader_alternate_.open(t_cme_filename_alternate_);

      if (!bulk_file_reader_primary_.is_open() || !bulk_file_reader_alternate_.is_open()) {
        std::cerr << "Could not open primary/secondary file\n";
        exit(-1);
      }

      while (true) {
        CME_MDS::CMECommonStruct next_event_;

        size_t available_len_ = bulk_file_reader_primary_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));

        if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file
          uint32_t inset_seq_no_ = -1;

          if (next_event_.msg_ == CME_MDS::CME_TRADE)
            inset_seq_no_ = next_event_.data_.cme_trds_.seqno_;
          else if (next_event_.msg_ == CME_MDS::CME_DELTA)
            inset_seq_no_ = next_event_.data_.cme_dels_.seqno_;

          primary_instseq_to_timestamp_[inset_seq_no_] = next_event_.time_;
        } else {
          break;
        }
      }

      while (true) {
        CME_MDS::CMECommonStruct next_event_;

        size_t available_len_ = bulk_file_reader_alternate_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));

        if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file
          uint32_t inset_seq_no_ = -1;

          if (next_event_.msg_ == CME_MDS::CME_TRADE)
            inset_seq_no_ = next_event_.data_.cme_trds_.seqno_;
          else if (next_event_.msg_ == CME_MDS::CME_DELTA)
            inset_seq_no_ = next_event_.data_.cme_dels_.seqno_;

          alternate_instseq_to_timestamp_[inset_seq_no_] = next_event_.time_;
        } else {
          break;
        }
      }
    }

    default: { } break; }

  std::vector<double> msec_time_diff_vec_;

  double mean_msec_ = 0.0;
  double min_msec_ = 1000000.0, max_msec_ = -1.0;

  double min_msec_diff_ = HFSAT::TradingLocationUtils::GetMSecsBetweenTradingLocations(primary_location_file_read_,
                                                                                       alternate_location_file_read_) *
                          0.8;

  // For intra day stats.
  std::map<HFSAT::ttime_t, std::vector<double> > intraday_time_to_msec_diffs_;
  HFSAT::ttime_t last_timestamp_(0, 0);

  for (std::map<uint32_t, HFSAT::ttime_t>::iterator _itr_ = primary_instseq_to_timestamp_.begin();
       _itr_ != primary_instseq_to_timestamp_.end(); ++_itr_) {
    if (alternate_instseq_to_timestamp_.find(_itr_->first) != alternate_instseq_to_timestamp_.end()) {
      HFSAT::ttime_t &alternate_time_ = alternate_instseq_to_timestamp_[_itr_->first];
      HFSAT::ttime_t &primary_time_ = primary_instseq_to_timestamp_[_itr_->first];

      HFSAT::ttime_t diff_ = alternate_time_ - primary_time_;
      double msec_diff_ = diff_.tv_usec / 1000.0;

      if (diff_.tv_sec == 0 && diff_.tv_usec >= 0 && msec_diff_ > min_msec_diff_) {
        msec_time_diff_vec_.push_back(msec_diff_);

        mean_msec_ += msec_diff_;
        if (min_msec_ > msec_diff_) {
          min_msec_ = msec_diff_;
        }
        if (max_msec_ < msec_diff_) {
          max_msec_ = msec_diff_;
        }

        if (show_intra_day_stats_) {
          // 10 min. span for intra day stats.
          if ((primary_time_.tv_sec - last_timestamp_.tv_sec) > (10 * 60) ||
              last_timestamp_.tv_sec == 0) {  // First time or 10 mins elapsed.
            last_timestamp_ = primary_time_;
          }

          intraday_time_to_msec_diffs_[last_timestamp_].push_back(msec_diff_);
        }

        continue;
      }

      // std::cerr << "Discarded : " << diff_.tv_sec << "." << diff_.tv_usec << std::endl;
    }
  }

  std::sort(msec_time_diff_vec_.begin(), msec_time_diff_vec_.end());

  double median_msec_ = msec_time_diff_vec_[msec_time_diff_vec_.size() / 2];
  mean_msec_ /= msec_time_diff_vec_.size();

  double std_dev_ = 0.0;
  for (unsigned int diff_ = 0; diff_ < msec_time_diff_vec_.size(); ++diff_) {
    std_dev_ += (msec_time_diff_vec_[diff_] - mean_msec_) * (msec_time_diff_vec_[diff_] - mean_msec_);
  }
  std_dev_ /= msec_time_diff_vec_.size();
  std_dev_ = sqrt(std_dev_);

  std::cerr << "#Date\t\tEvents\t\tMin\t\tMax\t\tMedian\t\tMean\t\tStdDev" << std::endl;
  if (show_intra_day_stats_) {
    std::cerr << input_date_ << "\t" << msec_time_diff_vec_.size() << "\t\t" << min_msec_ << "\t\t" << max_msec_
              << "\t\t" << median_msec_ << "\t\t" << mean_msec_ << "\t\t" << std_dev_ << std::endl;
  } else {
    std::cout << input_date_ << "\t" << msec_time_diff_vec_.size() << "\t\t" << min_msec_ << "\t\t" << max_msec_
              << "\t\t" << median_msec_ << "\t\t" << mean_msec_ << "\t\t" << std_dev_ << std::endl;
  }

  if (show_intra_day_stats_) {
    std::cerr << "#Time\t\t\tEvents\t\tMin\t\tMax\t\tMedian\t\tMean\t\tStdDev" << std::endl;

    // Compute stats
    for (std::map<HFSAT::ttime_t, std::vector<double> >::iterator _itr_ = intraday_time_to_msec_diffs_.begin();
         _itr_ != intraday_time_to_msec_diffs_.end(); ++_itr_) {
      std::vector<double> &msecs_vec_ = _itr_->second;

      std::sort(msecs_vec_.begin(), msecs_vec_.end());

      double median_msecs_ = msecs_vec_[msecs_vec_.size() / 2];

      double mean_msecs_ = 0.0;
      double min_msecs_ = 1000000.0, max_msecs_ = -1.0;

      for (unsigned int diff_ = 0; diff_ < msecs_vec_.size(); ++diff_) {
        mean_msecs_ += msecs_vec_[diff_];

        if (min_msecs_ > msecs_vec_[diff_]) {
          min_msecs_ = msecs_vec_[diff_];
        }
        if (max_msecs_ < msecs_vec_[diff_]) {
          max_msecs_ = msecs_vec_[diff_];
        }
      }

      mean_msecs_ /= msecs_vec_.size();

      double std_dev_msecs_ = 0.0;
      for (unsigned int diff_ = 0; diff_ < msecs_vec_.size(); ++diff_) {
        std_dev_msecs_ += (msecs_vec_[diff_] - mean_msecs_) * (msecs_vec_[diff_] - mean_msecs_);
      }
      std_dev_msecs_ /= msecs_vec_.size();
      std_dev_msecs_ = sqrt(std_dev_msecs_);

      std::cout << _itr_->first << "\t" << msecs_vec_.size() << "\t\t" << min_msecs_ << "\t\t" << max_msecs_ << "\t\t"
                << median_msecs_ << "\t\t" << mean_msecs_ << "\t\t" << std_dev_msecs_ << std::endl;
    }
  }

  return 0;
}
