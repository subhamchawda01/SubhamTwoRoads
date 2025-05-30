// =====================================================================================
//
//       Filename:  get_avg_volume_for_shortcode.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/26/2012 09:53:42 AM
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
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filenamer.hpp"

#define LOGGED_DATA_PREFIX "/spare/local/VolumeBasedSymbol/"
#define DAYS_TO_LOOK_BACK 10

unsigned int GetTotalVolumeForDate(std::string _shortcode_, int _date_) {
  int traded_volume_ = 0;
  int input_date_ = _date_;
  std::string shortcode_ = _shortcode_;

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  const char *t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
  bool is_bmf_equity_ = (shortcode_ == std::string(t_exchange_symbol_));

  if (shortcode_.find("DI1") != std::string::npos) {
    is_bmf_equity_ = false;
  }

  HFSAT::TradingLocation_t trading_location_file_read_;
  HFSAT::BulkFileReader bulk_file_reader_;

  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  trading_location_file_read_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);

  switch (exch_src_) {
    case HFSAT::kExchSourceCME: {
      std::string t_cme_filename_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      CME_MDS::CMECommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
          if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == CME_MDS::CME_TRADE) {
              traded_volume_ += next_event_.data_.cme_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceEUREX: {
      std::string t_eurex_filename_ =
          HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      EUREX_MDS::EUREXCommonStruct next_event_;
      bulk_file_reader_.open(t_eurex_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
          if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
              traded_volume_ += next_event_.data_.eurex_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceTMX: {
      std::string t_tmx_filename_ =
          HFSAT::TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      TMX_MDS::TMXCommonStruct next_event_;
      bulk_file_reader_.open(t_tmx_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
          if (available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
              traded_volume_ += next_event_.data_.tmx_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceBMFEQ:
    case HFSAT::kExchSourceBMF: {
      std::string t_cme_filename_ = HFSAT::NTPLoggedMessageFileNamer::GetName(
          t_exchange_symbol_, input_date_, trading_location_file_read_, false, is_bmf_equity_);

      NTP_MDS::NTPCommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
          if (available_len_ >= sizeof(NTP_MDS::NTPCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == NTP_MDS::NTP_TRADE && next_event_.data_.ntp_trds_.flags_[0] != 'X') {
              traded_volume_ += next_event_.data_.ntp_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceLIFFE: {
      std::string t_liffe_filename_ =
          HFSAT::LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      LIFFE_MDS::LIFFECommonStruct next_event_;
      bulk_file_reader_.open(t_liffe_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
          if (available_len_ >= sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
              traded_volume_ += next_event_.data_.liffe_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceJPY: {
      std::string t_ose_filename_ =
          HFSAT::OSELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      OSE_MDS::OSECommonStruct next_event_;
      bulk_file_reader_.open(t_ose_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSECommonStruct));
          if (available_len_ >= sizeof(OSE_MDS::OSECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == OSE_MDS::OSE_TRADE) {
              traded_volume_ += next_event_.data_.ose_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceHONGKONG: {
      std::string t_hkex_filename_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      HKEX_MDS::HKEXCommonStruct next_event_;
      bulk_file_reader_.open(t_hkex_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
          if (available_len_ >= sizeof(HKEX_MDS::HKEXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
              traded_volume_ += next_event_.data_.hkex_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;

    case HFSAT::kExchSourceRTS: {
      std::string t_rts_filename_ =
          HFSAT::RTSLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      RTS_MDS::RTSCommonStruct next_event_;
      bulk_file_reader_.open(t_rts_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
          if (available_len_ >= sizeof(RTS_MDS::RTSCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == RTS_MDS::RTS_TRADE) {
              traded_volume_ += next_event_.data_.rts_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceMICEX:
    case HFSAT::kExchSourceMICEX_EQ:
    case HFSAT::kExchSourceMICEX_CR: {
      std::string t_micex_filename_ =
          HFSAT::MICEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      MICEX_MDS::MICEXCommonStruct next_event_;
      bulk_file_reader_.open(t_micex_filename_);

      int lot_size_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcode_, input_date_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(MICEX_MDS::MICEXCommonStruct));
          if (available_len_ >= sizeof(MICEX_MDS::MICEXCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == MICEX_MDS::MICEX_TRADE) {
              traded_volume_ += (next_event_.data_.micex_trds_.trd_qty_ * lot_size_);
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceCFE: {
      std::string t_cfe_filename_ =
          HFSAT::CFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      CSM_MDS::CSMCommonStruct next_event_;
      bulk_file_reader_.open(t_cfe_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
          if (available_len_ >= sizeof(CSM_MDS::CSMCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == CSM_MDS::CSM_TRADE &&
                (shortcode_.substr(0, 3) == "SP_" || next_event_.data_.csm_trds_.trade_condition[0] != 'S')) {
              traded_volume_ += next_event_.data_.csm_trds_.trd_qty_;
            }
          } else
            break;
        }
    } break;
    case HFSAT::kExchSourceNSE: {
      std::string t_nse_filename_ =
          HFSAT::NSELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      std::cerr << t_nse_filename_ << "\n";
      NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
      bulk_file_reader_.open(t_nse_filename_);

      if (!bulk_file_reader_.is_open())
        traded_volume_ = -1;
      else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
          if (available_len_ >= sizeof(NSE_MDS::NSEDotexOfflineCommonStruct)) {  // data not found in file

            if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade) {
              traded_volume_ += next_event_.data.nse_dotex_trade.trade_quantity;
              std::cerr << traded_volume_ << "\n";
            }
          } else
            break;
        }
    } break;
    default: { } break; }
  return traded_volume_;
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << " Usage : < exec > < shortcode > < date > < number of days lookback > \n";
    exit(-1);
  }

  std::string shortcode_ = argv[1];
  int yyyymmdd_ = atoi(argv[2]);
  int num_days_lookback_for_avg_ = atoi(argv[3]);

  int max_days_loopback_attempts_limit_ = 2 * num_days_lookback_for_avg_;
  int max_days_loopback_counter_ = 0;

  unsigned int total_volume_ = 0;
  bool found_shortcode_in_volumefile_ = false;
  bool found_today_volume_ = false;

  int this_day_counter_ = 0;

  int yyyymmdd_volume_ = 0;

  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd_).LoadNSESecurityDefinitions();
  }
  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_);

  std::string liffe_str_ = "";

  if (exch_src_ == HFSAT::kExchSourceLIFFE) {
    liffe_str_ = "LIFFE";
  }

  std::ostringstream today_volume_stream_;

  today_volume_stream_ << LOGGED_DATA_PREFIX << "VOSymbol_" << yyyymmdd_ << ".txt";
  std::string today_volume_filename_ = today_volume_stream_.str();

  if (!HFSAT::FileUtils::exists(today_volume_filename_)) {
    yyyymmdd_volume_ = GetTotalVolumeForDate(shortcode_, yyyymmdd_);
  } else {
    std::ifstream today_file_;
    today_file_.open(today_volume_filename_.c_str());
    if (!today_file_.is_open()) {
      yyyymmdd_volume_ = GetTotalVolumeForDate(shortcode_, yyyymmdd_);
    }
    char today_buffer_[1024];
    std::string today_line_buffer_ = "";

    while (today_file_.good()) {
      today_file_.getline(today_buffer_, 1024);
      today_line_buffer_ = today_buffer_;
      if (today_line_buffer_.find("#") != std::string::npos) {
        continue;
      }
      HFSAT::PerishableStringTokenizer st_(today_buffer_, 1024);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() < 3) {
        continue;
      }
      std::string this_shortcode_ = tokens_[1];
      if (shortcode_.find(this_shortcode_) != std::string::npos) {
        found_today_volume_ = true;
        yyyymmdd_volume_ = atoi(tokens_[2]);
        break;
      }
    }

    today_file_.close();
    if (!found_today_volume_) {
      yyyymmdd_volume_ = GetTotalVolumeForDate(shortcode_, yyyymmdd_);
    }
  }

  yyyymmdd_ = HFSAT::DateTime::CalcPrevDay(yyyymmdd_);  // start with a day prior to that day for computing averege

  while (this_day_counter_ < num_days_lookback_for_avg_ &&
         max_days_loopback_counter_ < max_days_loopback_attempts_limit_) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << LOGGED_DATA_PREFIX << liffe_str_ << "VOSymbol_" << yyyymmdd_ << ".txt";
    std::string this_file_name_ = t_temp_oss_.str();
    if (!HFSAT::FileUtils::exists(this_file_name_)) {
      yyyymmdd_ = HFSAT::DateTime::CalcPrevDay(yyyymmdd_);
      max_days_loopback_counter_++;
      continue;
    } else {
      std::ifstream volume_file_;
      volume_file_.open(this_file_name_.c_str());
      if (!volume_file_.is_open()) {
        yyyymmdd_ = HFSAT::DateTime::CalcPrevDay(yyyymmdd_);
        max_days_loopback_counter_++;
        yyyymmdd_ = HFSAT::DateTime::CalcPrevDay(yyyymmdd_);
        continue;
      }
      found_shortcode_in_volumefile_ = false;
      char buffer[1024] = {0};
      std::string line_buffer_ = "";
      while (volume_file_.good()) {
        volume_file_.getline(buffer, 1024);
        line_buffer_ = buffer;
        if (line_buffer_.find("#") != std::string::npos) {
          continue;
        }
        HFSAT::PerishableStringTokenizer st_(buffer, 1024);
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() < 3) {
          continue;
        }
        std::string this_shortcode_ = tokens_[0];
        if (shortcode_.find(this_shortcode_) != std::string::npos) {
          found_shortcode_in_volumefile_ = true;
          total_volume_ += atoi(tokens_[2]);
          this_day_counter_++;
          break;
        }
      }
      volume_file_.close();
      if (!found_shortcode_in_volumefile_) {
        max_days_loopback_counter_++;
        yyyymmdd_ = HFSAT::DateTime::CalcPrevDay(yyyymmdd_);
        continue;
      }
    }
    yyyymmdd_ = HFSAT::DateTime::CalcPrevDay(yyyymmdd_);
    max_days_loopback_counter_++;
  }

  if (this_day_counter_ == 0) {
    std::cout << yyyymmdd_volume_ << " " << 1 << "\n";
  } else {
    std::cout << yyyymmdd_volume_ << " " << total_volume_ / (this_day_counter_) << "\n";  // int avg
  }
  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
