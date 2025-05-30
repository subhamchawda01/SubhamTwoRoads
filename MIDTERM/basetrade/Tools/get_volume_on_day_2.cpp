/**
   \file Tools/get_volume_on_day.cpp

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
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
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

#define LOGGED_FILE_NAME_PREFIX "/spare/local/VolumesCache/"

bool ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_, std::string& filename_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
    std::ostringstream t_oss_;
    t_oss_ << LOGGED_FILE_NAME_PREFIX << "/" << shortcode_ << "_" << input_date_;
    filename_ = t_oss_.str();
    int size_ = 0;

    if (argc > 3) {
      begin_secs_from_midnight_ = (atoi(argv[3]) / 100) * 60 + atoi(argv[3]) % 100;
    }
    if (argc > 4) {
      end_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 + atoi(argv[4]) % 100;
    }
    if (!(HFSAT::FileUtils::ExistsAndReadable(filename_.c_str()) &&
          HFSAT::FileUtils::ExistsWithSize(filename_.c_str(), size_))) {
      return false;
    }
    return true;
  }
}

/// input arguments : input_date
int main(int argc, char** argv) {
  std::string shortcode_ = "";
  std::string filename_ = "";
  int input_date_ = 20110101;
  int begin_mins_from_midnight_ = 0;
  int end_mins_from_midnight_ = 24 * 60;
  bool is_file_available_ = ParseCommandLineParams(argc, (const char**)argv, shortcode_, input_date_,
                                                   begin_mins_from_midnight_, end_mins_from_midnight_, filename_);
  std::map<int, int> mins_to_vol_map_;

  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  bool is_bmf_equity_ = (shortcode_ == std::string(t_exchange_symbol_));

  if (shortcode_.find("DI1") != std::string::npos) {
    is_bmf_equity_ = false;
  }

  HFSAT::TradingLocation_t trading_location_file_read_;
  HFSAT::BulkFileReader bulk_file_reader_;

  unsigned long traded_volume_ = 0;

  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  trading_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);  // initialize to primary location

  if ((exch_src_ == HFSAT::kExchSourceHONGKONG && trading_location_file_read_ == HFSAT::kTLocHK &&
       (input_date_ >= 20141204)) ||
      ((exch_src_ == HFSAT::kExchSourceHONGKONG) && (trading_location_file_read_ == HFSAT::kTLocJPY) &&
       (input_date_ >= 20150121))) {
    exch_src_ = HFSAT::kExchSourceHKOMDCPF;
  }

  if (!is_file_available_) {
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
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.cme_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.cme_trds_.trd_qty_;
              }
            } else
              break;
          }
      } break;
      case HFSAT::kExchSourceEUREX: {
        std::string t_eurex_filename_ = "";

        if (HFSAT::UseEOBIData(trading_location_file_read_, input_date_, shortcode_)) {
          t_eurex_filename_ = HFSAT::EOBIPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                                  trading_location_file_read_);
        } else {
          t_eurex_filename_ =
              HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
        }

        EUREX_MDS::EUREXCommonStruct next_event_;
        bulk_file_reader_.open(t_eurex_filename_);

        if (!bulk_file_reader_.is_open())
          traded_volume_ = -1;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
            if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

              if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;

                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.eurex_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.eurex_trds_.trd_qty_;
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
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.tmx_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.tmx_trds_.trd_qty_;
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

              if (next_event_.msg_ == NTP_MDS::NTP_TRADE) {
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.ntp_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.ntp_trds_.trd_qty_;
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
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.liffe_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.liffe_trds_.trd_qty_;
              }
            } else
              break;
          }
      } break;
      case HFSAT::kExchSourceICE: {
        std::string t_ice_filename_ =
            HFSAT::ICELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
        ICE_MDS::ICECommonStruct next_event_;
        bulk_file_reader_.open(t_ice_filename_);
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct));
          if (available_len_ >= sizeof(ICE_MDS::ICECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == ICE_MDS::ICE_TRADE) {
              int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
              thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
              if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.ice_trds_.size_;
              else
                mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.ice_trds_.size_;
            }
          } else
            break;
        }

      } break;
      case HFSAT::kExchSourceASX: {
        std::string t_asx_filename_ = HFSAT::CommonLoggedMessageFileNamer::GetName(
            HFSAT::kExchSourceASX, t_exchange_symbol_, input_date_, trading_location_file_read_);
        ASX_MDS::ASXPFCommonStruct next_event_;
        bulk_file_reader_.open(t_asx_filename_);
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ASX_MDS::ASXPFCommonStruct));
          if (available_len_ >= sizeof(ASX_MDS::ASXPFCommonStruct)) {  // data not found in file

            if (next_event_.msg_ == ASX_MDS::ASX_PF_TRADE) {
              int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
              thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
              if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.trade_.quantity_;
              else
                mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.trade_.quantity_;
            }
          } else
            break;
        }

      } break;
      case HFSAT::kExchSourceJPY: {
        std::string t_ose_filename_ = HFSAT::OSEPriceFeedLoggedMessageFileNamer::GetName(
            t_exchange_symbol_, input_date_, trading_location_file_read_);

        OSE_MDS::OSEPriceFeedCommonStruct next_event_;
        bulk_file_reader_.open(t_ose_filename_);

        if (!bulk_file_reader_.is_open())
          traded_volume_ = -1;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
            if (available_len_ >= sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) {  // data not found in file

              if (next_event_.type_ == 2) {
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.size;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.size;
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
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.hkex_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.hkex_trds_.trd_qty_;
              }
            } else
              break;
          }
      } break;
      case HFSAT::kExchSourceHKOMDCPF:
      case HFSAT::kExchSourceHKOMDPF: {
        std::string t_hkex_filename_ = HFSAT::HKOMDCPFLoggedMessagefileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                                      trading_location_file_read_);

        HKOMD_MDS::HKOMDPFCommonStruct next_event_;
        bulk_file_reader_.open(t_hkex_filename_);

        if (!bulk_file_reader_.is_open())
          traded_volume_ = -1;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
            if (available_len_ >= sizeof(HKOMD_MDS::HKOMDPFCommonStruct)) {  // data not found in file

              if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_TRADE) {
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.trade_.quantity_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.trade_.quantity_;
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
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.rts_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.rts_trds_.trd_qty_;
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

        // int lot_size_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize ( shortcode_ , input_date_ );

        if (!bulk_file_reader_.is_open())
          traded_volume_ = -1;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(MICEX_MDS::MICEXCommonStruct));
            if (available_len_ >= sizeof(MICEX_MDS::MICEXCommonStruct)) {  // data not found in file

              if (next_event_.msg_ == MICEX_MDS::MICEX_TRADE) {
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.micex_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.micex_trds_.trd_qty_;
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
                int thirty_min_bucket_ = (next_event_.time_.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data_.csm_trds_.trd_qty_;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data_.csm_trds_.trd_qty_;
              }
            } else
              break;
          }
      } break;

      case HFSAT::kExchSourceNSE: {
        std::string t_nse_filename_ =
            HFSAT::NSELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

        NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
        bulk_file_reader_.open(t_nse_filename_);

        if (!bulk_file_reader_.is_open())
          traded_volume_ = -1;
        else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
            if (available_len_ >= sizeof(NSE_MDS::NSEDotexOfflineCommonStruct)) {  // data not found in file
              if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade) {
                int thirty_min_bucket_ = (next_event_.source_time.tv_sec % 86400) / 60;
                thirty_min_bucket_ = thirty_min_bucket_ - thirty_min_bucket_ % 30;
                if (mins_to_vol_map_.find(thirty_min_bucket_) != mins_to_vol_map_.end())
                  mins_to_vol_map_[thirty_min_bucket_] += next_event_.data.nse_dotex_trade.trade_quantity;
                else
                  mins_to_vol_map_[thirty_min_bucket_] = next_event_.data.nse_dotex_trade.trade_quantity;
              }

            } else
              break;
          }
      } break;

      default: { } break; }
  } else {
    std::ifstream precomputed_vols_;
    precomputed_vols_.open(filename_.c_str(), std::ifstream::in);
    if (!precomputed_vols_.is_open()) {
      std::cerr << " Could not open precomputed volume file = " << filename_ << std::endl;
    }
    char line_[1024];
    memset(line_, 0, sizeof(line_));

    while (precomputed_vols_.getline(line_, sizeof(line_))) {
      HFSAT::PerishableStringTokenizer st_(line_, 1024);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if (tokens_.size() == 2) {
        int thirty_min_bucket_ = atoi(tokens_[0]);
        mins_to_vol_map_[thirty_min_bucket_] = atoi(tokens_[1]);
        if (begin_mins_from_midnight_ <= thirty_min_bucket_ && thirty_min_bucket_ < end_mins_from_midnight_)
          traded_volume_ += mins_to_vol_map_[thirty_min_bucket_];
        if (thirty_min_bucket_ >= end_mins_from_midnight_) break;
      }
    }
    precomputed_vols_.close();
  }

  if (!is_file_available_) {
    std::ofstream precomputed_vols_;
    precomputed_vols_.open(filename_.c_str(), std::ifstream::out);
    for (std::map<int, int>::iterator it = mins_to_vol_map_.begin(); it != mins_to_vol_map_.end(); it++) {
      int thirty_min_bucket_ = it->first;
      if (begin_mins_from_midnight_ <= thirty_min_bucket_ && thirty_min_bucket_ < end_mins_from_midnight_)
        traded_volume_ += mins_to_vol_map_[thirty_min_bucket_];
      precomputed_vols_ << thirty_min_bucket_ << " " << it->second;
      precomputed_vols_ << std::endl;
    }
    precomputed_vols_.close();
  }
  char print_secname_[24] = {0};
  strcpy(print_secname_, t_exchange_symbol_);
  for (size_t i = 0; i < strlen(print_secname_); ++i) {
    if (print_secname_[i] == ' ') {  // Liffe symbols have space, which is bad for the post processing script
      print_secname_[i] = '~';
    }
  }

  std::cout << shortcode_ << ' ' << print_secname_ << ' ' << traded_volume_ << std::endl;
}
