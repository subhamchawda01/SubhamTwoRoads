// =====================================================================================
//
//       Filename:  nifty_market_data_thread.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/16/2015 05:57:22 PM
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
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <map>

#include "dvccode/CDef/nse_mds_defines.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/lock.hpp"
#include "infracore/NSET/nse_config_based_checks.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#define NSE_LTP_BASED_PRICE_DEVIATION 10

namespace HFSAT {
namespace NSE {

class NSEMktBook : public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::MulticastReceiverSocket* multicast_receiver_socket_;
  NSE_UDP_MDS::NSECommonStruct nse_common_struct_;
  HFSAT::Lock mutex_;
  HFSAT::NSE::NSEConfigBasedChecks& nse_config_based_checks_;

  NSEMktBook(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings settings)
      : dbglogger_(dbglogger),
        multicast_receiver_socket_(NULL),
        mutex_(),
        nse_config_based_checks_(HFSAT::NSE::NSEConfigBasedChecks::GetUniqueInstance(dbglogger_, settings))

  {
    HFSAT::NetworkAccountInfoManager network_account_info_manager;
    HFSAT::DataInfo data_info = network_account_info_manager.GetSrcDataInfo(HFSAT::kExchSourceNSE, "dummy_0");

    multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(
        data_info.bcast_ip_, data_info.bcast_port_,
        HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataLive));
  }

  NSEMktBook(NSEMktBook const& disabled_copy_constructor);

 public:
  static NSEMktBook& GetUniqueInstance(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings settings) {
    static NSEMktBook unique_instance(dbglogger, settings);
    return unique_instance;
  }

  inline int GetSocketFd() { return multicast_receiver_socket_->socket_file_descriptor(); }

  inline void ProcessAllEvents(int32_t socket_fd) {
    while (true) {
      int32_t socket_read_size =
          multicast_receiver_socket_->ReadN(sizeof(NSE_UDP_MDS::NSECommonStruct), (void*)&nse_common_struct_);

      if (socket_read_size < (int32_t)sizeof(NSE_UDP_MDS::NSECommonStruct)) break;

      mutex_.LockMutex();

      switch (nse_common_struct_.msg_) {
        case NSE_UDP_MDS::NSE_BOOK: {
          double mid_price =
              (nse_common_struct_.data.book.bid_prices[0] + nse_common_struct_.data.book.ask_prices[0]) / 2.00;
          nse_config_based_checks_.UpdataMidPriceAndStatus(nse_common_struct_.token, mid_price,
                                                           nse_common_struct_.status);

        } break;

        case NSE_UDP_MDS::NSE_TRADE: {
          nse_config_based_checks_.UpdateTradePriceAndStatus(
              nse_common_struct_.token, nse_common_struct_.data.trade.price, nse_common_struct_.status);

        } break;

        case NSE_UDP_MDS::NSE_GENERAL: {
          if (std::string(nse_common_struct_.data.general.action_code) == "MWP") {
            int32_t token = nse_common_struct_.token;
            bool is_refdata_available = true;
            NSE_UDP_MDS::NSERefData nse_ref_data =
                nse_config_based_checks_.GetNseRefDataForGivenToken(token, is_refdata_available);

            if (true == is_refdata_available) {
              nse_config_based_checks_.UpdateSecurityForMWPLAndStatus(nse_ref_data.symbol, token,
                                                                      nse_common_struct_.status);

            } else {
              DBGLOG_CLASS_FUNC_LINE_ERROR << "REFERENCE DATA NOT AVAILABLE FOR TOKEN : " << token
                                           << " WHICH HAS RECEIVED MWPL MSG" << DBGLOG_ENDL_NOFLUSH;
              DBGLOG_DUMP;
            }
          }

        } break;

        default: {
          std::cerr << "UNexpected Type Of Message Recevied From Mkt Broadcast : " << (int32_t)nse_common_struct_.msg_
                    << std::endl;

        } break;
      }

      mutex_.UnlockMutex();
    }
  }

  bool CheckForMktPriceProtection(char const* security, double order_price) { return true; }

  bool CheckForTradePriceProtection(char const* security, double order_price) { return true; }
};

class NSEMktBookThread : public HFSAT::Thread {
 private:
  NSEMktBook& nse_mkt_book_;
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  NSEMktBookThread(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings settings)
      : nse_mkt_book_(NSEMktBook::GetUniqueInstance(dbglogger, settings)),
        simple_live_dispatcher_()

  {}

  NSEMktBookThread(NSEMktBookThread const& disabled_copy_constructor);

 public:
  static NSEMktBookThread& GetUniqueInstance(HFSAT::DebugLogger& dbglogger, HFSAT::ORS::Settings settings) {
    static NSEMktBookThread unique_instance(dbglogger, settings);
    return unique_instance;
  }

  void thread_main() {
    simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(&nse_mkt_book_, nse_mkt_book_.GetSocketFd());
    simple_live_dispatcher_.RunLive();
  }
};
}
}
