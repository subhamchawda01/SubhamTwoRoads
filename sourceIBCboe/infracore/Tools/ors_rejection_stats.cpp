#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <getopt.h>
#include <map>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"
class ORSrejectReader {
 public:
  ORSrejectReader(std::string _shortcode_, int _yyyymmdd_, int _yyyymmdd1_)
      : shortcode_(_shortcode_), yyyymmdd_(_yyyymmdd_), yyyymmdd1_(_yyyymmdd1_) {}
  void MsgRcvd() {
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char *t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    int *A = (int *)calloc(13, sizeof(int));
    std::string location_ =
        HFSAT::TradingLocationUtils::GetTradingLocationName(HFSAT::TradingLocationUtils::GetTradingLocationExch(
            HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, yyyymmdd_)));
    while (yyyymmdd_ <= yyyymmdd1_) {
      std::ostringstream t_temp;
      t_temp << yyyymmdd_;
      std::string date_ = t_temp.str();
      // std::cout << t_temp.str() << std::endl;
      std::ostringstream ff;
      ff << LOGGED_DATA_PREFIX << location_ << "/" << date_.substr(0, 4) << "/" << date_.substr(4, 2) << "/"
         << date_.substr(6, 2) << "/" << t_exchange_symbol_ << "_" << yyyymmdd_;
      std::string filename_to_read = ff.str();
      // std::cout << ff.str() << std::endl;
      std::ifstream filename_stream;
      filename_stream.open(filename_to_read.c_str(), std::ifstream::in);
      if (!filename_stream.is_open()) {
        // std::cout << "No ORS data available." << std::endl;
        // exit(0);
      } else {
        while (filename_stream.good() && !(filename_stream.eof())) {
          HFSAT::GenericORSReplyStruct reply_struct;
          filename_stream.read(reinterpret_cast<char *>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));
          if (reply_struct.orr_type_ == HFSAT::kORRType_Rejc) {
            //					std::cout << "SYM: "<<reply_struct.symbol_<< " TIMESET
            //:"<<reply_struct.time_set_by_server_<<"
            // REJECTREASON: "<< HFSAT::ORSRejectionReasonStr ( HFSAT::ORSRejectionReason_t (
            // reply_struct.size_executed_
            //)) << std::endl;
            switch (HFSAT::ORSRejectionReason_t(reply_struct.size_executed_)) {
              case HFSAT::kORSOrderAllowed:
                A[0]++;
                break;
              case HFSAT::kORSRejectSecurityNotFound:
                A[1]++;
                break;
              case HFSAT::kORSRejectMarginCheckFailedOrderSizes:
                A[2]++;
                break;
              case HFSAT::kORSRejectMarginCheckFailedMaxPosition:
                A[3]++;
                break;
              case HFSAT::kORSRejectMarginCheckFailedWorstCasePosition:
                A[4]++;
                break;
              case HFSAT::kExchCancelReject:
                A[5]++;
                break;
              case HFSAT::kExchOrderReject:
                A[6]++;
                break;
              case HFSAT::kExchDataEntryOrderReject:
                A[6]++;
                break;
              case HFSAT::kORSRejectMarginCheckFailedMaxLiveOrders:
                A[7]++;
                break;
              case HFSAT::kORSRejectSelfTradeCheck:
                A[8]++;
                break;
              case HFSAT::kORSRejectThrottleLimitReached:
                A[9]++;
                break;
              case HFSAT::kORSRejectMarketClosed:
                A[10]++;
                break;
              case HFSAT::kORSRejectNewOrdersDisabled:
                A[11]++;
                break;
              default:
                A[12]++;
            }
          }
        }
      }
      std::cout << shortcode_ << " " << yyyymmdd_;
      std::cout << " SNF : " << A[1];
      std::cout << " FOC : " << A[2];
      std::cout << " FMC : " << A[3];
      std::cout << " FWC : " << A[4];
      std::cout << " CR : " << A[5];
      std::cout << " ER : " << A[6];
      std::cout << " FMLOC : " << A[7];
      std::cout << " STC : " << A[8];
      std::cout << " TR : " << A[9];
      std::cout << " MC : " << A[10];
      std::cout << " NOD : " << A[11];
      std::cout << " UNDEF : " << A[12] << std::endl;

      yyyymmdd_++;
    }
    // std::cout << "Allowed :" << A[0] << std::endl;
    std::cout << std::endl << shortcode_;
    std::cout << " SNF : " << A[1];
    std::cout << " FOC : " << A[2];
    std::cout << " FMC : " << A[3];
    std::cout << " FWC : " << A[4];
    std::cout << " CR : " << A[5];
    std::cout << " ER : " << A[6];
    std::cout << " FMLOC : " << A[7];
    std::cout << " STC : " << A[8];
    std::cout << " TR : " << A[9];
    std::cout << " MC : " << A[10];
    std::cout << " NOD : " << A[11];
    std::cout << " UNDEF : " << A[12] << std::endl;
  }

 private:
  HFSAT::GenericORSReplyStruct generic_ors_reply_struct_;
  std::string shortcode_;
  int yyyymmdd_;
  int yyyymmdd1_;
};
int main(int argc, char **argv) {
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  int yyyymmdd1_ = 0;
  if (argc < 4) {
    std::cout << "USAGE " << argv[0] << " SHORTCODE START_DATE END_DATE " << std::endl;
    exit(0);
  } else {
    shortcode_ = std::string(argv[1]);
    yyyymmdd_ = atoi(argv[2]);
    yyyymmdd1_ = atoi(argv[3]);
  }
  ORSrejectReader logger(shortcode_, yyyymmdd_, yyyymmdd1_);
  logger.MsgRcvd();
  return 0;
}
