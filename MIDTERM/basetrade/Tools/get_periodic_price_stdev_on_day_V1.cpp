/**
   \file Tools/get_volume_on_day_utc.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717

*/

#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"

#define DELTA_TIME_IN_SECS 600

void ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_, int& period_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " shortcode input_date_YYYYMMDD  PERIOD(INSEC) [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]" << std::endl;
    exit(0);
  } else {
    //      std::cout  << "REMEBER IF GOT A CORE Usage: " << argv[0] << " shortcode input_date_YYYYMMDD  PERIOD(INSEC) [
    //      start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]" << std::endl;

    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
    period_ = atoi(argv[3]);
    if (period_ == 0) period_ = DELTA_TIME_IN_SECS;

    if (argc > 4) {
      begin_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
    if (argc > 5) {
      end_secs_from_midnight_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[5]) % 100) * 60;
    }
  }
}

/// input arguments : input_date
int main(int argc, char** argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  int period_ = 0;
  ParseCommandLineParams(argc, (const char**)argv, shortcode_, input_date_, period_, begin_secs_from_midnight_,
                         end_secs_from_midnight_);

  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::TradingLocation_t trading_location_file_read_;
  HFSAT::BulkFileReader bulk_file_reader_;

  double sum_price_ = 0.0, sum2_price_ = 0.0;
  int cnt_ = 0;

  bool is_bmf_equity_ = (shortcode_ == std::string(t_exchange_symbol_));

  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  trading_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);  // initialize to primary location

  switch (exch_src_) {
    case HFSAT::kExchSourceCME: {
      std::string t_cme_filename_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      CME_MDS::CMECommonStruct next_event_;

      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_cme_filename_);
      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.time_.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;

            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == CME_MDS::CME_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.cme_trds_.trd_px_;
                sum2_price_ += next_event_.data_.cme_trds_.trd_px_ * next_event_.data_.cme_trds_.trd_px_;
                cnt_++;
              }
            }
            if (next_event_.time_.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //			  tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }

          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceEUREX: {
      std::string t_eurex_filename_ =
          HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      EUREX_MDS::EUREXCommonStruct next_event_;

      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_eurex_filename_);

      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.time_.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;
            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.eurex_trds_.trd_px_;
                sum2_price_ += next_event_.data_.eurex_trds_.trd_px_ * next_event_.data_.eurex_trds_.trd_px_;
                cnt_++;
              }
            }
            if (next_event_.time_.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //			  tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }

          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceTMX: {
      std::string t_tmx_filename_ =
          HFSAT::TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      TMX_MDS::TMXCommonStruct next_event_;
      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_tmx_filename_);

      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.time_.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;
            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.time_.tv_sec < next_delta_time_event.tv_sec)

              {
                sum_price_ += next_event_.data_.tmx_trds_.trd_px_;
                sum2_price_ += next_event_.data_.tmx_trds_.trd_px_ * next_event_.data_.tmx_trds_.trd_px_;
                cnt_++;
              }
            }
            if (next_event_.time_.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //			  tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }
          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceBMFEQ:
    case HFSAT::kExchSourceNTP:
    case HFSAT::kExchSourceBMF: {
      std::string t_cme_filename_ = HFSAT::NTPLoggedMessageFileNamer::GetName(
          t_exchange_symbol_, input_date_, trading_location_file_read_, false, is_bmf_equity_);

      NTP_MDS::NTPCommonStruct next_event_;
      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.time_.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;
            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(NTP_MDS::NTPCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == NTP_MDS::NTP_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.time_.tv_sec < next_delta_time_event.tv_sec)

              {
                sum_price_ += next_event_.data_.ntp_trds_.trd_px_;
                sum2_price_ += next_event_.data_.ntp_trds_.trd_px_ * next_event_.data_.ntp_trds_.trd_px_;
                cnt_++;
              }
            }
            if (next_event_.time_.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //			  tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }

          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;

    case HFSAT::kExchSourceLIFFE: {
      std::string t_cme_filename_ =
          HFSAT::LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      LIFFE_MDS::LIFFECommonStruct next_event_;
      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.time_.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;
            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.time_.tv_sec < next_delta_time_event.tv_sec)

              {
                sum_price_ += next_event_.data_.liffe_trds_.trd_px_;
                sum2_price_ += next_event_.data_.liffe_trds_.trd_px_ * next_event_.data_.liffe_trds_.trd_px_;
                cnt_++;
              }
            }
            if (next_event_.time_.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //			  tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }

          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceHONGKONG: {
      std::string t_cme_filename_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      HKEX_MDS::HKEXCommonStruct next_event_;
      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.time_.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;
            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(HKEX_MDS::HKEXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.time_.tv_sec < next_delta_time_event.tv_sec)

              {
                sum_price_ += next_event_.data_.hkex_trds_.trd_px_;
                sum2_price_ += next_event_.data_.hkex_trds_.trd_px_ * next_event_.data_.hkex_trds_.trd_px_;
                cnt_++;
              }
            }
            if (next_event_.time_.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }
          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceJPY: {
      std::string t_filename_ = HFSAT::OSEPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                                   trading_location_file_read_);

      OSE_MDS::OSEPriceFeedCommonStruct next_event_;
      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_filename_);

      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.time_.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;
            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) {  // data not found in file

            if (next_event_.type_ == 2) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.time_.tv_sec < next_delta_time_event.tv_sec)

              {
                sum_price_ += next_event_.price;
                sum2_price_ += next_event_.price * next_event_.price;
                cnt_++;
              }
            }
            if (next_event_.time_.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }
          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.time_.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;

    case HFSAT::kExchSourceNSE: {
      std::string t_nse_filename_ =
          HFSAT::NSELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      // std::cerr<< "HK Product: " << t_hk_filename_ << std::endl;
      NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
      bool first_time = true;
      struct timeval next_delta_time_event;
      next_delta_time_event.tv_sec = 0;
      next_delta_time_event.tv_usec = 0;

      bulk_file_reader_.open(t_nse_filename_);

      if (!bulk_file_reader_.is_open())
        cnt_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
          if (first_time) {
            next_delta_time_event.tv_sec = next_event_.source_time.tv_sec + period_;
            next_delta_time_event.tv_usec = 0;
            // Convert it to perfect clock time
            int tval = ((next_delta_time_event.tv_sec) / 3600) * 3600;
            next_delta_time_event.tv_sec = tval;
            first_time = false;
          }

          if (available_len_ >= sizeof(NSE_MDS::NSEDotexOfflineCommonStruct)) {  // data not found in file

            if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade) {
              if (next_event_.source_time.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.source_time.tv_sec % 86400 < end_secs_from_midnight_ &&
                  next_event_.source_time.tv_sec < next_delta_time_event.tv_sec)

              {
                sum_price_ += next_event_.data.nse_dotex_trade.trade_price;
                sum2_price_ +=
                    next_event_.data.nse_dotex_trade.trade_price * next_event_.data.nse_dotex_trade.trade_price;
                cnt_++;
              }
            }
            if (next_event_.source_time.tv_sec >= next_delta_time_event.tv_sec) {
              if (cnt_ > 0) {
                //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
                double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
                std::cout << next_event_.source_time.tv_sec << " " << p_stdev_ << std::endl;
                // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
                // p_stdev_);
              }

              next_delta_time_event.tv_sec += period_;
              sum_price_ = sum2_price_ = 0.0;
              cnt_ = 0;
            }
          } else {
            if (cnt_ > 0) {
              //                          tm* time_value = gmtime (&next_delta_time_event.tv_sec);
              double p_stdev_ = sqrt(std::max(0.0, sum2_price_ / cnt_ - (sum_price_ * sum_price_) / (cnt_ * cnt_)));
              std::cout << next_event_.source_time.tv_sec << " " << p_stdev_ << std::endl;
              // printf("%06d%s%f\n",  time_value->tm_hour*10000 + time_value->tm_min*100 +time_value->tm_sec, " ",
              // p_stdev_);
            }
            break;
          }
        }
    } break;

    default: { } break; }

  //  std::cout << shortcode_ << ' ' << t_exchange_symbol_ << ' ' << cnt_ << std::endl;
}
