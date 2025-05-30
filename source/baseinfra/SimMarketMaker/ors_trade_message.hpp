// =====================================================================================
//
//       Filename:  ors_trade_message.hpp
//
//    Description:  Load the ors trdes file, used in simmarketmaker
//
//        Version:  1.0
//        Created:  Thursday 05 December 2013 03:32:42  GMT
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
#pragma once
#include <dirent.h>
#include <boost/filesystem.hpp>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#define ORS_TRADE_FILE_DIR "/NAS1/logs/ORSTrades"
namespace HFSAT {

class ORSTradeMessage {
 public:
  ORSTradeMessage(){};

  static std::string GetExchangeDirName(HFSAT::ExchSource_t _this_exch_source_) {
    std::string retval = "";
    switch (_this_exch_source_) {
      case kExchSourceCME:
        retval = "CME";
        break;

      case kExchSourceEUREX:
        retval = "EUREX";
        break;

      case kExchSourceBMF:

      case kExchSourceBMFEQ:
      case kExchSourceNTP:
        retval = "BMFEP";
        break;

      case kExchSourceTMX:
        retval = "TMX";
        break;

      case kExchSourceMEFF:
        retval = "MEFF";
        break;

      case kExchSourceIDEM:
        retval = "IDEM";
        break;

      case kExchSourceHONGKONG:
        retval = "HKEX";
        break;

      case kExchSourceREUTERS:
        retval = "REUTERS";
        break;

      case kExchSourceICE:
        retval = "ICE";
        break;

      case kExchSourceEBS:
        retval = "EBS";
        break;

      case kExchSourceLIFFE:
        retval = "LIFFE";
        break;

      case kExchSourceRTS:
        retval = "RTS";
        break;

      case kExchSourceMICEX_EQ:
      case kExchSourceMICEX_CR:
      case kExchSourceMICEX:
        retval = "MICEX";
        break;

      case kExchSourceLSE:
        retval = "LSE";
        break;

      case kExchSourceNASDAQ:
        retval = "NASDAQ";
        break;

      case kExchSourceBATSCHI:
        retval = "CHIX";
        break;

      case kExchSourceHYB:
        retval = "HYB";
        break;

      case kExchSourceJPY_L1:
      case kExchSourceJPY:
        retval = "OSE";
        break;

        retval = "OSE_L1";
        break;

      case kExchSourceTSE:
        retval = "TSE";
        break;

      default:
        retval = "INVALID";
        break;
    }
    return retval;
  }

  static std::map<int, int> LoadExchangeExecutedOrders(std::string _shortcode_, int _tradingdate_) {
    HFSAT::ExchSource_t this_exch_source_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(_shortcode_, _tradingdate_);
    std::string this_exchange_name_ = GetExchangeDirName(this_exch_source_);
    const char *this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_shortcode_);
    std::stringstream st_;
    st_ << _tradingdate_;
    std::string this_date_str_ = st_.str();
    std::string this_year_ = this_date_str_.substr(0, 4);
    std::string this_month_ = this_date_str_.substr(4, 2);
    std::string this_day_ = this_date_str_.substr(6, 2);
    std::map<int, int> saos_price_map_;
    saos_price_map_.clear();
    std::string this_directory_ = std::string(ORS_TRADE_FILE_DIR) + "/" + this_exchange_name_;
    if (!boost::filesystem::is_directory(this_directory_)) {
      std::cerr << "Trdes file Directory does not exist: " << this_directory_ << std::endl;
      return saos_price_map_;
    }
    DIR *dir_ = opendir(this_directory_.c_str());
    struct dirent *content_ = readdir(dir_);
    bool trade_file_found_ = false;
    char line_buffer_[512];
    while (content_ != NULL && !trade_file_found_) {
      if (content_->d_type == DT_DIR)  // find the direcotries
      {
        std::string trade_file_ = this_directory_ + "/" + std::string(content_->d_name) + "/" + this_year_ + "/" +
                                  this_month_ + "/" + this_day_ + "/" + "trades." + this_date_str_;
        if (boost::filesystem::exists(trade_file_)) {
          std::ifstream this_trade_file_(trade_file_);
          while (this_trade_file_.good()) {
            this_trade_file_.getline(line_buffer_, sizeof(line_buffer_));
            char *this_word_ = std::strtok(line_buffer_, "\001");
            char *prev_word_ = this_word_;
            if (this_word_ == NULL) break;

            if ((std::string(this_word_)).compare(std::string(this_exchange_symbol_)) == 0) {
              trade_file_found_ = true;
              while (this_word_ != NULL) {
                prev_word_ = this_word_;
                this_word_ = std::strtok(NULL, "\001");
              }
              saos_price_map_[atoi(prev_word_)] = atoi(prev_word_);
            } else {
              // this is not the correct file , read the next one
              // break;
            }
          }
        }
      }
      content_ = readdir(dir_);
    }
    closedir(dir_);
    if (!trade_file_found_) {
      std::cerr << "Could not find the ors trade file for this product\n";
    }
    return saos_price_map_;
  }
};
}
