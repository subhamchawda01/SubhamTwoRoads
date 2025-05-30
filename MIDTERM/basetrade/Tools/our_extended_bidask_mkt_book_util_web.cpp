// =====================================================================================
// 
//       Filename:  our_extended_bidask_mkt_book_util_web.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  07/12/2018 03:23:38 AM
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
// @ramkris

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <signal.h>
#include <sys/stat.h>

#include "dvccode/Utils/thread.hpp"

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#include "dvccode/ExternalData/historical_dispatcher.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"

#include "dvccode/ORSMessages/ors_message_listener.hpp"
#include "dvccode/ORSMessages/ors_message_livesource.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/OrderRouting/base_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/prom_pnl.hpp"

#include "baseinfra/MarketAdapter/market_adapter_list.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"

#include "dvccode/Utils/send_alert.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/shortcode_request_helper.hpp"

#define OEBU_MDS_ORS_TIMEOUT_CONFIG "/spare/local/files/oebu_ors_mds_timeout.cfg"

#define SECURITYMARGINFILEPATH "/spare/local/tradeinfo/NSE_Files/SecuritiesMarginFiles/"

#define _USE_EUREX_LIVE_DATA_SOURCE_ true

#define _AGGRESSIVE_ORDERS_THRESHOLD_ 10
#define _AGGRESSIVE_CHECK_TIMEOUT_ 300000
#define MARKET_DATA_MINIMUM_TIME 3000
#define ORS_MINIMUM_TIME 1000
#define LAKH_DIVISOR 100000

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[33m"
#define NC "\x1B[37m"

#define _ENABLE_PNL_ALERTS_ false

#define ORS_MSG_TIMEOUT_ 600000
#define MDS_MSG_TIMEOUT_ 300000
#define PNL_DIFF_TIME_ 300000

char *data_file= NULL;
std::map<uint32_t, std::string> sec_id_to_data;
std::map<uint32_t, double> sid_margin_factor_;
std::map<uint32_t, double> sid_margin_consumed;
std::ofstream outstream;
// Sending voice alert for sharp pnl changes
bool send_pnl_alert_ = false;
bool _USE_LIFFE_LIVE_DATA_SOURCE_ = false;
bool _USE_CME_LIVE_DATA_SOURCE_ = false;
bool _USE_HK_SHM_SOURCE_ = false;
std::map<unsigned int, int> sid_to_ors_timeout_map_;
std::map<unsigned int, int> sid_to_mds_timeout_map_;

std::string rejection_reason_string_ = "";

char final_oebu_info[1024];
char final_json_file[1024];
char temp_json_file[1024];
char temp_oebu_info[1024];

struct PnlCheckStruct {
  int pnl_check_;
  int pnl_checkout_time_;
  int recent_pnl_;
  int last_execution_time_;
};

// ================================================================
// Simple struct to contain information to be logged in binary
struct BAUpdateStruct {
  char time_string_[14];

  unsigned int sec_id_;
  char symbol_[13];
  int top_bid_size_;
  int top_bid_order_;
  double top_bid_price_;
  int int_top_bid_price_;
  int int_top_ask_price_;
  double top_ask_price_;
  int top_ask_order_;
  int top_ask_size_;

  int sid_to_best_bid_size_;
  int sid_to_best_bid_price_;
  int sid_to_best_ask_price_;
  int sid_to_best_ask_size_;

  int total_pnl_;
  int global_position_;
  int executed_;
  int traded_volume_;
  int rejected_;
  int num_ors_messages_;
};
// ================================================================

#define DEFAULT_VAL -1
class SimpleBAwithMarket : public HFSAT::SecurityMarketViewChangeListener,
                           public HFSAT::GlobalOrderChangeListener,
                           public HFSAT::GlobalOrderExecListener,
                           public HFSAT::TimePeriodListener {
 public:
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnGlobalOrderChange(const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_, const int _int_price_);
  void OnGlobalOrderExec(const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_, const int _size_,
                         const double _trade_px_);

  void OnTimePeriodUpdate(const int num_pages_to_add_);

 public:
  SimpleBAwithMarket(HFSAT::SecurityMarketView& this_svm_t, HFSAT::DebugLogger& dbglogger_t, HFSAT::Watch& watch_t,
                     HFSAT::PromOrderManager* r_p_prom_order_manager_, const unsigned int r_this_sid,
                     HFSAT::PromPNL* _prom_pnl_, HFSAT::BulkFileWriter* _p_binary_log_,
                     std::map<std::pair<std::string, std::string>, std::vector<PnlCheckStruct>>& r_pnl_check_map_)
      : this_smv_(this_svm_t),
        dbglogger_(dbglogger_t),
        watch_(watch_t),
        p_prom_order_manager_(r_p_prom_order_manager_),
        traded_volume_(0uL),
        sid_to_last_update_time_(),
        sid_to_traded_volume_10_mins_prior_(),
        sid_to_position_10_mins_prior_(),

        max_levels_(10),
        this_sid_(r_this_sid),
        prom_pnl_(_prom_pnl_),
        prev_time_(0),
        prev_time_tp_(0),
        current_exposure(0),
        current_pnl(0),
        p_binary_log_(_p_binary_log_),
        shortcode_timeperiod_to_pnl_check_(r_pnl_check_map_),
        last_rej_reason(""),
        last_rej_time(-1),
        sid_to_last_execution_time_(),
        sid_to_aggressive_buy_in_5_mins_(),
        sid_to_aggressive_sell_in_5_mins_(),
        sid_to_aggressive_orders_alert_notified_(),
        sid_to_last_ors_message_capture_time_(),
        sid_to_ors_messages_not_received_alert_(),
        sid_to_last_mds_message_capture_time_(),
        sid_to_mds_messages_not_received_alert_(),
        numbers_to_dollars_(
            HFSAT::SecurityDefinitions::GetContractNumbersToDollars(this_svm_t.shortcode(), watch_.YYYYMMDD())) {
    memset(&ba_update_struct_, 0, sizeof(ba_update_struct_));

    p_prom_order_manager_->AddGlobalOrderChangeListener(this);
    p_prom_order_manager_->AddGlobalOrderExecListener(this);
    p_prom_order_manager_->ManageOrdersAlso();
    this_svm_t.subscribe_tradeprints(this);
    watch_.subscribe_BigTimePeriod(this);
  }

  ~SimpleBAwithMarket() {
    if (p_binary_log_) {
      p_binary_log_->Close();
      delete p_binary_log_;
    }
  }

  void DumpHeader();
  void DumpTrailer();

  void DefLoop(unsigned int this_sid_number, int _trade_px_ = 0,
               const HFSAT::TradeType_t buysell_ = HFSAT::kTradeTypeBuy);
  // Doesnot do anything
  void DoNothing() { return; }

  void SendAlertForAggressiveOrders(std::string alert_msg_) {
    HFSAT::SendAlert::sendAlert(alert_msg_);

    HFSAT::Email email_;
    email_.setSubject("Subject : Aggressive Orders Alert");
    email_.addRecepient("ravi.parikh@tworoads.co.in,sghosh@circulumvite.com");
    email_.addSender("OEBU@ny16");
    email_.content_stream << alert_msg_;

    email_.sendMail();
  }
  // // Clean maps of Bid/Ask in the prom  order manager depending on the market price
  // void CleanPromBAConfirmedMap ( );

  int last_mds_update_time = 0;
  int last_ors_update_time = 0;
  int market_data_minimum_time = 100;
  int ors_minimum_time = 0;

  // map to hold day's event in sorted order
  static std::multimap<time_t, HFSAT::EventLine> event_time_map_;
  static std::map<HFSAT::EconomicZone_t, std::vector<int>> ez_to_sid_map_;
  static unsigned int total_sid_;
  static unsigned int event_counter_;
  static unsigned int last_event_update_;
  static long int total_pnl;
  static long int gross_exposure;
  static long int net_exposure;
  static long double total_traded_value;
  static long double total_margin_consumed;

 private:
  HFSAT::SecurityMarketView& this_smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  HFSAT::PromOrderManager* p_prom_order_manager_;
  unsigned long traded_volume_;
  std::map<unsigned int, int> sid_to_last_update_time_;
  std::map<unsigned int, int> sid_to_traded_volume_10_mins_prior_;
  std::map<unsigned int, int> sid_to_position_10_mins_prior_;

  int max_levels_;
  const unsigned int this_sid_;
  HFSAT::PromPNL* prom_pnl_;
  int prev_time_;
  int prev_time_tp_;
  long int current_exposure;
  long int current_pnl;
  HFSAT::BulkFileWriter* p_binary_log_;

  BAUpdateStruct ba_update_struct_;

  std::map<unsigned int, int> sid_to_best_bid_size;
  std::map<unsigned int, int> sid_to_best_ask_size;
  std::map<unsigned int, int> sid_to_best_bid_price;
  std::map<unsigned int, int> sid_to_best_ask_price;

  std::map<std::pair<std::string, std::string>, std::vector<PnlCheckStruct>>& shortcode_timeperiod_to_pnl_check_;

  // store last_reject_reason & time at which it occurs
  std::string last_rej_reason;
  int last_rej_time;

  std::map<unsigned int, int> sid_to_last_execution_time_;
  std::map<unsigned int, int> sid_to_aggressive_buy_in_5_mins_;
  std::map<unsigned int, int> sid_to_aggressive_sell_in_5_mins_;
  std::map<unsigned int, bool> sid_to_aggressive_orders_alert_notified_;

  std::map<unsigned int, int> sid_to_last_ors_message_capture_time_;
  std::map<unsigned int, int> sid_to_ors_messages_not_received_alert_;

  std::map<unsigned int, int> sid_to_last_mds_message_capture_time_;
  std::map<unsigned int, int> sid_to_mds_messages_not_received_alert_;
  double numbers_to_dollars_;
};

std::multimap<time_t, HFSAT::EventLine> SimpleBAwithMarket::event_time_map_;
std::map<HFSAT::EconomicZone_t, std::vector<int>> SimpleBAwithMarket::ez_to_sid_map_;
unsigned int SimpleBAwithMarket::total_sid_ = 0;
unsigned int SimpleBAwithMarket::event_counter_ = 0;
unsigned int SimpleBAwithMarket::last_event_update_ = 0;
long int SimpleBAwithMarket::gross_exposure = 0;
long int SimpleBAwithMarket::net_exposure = 0;
long int SimpleBAwithMarket::total_pnl = 0;
long double SimpleBAwithMarket::total_traded_value = 0;
long double SimpleBAwithMarket::total_margin_consumed = 0;

void SimpleBAwithMarket::OnMarketUpdate(const unsigned int _security_id_,
                                        const HFSAT::MarketUpdateInfo& _market_update_info_) {
  sid_to_last_mds_message_capture_time_[_security_id_] = watch_.msecs_from_midnight();

  if (last_mds_update_time == 0 || (watch_.msecs_from_midnight() - last_mds_update_time) >= market_data_minimum_time) {
    p_prom_order_manager_->CleanPromBAConfirmedMap(this_smv_.bid_int_price(0), this_smv_.ask_int_price(0));
    DefLoop(_security_id_);
    last_mds_update_time = watch_.msecs_from_midnight();
  }
}
void SimpleBAwithMarket::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                      const HFSAT::MarketUpdateInfo& _market_update_info_) {
  sid_to_last_mds_message_capture_time_[_security_id_] = watch_.msecs_from_midnight();

  traded_volume_ += _trade_print_info_.size_traded_;
  if (prev_time_tp_ == 0 || (watch_.msecs_from_midnight() - prev_time_tp_) >= market_data_minimum_time) {
    OnMarketUpdate(_security_id_, _market_update_info_);
    prev_time_tp_ = watch_.msecs_from_midnight();
  }
}

void SimpleBAwithMarket::OnTimePeriodUpdate(const int num_pages_to_add_) {
  std::map<unsigned int, int>::iterator itr_ = sid_to_last_ors_message_capture_time_.begin();

  while (itr_ != sid_to_last_ors_message_capture_time_.end()) {
    if ((watch_.msecs_from_midnight() - (itr_->second)) > sid_to_ors_timeout_map_[itr_->first]) {
      sid_to_ors_messages_not_received_alert_[itr_->first] = true;

    } else {
      sid_to_ors_messages_not_received_alert_[itr_->first] = false;
    }

    itr_++;
  }

  std::map<unsigned int, int>::iterator mds_itr_ = sid_to_last_mds_message_capture_time_.begin();

  while (mds_itr_ != sid_to_last_mds_message_capture_time_.end()) {
    if ((watch_.msecs_from_midnight() - (mds_itr_->second)) > sid_to_mds_timeout_map_[mds_itr_->first]) {
      sid_to_mds_messages_not_received_alert_[mds_itr_->first] = true;

    } else {
      sid_to_mds_messages_not_received_alert_[mds_itr_->first] = false;
    }

    mds_itr_++;
  }
}

void SimpleBAwithMarket::OnGlobalOrderExec(const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_,
                                           const int _size_, const double _trade_px_) {
  sid_to_last_ors_message_capture_time_[_security_id_] = watch_.msecs_from_midnight();
  DefLoop(_security_id_);
}

// void SimpleBAwithMarket:: CleanPromBAConfirmedMap ( )
// {

//   HFSAT::BidPriceSizeMap & intpx_2_sum_bid_confirmed_ = p_prom_order_manager_->intpx_2_sum_bid_confirmed ();
//   HFSAT::AskPriceSizeMap & intpx_2_sum_ask_confirmed_ = p_prom_order_manager_->intpx_2_sum_ask_confirmed ();

//   // Best Bid, Assuming sorted decreasing
//   HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin ( ) ;
//   // Best Ask, Assuming sorted increasing
//   HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin ( ) ;

//   int t_int_best_ask_price = this_smv_.ask_int_price ( 0 ); // lowest
//   int t_int_best_bid_price = this_smv_.bid_int_price ( 0 ); // highest

//   for (intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin ( );
//        intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end ( );
//        intpx_2_sum_bid_confirmed_iter ++)
//     {
//       // No bid map should have entries  higher than mkt price
//       if ( intpx_2_sum_bid_confirmed_iter -> first  > t_int_best_bid_price && intpx_2_sum_bid_confirmed_iter ->
//       second > 0  )
// 	{
// 	  //	  intpx_2_sum_bid_confirmed_iter -> second = 0;
// 	  int t_int_price_ = intpx_2_sum_ask_confirmed_iter -> first;
// 	  intpx_2_sum_bid_confirmed_[t_int_price_] = 0;
// 	}

//     }

//   for (intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin ( );
//        intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end ( );
//        intpx_2_sum_ask_confirmed_iter ++)
//     {
//       if ( intpx_2_sum_ask_confirmed_iter -> second  > 0  && intpx_2_sum_ask_confirmed_iter -> first <
//       t_int_best_ask_price )
// 	{
// 	  int t_int_price_ = intpx_2_sum_ask_confirmed_iter -> first;
// 	  intpx_2_sum_ask_confirmed_[t_int_price_] = 0;
// 	}
//     }

//}

void SimpleBAwithMarket::OnGlobalOrderChange(const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_,
                                             const int _int_price_) {
  sid_to_last_ors_message_capture_time_[_security_id_] = watch_.msecs_from_midnight();

  if (sid_to_last_execution_time_.find(_security_id_) == sid_to_last_execution_time_.end()) {
    sid_to_last_execution_time_[_security_id_] = watch_.msecs_from_midnight();
    sid_to_aggressive_orders_alert_notified_[_security_id_] = false;

  } else {
    if (watch_.msecs_from_midnight() - sid_to_last_execution_time_[_security_id_] >= _AGGRESSIVE_CHECK_TIMEOUT_) {
      dbglogger_ << " Aggresive Executions Reset Check SID : " << _security_id_
                 << " Msecs From Midnight : " << (int)watch_.msecs_from_midnight() << "\n";
      dbglogger_.DumpCurrentBuffer();

      sid_to_aggressive_buy_in_5_mins_[_security_id_] = 0;
      sid_to_aggressive_sell_in_5_mins_[_security_id_] = 0;

      sid_to_last_execution_time_[_security_id_] = watch_.msecs_from_midnight();
      sid_to_aggressive_orders_alert_notified_[_security_id_] = false;
    }
  }

#if _ENABLE_PNL_ALERTS_
  if (send_pnl_alert_) {
    string timeofday_ = "EU_MORN_DAY";
    if (watch_.msecs_from_midnight() > 43200000) timeofday_ = "US_MORN_DAY";
    for (unsigned int ii = 0;
         ii < shortcode_timeperiod_to_pnl_check_[std::make_pair(this_smv_.market_update_info_.shortcode_, timeofday_)]
                  .size();
         ii++) {
      PnlCheckStruct* pnl_check_struct_ =
          &shortcode_timeperiod_to_pnl_check_[std::make_pair(this_smv_.market_update_info_.shortcode_, timeofday_)][ii];
      if (pnl_check_struct_->last_execution_time_ == 0) {
        pnl_check_struct_->recent_pnl_ = (int)prom_pnl_->total_pnl();
        pnl_check_struct_->last_execution_time_ = watch_.msecs_from_midnight();
      } else {
        if (watch_.msecs_from_midnight() - pnl_check_struct_->last_execution_time_ >=
            pnl_check_struct_->pnl_checkout_time_) {
          pnl_check_struct_->recent_pnl_ = (int)prom_pnl_->total_pnl();
          pnl_check_struct_->last_execution_time_ = watch_.msecs_from_midnight();
        }
      }
    }
  }
#endif

  const HFSAT::BidPriceSizeMap& intpx_2_sum_bid_confirmed_ = p_prom_order_manager_->intpx_2_sum_bid_confirmed();
  const HFSAT::AskPriceSizeMap& intpx_2_sum_ask_confirmed_ = p_prom_order_manager_->intpx_2_sum_ask_confirmed();

  // Best Bid, Assuming sorted decreasing
  HFSAT::BidPriceSizeMapConstIter_t intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
  // Best Ask, Assuming sorted increasing
  HFSAT::AskPriceSizeMapConstIter_t intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();

  // We need to find out the first entry in the map with nonzero confirmed order
  // Max bidprice for nonzero size , min askprice for nonzero size
  int best_bid_size = DEFAULT_VAL;
  int best_bid_price = DEFAULT_VAL;
  // dbglogger_<< "SBC: "<<"\n";
  // dbglogger_.DumpCurrentBuffer( ) ;

  for (intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin();
       intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end(); intpx_2_sum_bid_confirmed_iter++) {
    if (intpx_2_sum_bid_confirmed_iter->second > 0) {
      best_bid_size = intpx_2_sum_bid_confirmed_iter->second;
      best_bid_price = intpx_2_sum_bid_confirmed_iter->first;
      break;
    }
  }

  // // Printing info //TODO {} remove
  // for (intpx_2_sum_bid_confirmed_iter = intpx_2_sum_bid_confirmed_.begin ( );
  //      intpx_2_sum_bid_confirmed_iter != intpx_2_sum_bid_confirmed_.end ( );
  //      intpx_2_sum_bid_confirmed_iter ++)
  //   {
  //     dbglogger_ << "SIZE: "<< intpx_2_sum_bid_confirmed_iter -> second << " PX: "<< intpx_2_sum_bid_confirmed_iter
  //     -> first <<"\n";
  //     dbglogger_.DumpCurrentBuffer ( ) ;
  //   }

  // dbglogger_<< "SAC: "<<"\n";
  // dbglogger_.DumpCurrentBuffer( ) ;
  // for (intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin ( );
  //      intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end ( );
  //      intpx_2_sum_ask_confirmed_iter ++)
  //   {
  //     dbglogger_ << "SIZE: "<< intpx_2_sum_ask_confirmed_iter -> second << " PX: "<<intpx_2_sum_ask_confirmed_iter ->
  //     first  << "\n";
  //     dbglogger_.DumpCurrentBuffer ( ) ;
  //   }

  // // end TODO{}

  int best_ask_size = DEFAULT_VAL;
  int best_ask_price = DEFAULT_VAL;

  for (intpx_2_sum_ask_confirmed_iter = intpx_2_sum_ask_confirmed_.begin();
       intpx_2_sum_ask_confirmed_iter != intpx_2_sum_ask_confirmed_.end(); intpx_2_sum_ask_confirmed_iter++) {
    if (intpx_2_sum_ask_confirmed_iter->second > 0) {
      best_ask_size = intpx_2_sum_ask_confirmed_iter->second;
      best_ask_price = intpx_2_sum_ask_confirmed_iter->first;
      break;
    }
  }

  sid_to_best_bid_size[_security_id_] = best_bid_size;
  sid_to_best_ask_size[_security_id_] = best_ask_size;
  sid_to_best_bid_price[_security_id_] = best_bid_price;
  sid_to_best_ask_price[_security_id_] = best_ask_price;

  if (ors_minimum_time == 0 || (watch_.msecs_from_midnight() - last_ors_update_time) >= ors_minimum_time) {
    last_ors_update_time = watch_.msecs_from_midnight();
    DefLoop(_security_id_, _int_price_, _buysell_);
  }
}

void SimpleBAwithMarket::DumpHeader() {
  std::ofstream oebu_info_outstream;
  oebu_info_outstream.open(temp_oebu_info, std::ofstream::out);
  if (false == oebu_info_outstream.is_open()) {
    fprintf(stderr," Can't Open File : %s\n",temp_oebu_info);
    std::exit(-1);
  }
  chmod(TEMP_OEBU_INFO_JSON_FILE, S_IRWXU | S_IRWXG | S_IRWXO);
  oebu_info_outstream << "jsonstr= [{ "
                      << " \"TotalPnl\": \"" << total_pnl << "\", "
                      << " \"Time\": \"" << ba_update_struct_.time_string_ << " " << watch_.tv().tv_sec << "."
                      << watch_.tv().tv_usec << " " << watch_.YYYYMMDD() << "\", "
                      << " \"NetExposure\": \"" << net_exposure << "\","
                      << " \"GrossExposure\" :\"" << gross_exposure << "\","
                      << " \"TotalMarginConsumed\" :\"" << total_margin_consumed / LAKH_DIVISOR << "\","
                      << " \"TotalTradedValue\" : \" "
                      << (p_prom_order_manager_->get_total_traded_value() / LAKH_DIVISOR) * numbers_to_dollars_
                      << "\"}]";
  oebu_info_outstream.flush();
  oebu_info_outstream.close();

  if (rename(TEMP_OEBU_INFO_JSON_FILE, FINAL_OEBU_INFO_JSON_FILE)) {
    fprintf(stderr,"Error renaming json file : %s\n ",temp_oebu_info);
  }
  chmod(final_oebu_info, S_IRWXU | S_IRWXG | S_IRWXO);

  chmod(temp_json_file, S_IRWXU | S_IRWXG | S_IRWXO);
  outstream.open(temp_json_file, std::ofstream::out);

  if (false == outstream.is_open()) {
    fprintf(stderr," Can't Open File : %s\n",temp_json_file);
    std::exit(-1);
  }

#if _ENABLE_PNL_ALERTS_
  if (send_pnl_alert_) {
    string timeofday_ = "EU_MORN_DAY";
    if (watch_.msecs_from_midnight() > 43200000) timeofday_ = "US_MORN_DAY";

    for (unsigned int ii = 0;
         ii < shortcode_timeperiod_to_pnl_check_[std::make_pair(this_smv_.market_update_info_.shortcode_, timeofday_)]
                  .size();
         ii++) {
      PnlCheckStruct* pnl_check_struct_ =
          &shortcode_timeperiod_to_pnl_check_[std::make_pair(this_smv_.market_update_info_.shortcode_, timeofday_)][ii];
      if (pnl_check_struct_->pnl_check_ != 0) {
        if (pnl_check_struct_->recent_pnl_ - (int)prom_pnl_->total_pnl() > pnl_check_struct_->pnl_check_) {
          // sending alert
          char alert_message_[100] = "";
          sprintf(alert_message_, "pnl limit of %d crossed for %s", pnl_check_struct_->pnl_check_,
                  this_smv_.market_update_info_.shortcode_.c_str());
          HFSAT::SendAlert::sendAlert(alert_message_);
          dbglogger_ << "pnl limit alert message: " << alert_message_ << "\n";
          dbglogger_.DumpCurrentBuffer();

          pnl_check_struct_->recent_pnl_ = (int)prom_pnl_->total_pnl();
          pnl_check_struct_->last_execution_time_ = watch_.msecs_from_midnight();
        }
      }
    }
    //      return;
  }
#endif
}

void SimpleBAwithMarket::DumpTrailer() {
  outstream.flush();
  outstream.close();
}

void SimpleBAwithMarket::DefLoop(unsigned int this_sid_number, const int _trade_px_,
                                 const HFSAT::TradeType_t _buysell_) {
  memset(&ba_update_struct_, 0, sizeof(ba_update_struct_));

  strncpy(ba_update_struct_.time_string_, watch_.time_string(), sizeof(ba_update_struct_.time_string_));
  // proceeding to display next event notif
  int event_pos = total_sid_ + 5;  // leave an empty line after all products
  int current_time = watch_.tv().tv_sec;
  int interval = 10 * 60;  // 10 minutes
  std::multimap<time_t, HFSAT::EventLine>::iterator event_time_map_itr_ = event_time_map_.begin();
  int flag = 0;
  int num_seconds_before_highlight = 3 * 60;  // 3 minutes
  bool highlight_products = false;
  HFSAT::EventLine sev_event_;
  unsigned int sev_event_pos = 1;
  unsigned int total_events_in_interval = 1;

  while (event_time_map_itr_ != event_time_map_.end()) {
    time_t next_event_time = event_time_map_itr_->first;
    if (next_event_time < current_time) {
      if (event_counter_ > 0) event_counter_--;
      event_time_map_.erase(event_time_map_itr_++);
    } else if (next_event_time - current_time <= interval) {
      flag = 1;
      if (event_time_map_itr_->second.severity_ > sev_event_.severity_) {
        sev_event_ = event_time_map_itr_->second;
        sev_event_pos = total_events_in_interval;
      } else if (event_time_map_itr_->second.severity_ == sev_event_.severity_) {
        if (event_time_map_itr_->second.ez_ == HFSAT::EZ_USD) {
          sev_event_ = event_time_map_itr_->second;
          sev_event_pos = total_events_in_interval;
        } else if (event_time_map_itr_->second.ez_ == HFSAT::EZ_EUR && sev_event_.ez_ != HFSAT::EZ_USD) {
          sev_event_ = event_time_map_itr_->second;
          sev_event_pos = total_events_in_interval;
        }
      }
      event_time_map_itr_++;
      total_events_in_interval++;
    } else {
      break;
    }
  }
  ++event_pos;
  total_pnl = total_pnl - current_pnl + (long int)prom_pnl_->total_pnl();
  current_pnl = (long int)prom_pnl_->total_pnl();
  net_exposure = net_exposure - current_exposure +
                 (long int)(p_prom_order_manager_->global_position() * this_smv_.mid_price()) / LAKH_DIVISOR;
  gross_exposure = gross_exposure - abs(current_exposure) +
                   abs((long int)(p_prom_order_manager_->global_position() * this_smv_.mid_price()) / LAKH_DIVISOR);
  current_exposure = (long int)(p_prom_order_manager_->global_position() * this_smv_.mid_price()) / LAKH_DIVISOR;
  std::ostringstream pnl_exposure_str;
  pnl_exposure_str << "Total Pnl is " << total_pnl << " Net Exposure is " << net_exposure << " Gross Exposure is "
                   << gross_exposure << " Total Traded Value is " << (total_traded_value / LAKH_DIVISOR) << "\n";

  HFSAT::EconomicZone_t ez_ = HFSAT::EZ_MAX;
  unsigned int total_events_ = event_time_map_.size();
  if (total_events_ == 0) {
    event_pos++;

  } else {
    if (flag == 0)  // no event in next 10 mins
      sev_event_ = event_time_map_.begin()->second;

    total_events_--;
    event_pos++;
    int seconds_remaining = sev_event_.event_time_ - current_time;
    time_t t_sev_event_time = sev_event_.event_time_;
    struct tm* tm = localtime(&t_sev_event_time);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    std::ostringstream next_event_str;
    next_event_str << "Next Event(in " << seconds_remaining << " sec): " << sev_event_.event_text_ << " at " << time_str
                   << ". Severity: " << sev_event_.severity_
                   << " Ezone: " << HFSAT::GetStrFromEconomicZone(sev_event_.ez_) << "\n";
    std::ostringstream pnl_str;
    ez_ = sev_event_.ez_;
    if (sev_event_.event_time_ - current_time <= num_seconds_before_highlight) highlight_products = true;

    // update next upcoming events only after 5 sec
    if (total_events_ > 0 && current_time - last_event_update_ > 5) {
      event_time_map_itr_ = event_time_map_.begin();
      last_event_update_ = current_time;
      for (unsigned int counter = 1; counter <= event_counter_;) {
        if (strcmp(event_time_map_itr_->second.event_text_, sev_event_.event_text_) != 0 ||
            event_time_map_itr_->second.event_time_ != sev_event_.event_time_ ||
            event_time_map_itr_->second.ez_ != sev_event_.ez_)
          counter++;
        if (event_time_map_itr_ != event_time_map_.end()) event_time_map_itr_++;
      }
      // skipped over event_counter_ number of events, take care of special cases
      // when severest event is just the one after the amount skipped
      // this is severest event, skip this one too
      if (sev_event_pos == event_counter_ + 1) event_time_map_itr_++;
      event_counter_++;
      event_pos++;
      int seconds_remaining = event_time_map_itr_->second.event_time_ - current_time;
      time_t t_event_time = (time_t)event_time_map_itr_->second.event_time_;
      struct tm* tm = localtime(&t_event_time);
      char time_str[20];
      strftime(time_str, 20, " %H:%M:%S", tm);
      std::ostringstream later_event_str;
      later_event_str << "Later(" << event_counter_ << "/" << total_events_ << ") (in " << seconds_remaining
                      << " sec): " << event_time_map_itr_->second.event_text_ << " at " << time_str
                      << ". Severity: " << event_time_map_itr_->second.severity_
                      << " Ezone: " << HFSAT::GetStrFromEconomicZone(event_time_map_itr_->second.ez_) << "\n";
      std::cout << later_event_str.str() << "\n";

      ez_ = event_time_map_itr_->second.ez_;
      if (event_counter_ >= total_events_) event_counter_ = 0;
    }
  }
  // event notifcation code ends

  std::string this_color_str_ = NC;
  if (sid_to_ors_messages_not_received_alert_.find(this_sid_number) != sid_to_ors_messages_not_received_alert_.end()) {
    this_color_str_ = (sid_to_ors_messages_not_received_alert_[this_sid_number] == true) ? RED : NC;

  } else {
    this_color_str_ = NC;
  }

  if (this_color_str_ == NC) {
    if (sid_to_mds_messages_not_received_alert_.find(this_sid_number) !=
        sid_to_mds_messages_not_received_alert_.end()) {
      this_color_str_ = (sid_to_mds_messages_not_received_alert_[this_sid_number] == true) ? YELLOW : NC;

    } else {
      this_color_str_ = NC;
    }

  } else {
    // WE are already not placing orders, now if the market data has stopped than highlight that
    if (this_color_str_ == RED) {
      if (sid_to_mds_messages_not_received_alert_.find(this_sid_number) !=
          sid_to_mds_messages_not_received_alert_.end()) {
        this_color_str_ = (sid_to_mds_messages_not_received_alert_[this_sid_number] == true) ? BLUE : RED;
      }
    }
  }
  if (highlight_products) {
    // event_pos++;
    std::map<HFSAT::EconomicZone_t, std::vector<int>>::iterator ez_sc_itr;
    int sid_matches = -1;
    ez_sc_itr = ez_to_sid_map_.find(ez_);
    if (ez_sc_itr != ez_to_sid_map_.end()) {
      sid_matches = ez_sc_itr->second.size();

      for (int i = 0; i < sid_matches; i++) {
        if ((unsigned)ez_sc_itr->second[i] == this_sid_number && p_prom_order_manager_->global_position() != 0) {
          this_color_str_ = GREEN;
          break;
        }
      }
    }
  }

  ba_update_struct_.sec_id_ = this_sid_number;

  if (_trade_px_ != 0) {
    if (_buysell_ == HFSAT::kTradeTypeBuy) {
      if (this_smv_.market_update_info_.asklevels_.size() > 0 && _trade_px_ >= this_smv_.ask_int_price(0)) {
        if (sid_to_aggressive_buy_in_5_mins_.find(this_sid_number) == sid_to_aggressive_buy_in_5_mins_.end())
          sid_to_aggressive_buy_in_5_mins_[this_sid_number] = 1;
        else
          sid_to_aggressive_buy_in_5_mins_[this_sid_number]++;

        dbglogger_ << "Sec : " << this_smv_.secname() << " TrdPX : " << _trade_px_ << " Side : " << (int)_buysell_
                   << " MktBid : " << this_smv_.bid_int_price(0) << " X " << this_smv_.ask_int_price(0)
                   << " Position : " << p_prom_order_manager_->global_position()
                   << " Total Agg Orders : " << sid_to_aggressive_buy_in_5_mins_[this_sid_number] << " X "
                   << sid_to_aggressive_sell_in_5_mins_[this_sid_number] << "\n";
        dbglogger_.DumpCurrentBuffer();
      }

    } else {
      if (this_smv_.market_update_info_.bidlevels_.size() > 0 && _trade_px_ <= this_smv_.bid_int_price(0)) {
        if (sid_to_aggressive_sell_in_5_mins_.find(this_sid_number) == sid_to_aggressive_sell_in_5_mins_.end())
          sid_to_aggressive_sell_in_5_mins_[this_sid_number] = 1;
        else
          sid_to_aggressive_sell_in_5_mins_[this_sid_number]++;

        dbglogger_ << "Sec : " << this_smv_.secname() << " TrdPX : " << _trade_px_ << " Side : " << (int)_buysell_
                   << " MktBid : " << this_smv_.bid_int_price(0) << " X " << this_smv_.ask_int_price(0)
                   << " Position : " << p_prom_order_manager_->global_position()
                   << " Total Agg Orders : " << sid_to_aggressive_buy_in_5_mins_[this_sid_number] << " X "
                   << sid_to_aggressive_sell_in_5_mins_[this_sid_number] << "\n";
        dbglogger_.DumpCurrentBuffer();
      }
    }
  }

  if (sid_to_aggressive_buy_in_5_mins_.find(this_sid_number) != sid_to_aggressive_buy_in_5_mins_.end() &&
      sid_to_aggressive_sell_in_5_mins_.find(this_sid_number) != sid_to_aggressive_sell_in_5_mins_.end() &&
      (abs(sid_to_aggressive_buy_in_5_mins_[this_sid_number] - sid_to_aggressive_sell_in_5_mins_[this_sid_number]) >
       _AGGRESSIVE_ORDERS_THRESHOLD_)) {
    std::ostringstream alert_txt_;
    alert_txt_ << "Aggressive Executions For : " << std::string(this_smv_.secname()) << " "
               << sid_to_aggressive_buy_in_5_mins_[this_sid_number] << " X "
               << sid_to_aggressive_sell_in_5_mins_[this_sid_number];

    if (!sid_to_aggressive_orders_alert_notified_[this_sid_number]) {
      //      SendAlertForAggressiveOrders( alert_txt_.str() ) ;
      sid_to_aggressive_orders_alert_notified_[this_sid_number] = true;
    }
  }

  int m_m_levels = std::min(max_levels_, std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()));
  int t_level_ = 0;  // Max Bid and Min Ask
  if (m_m_levels >= 1 && std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()) >= 1) {
    switch (this_smv_.secname()[0]) {
      case 'Z':
      case 'U': {
        rejection_reason_string_ = "";

        switch (p_prom_order_manager_->get_last_order_rejection_reason()) {
          case HFSAT::kORSRejectSecurityNotFound: {
            rejection_reason_string_ = "SecNF";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedOrderSizes: {
            rejection_reason_string_ = "OrSz";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedMaxPosition: {
            rejection_reason_string_ = "MxPos";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedWorstCasePosition: {
            rejection_reason_string_ = "WrsPos";

          } break;

          case HFSAT::kSendOrderRejectNotMinOrderSizeMultiple: {
            rejection_reason_string_ = "MinSz";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedMaxLiveOrders: {
            rejection_reason_string_ = "LvOrd";

          } break;

          case HFSAT::kORSRejectSelfTradeCheck: {
            rejection_reason_string_ = "STC";

          } break;

          case HFSAT::kORSRejectThrottleLimitReached: {
            rejection_reason_string_ = "THRTL";

          } break;

          case HFSAT::kTAPRejectThrottleLimitReached: {
            rejection_reason_string_ = "TAP";

          } break;

          case HFSAT::kORSRejectMarketClosed: {
            rejection_reason_string_ = "MktCl";

          } break;

          case HFSAT::kORSRejectNewOrdersDisabled: {
            rejection_reason_string_ = "OrdDis";

          } break;

          default: { rejection_reason_string_ = ""; } break;
        }

        // code to remove timedout reject reason
        if (!rejection_reason_string_.empty() && rejection_reason_string_ != last_rej_reason) {
          last_rej_reason = rejection_reason_string_;
          last_rej_time = current_time;
        } else if (!rejection_reason_string_.empty() && last_rej_reason == rejection_reason_string_) {
          if (current_time - last_rej_time > 300)  // rej reason already displayed for 5 minutes
          {
            rejection_reason_string_ = "";
          }
        } else {
          rejection_reason_string_ = "";
        }

        if (sid_to_last_update_time_[this_sid_number] == 0 ||
            watch_.tv().tv_sec - sid_to_last_update_time_[this_sid_number] > 10 * 60) {
          sid_to_last_update_time_[this_sid_number] = watch_.tv().tv_sec;
          sid_to_traded_volume_10_mins_prior_[this_sid_number] = traded_volume_;
          sid_to_position_10_mins_prior_[this_sid_number] = p_prom_order_manager_->get_executed_totalsize();
        }

        // ================================================================
        // Log this update.
        if (p_binary_log_) {
          strncpy(ba_update_struct_.symbol_, this_smv_.secname(), sizeof(ba_update_struct_.symbol_));
          ba_update_struct_.top_bid_size_ = this_smv_.bid_size(t_level_);
          ba_update_struct_.top_bid_order_ = this_smv_.bid_order(t_level_);
          ba_update_struct_.top_bid_price_ = this_smv_.bid_price(t_level_);
          ba_update_struct_.int_top_bid_price_ = this_smv_.bid_int_price(t_level_);
          ba_update_struct_.int_top_ask_price_ = this_smv_.ask_int_price(t_level_);
          ba_update_struct_.top_ask_price_ = this_smv_.ask_price(t_level_);
          ba_update_struct_.top_ask_order_ = this_smv_.ask_order(t_level_);
          ba_update_struct_.top_ask_size_ = this_smv_.ask_size(t_level_);

          ba_update_struct_.sid_to_best_bid_size_ = sid_to_best_bid_size[this_sid_number];
          ba_update_struct_.sid_to_best_bid_price_ = sid_to_best_bid_price[this_sid_number];
          ba_update_struct_.sid_to_best_ask_price_ = sid_to_best_ask_price[this_sid_number];
          ba_update_struct_.sid_to_best_ask_size_ = sid_to_best_ask_size[this_sid_number];

          ba_update_struct_.total_pnl_ = (int)prom_pnl_->total_pnl();
          ba_update_struct_.global_position_ = p_prom_order_manager_->global_position();
          ba_update_struct_.executed_ = p_prom_order_manager_->get_executed_totalsize();
          ba_update_struct_.traded_volume_ = traded_volume_;
          ba_update_struct_.rejected_ = p_prom_order_manager_->get_rejected();
          ba_update_struct_.num_ors_messages_ = p_prom_order_manager_->get_num_ors_messages();

          p_binary_log_->Write((void*)&ba_update_struct_, sizeof(ba_update_struct_));
        }
        // ================================================================

        // dbglogger_ << this_smv_.secname ( ) << "\t"  <<	      this_smv_.bid_int_price_level ( t_level_ ) <<"\t"
        // <<this_smv_.bid_size ( t_level_ )<<"\t" << this_smv_.bid_order ( t_level_) <<"\t"<<	this_smv_.bid_price (
        // t_level_ ) << "\t"<< this_smv_.bid_int_price ( t_level_ )<<"\t" <<this_smv_.ask_int_price ( t_level_ )<<"\t"
        // <<this_smv_.ask_price ( t_level_ )<<"\t"<<this_smv_.ask_order ( t_level_ )<<"\t"<<	this_smv_.ask_size (
        // t_level_ )<<"\t"<<	this_smv_.ask_int_price_level ( t_level_ ) <<"\t"<<sid_to_best_bid_size_c
        // <<"\t"<<sid_to_best_bid_price_c <<"\t"<<sid_to_best_ask_price_c<<"\t"
        // <<sid_to_best_ask_size_c<<"\t"<<prom_pnl_ -> total_pnl ( ) <<"\t"<<p_prom_order_manager_ -> global_position (
        // )<<"\t"<<	p_prom_order_manager_ -> get_executed ( ) <<"\t"<<p_prom_order_manager_ -> get_num_ors_messages
        // ( )<<"\n";
        // dbglogger_.DumpCurrentBuffer ( ) ;

      } break;
      default: {
        char sid_to_best_bid_size_c[20] = {'\0'};
        char sid_to_best_bid_price_c[20] = {'\0'};
        char sid_to_best_ask_price_c[20] = {'\0'};
        char sid_to_best_ask_size_c[20] = {'\0'};

        // char sid_to_mkt_ask_price_c[11] ={'\0'};
        // char sid_to_mkt_bid_price_c[11] ={'\0'};

        if (sid_to_best_bid_size[this_sid_number] == DEFAULT_VAL) {
          sprintf(sid_to_best_bid_size_c, "%s", "--");
        } else {
          sprintf(sid_to_best_bid_size_c, "%d", sid_to_best_bid_size[this_sid_number]);
        }

        if (sid_to_best_bid_price[this_sid_number] == DEFAULT_VAL) {
          sprintf(sid_to_best_bid_price_c, "%s", "--");
        } else {
          sprintf(sid_to_best_bid_price_c, "%d", sid_to_best_bid_price[this_sid_number]);
        }
        if (sid_to_best_ask_price[this_sid_number] == DEFAULT_VAL) {
          sprintf(sid_to_best_ask_price_c, "%s", "--");
        } else {
          sprintf(sid_to_best_ask_price_c, "%d", sid_to_best_ask_price[this_sid_number]);
        }
        if (sid_to_best_ask_size[this_sid_number] == DEFAULT_VAL) {
          sprintf(sid_to_best_ask_size_c, "%s", "--");
        } else {
          sprintf(sid_to_best_ask_size_c, "%d", sid_to_best_ask_size[this_sid_number]);
        }

        // char sid_to_mkt_ask_price_c[11] ={'\0'};
        // char sid_to_mkt_bid_price_c[11] ={'\0'};

        // Market Price will always be available

        // memcpy ( sid_to_mkt_ask_price_c, &(this_smv_.bid_price(t_level_)), 11);
        // memcpy ( sid_to_mkt_bid_price_c, &(this_smv_.ask_price(t_level_)), 11);
        rejection_reason_string_ = "";

        switch (p_prom_order_manager_->get_last_order_rejection_reason()) {
          case HFSAT::kORSRejectSecurityNotFound: {
            rejection_reason_string_ = "SecNF";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedOrderSizes: {
            rejection_reason_string_ = "OrSz";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedMaxPosition: {
            rejection_reason_string_ = "MxPos";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedWorstCasePosition: {
            rejection_reason_string_ = "WrsPos";

          } break;

          case HFSAT::kSendOrderRejectNotMinOrderSizeMultiple: {
            rejection_reason_string_ = "MinSz";

          } break;

          case HFSAT::kORSRejectMarginCheckFailedMaxLiveOrders: {
            rejection_reason_string_ = "LvOrd";

          } break;

          case HFSAT::kORSRejectSelfTradeCheck: {
            rejection_reason_string_ = "STC";

          } break;

          case HFSAT::kORSRejectThrottleLimitReached: {
            rejection_reason_string_ = "THRTL";

          } break;

          case HFSAT::kORSRejectMarketClosed: {
            rejection_reason_string_ = "MktCl";

          } break;

          case HFSAT::kORSRejectNewOrdersDisabled: {
            rejection_reason_string_ = "OrdDis";

          } break;

          default: { rejection_reason_string_ = ""; } break;
        }

        // code to remove timedout reject reason
        if (!rejection_reason_string_.empty() && rejection_reason_string_ != last_rej_reason) {
          last_rej_reason = rejection_reason_string_;
          last_rej_time = current_time;
        } else if (!rejection_reason_string_.empty() && last_rej_reason == rejection_reason_string_) {
          if (current_time - last_rej_time > 300)  // rej reason already displayed for 5 minutes
          {
            rejection_reason_string_ = "";
          }
        } else {
          rejection_reason_string_ = "";
        }

        if (this_smv_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX ||
            this_smv_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ ||
            this_smv_.this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR) {
        } else {
          string secname = std::string(this_smv_.secname());
          // This is done show readable names in case of NSE. Instead of NSE3669 etc., we want to show
          // NSE_NIFTY_FUT_20150827 etc.
          if (this_smv_.exch_source() == HFSAT::kExchSourceNSE) {
            HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
            secname = sec_name_indexer_.GetShortcodeFromId(this_smv_.security_id());

            if (std::string::npos != secname.find("NSE_")) {
              secname = secname.substr(secname.find("NSE_") + std::string("NSE_").length());
            }
          }

          if (sid_margin_factor_.find(this_smv_.security_id()) != sid_margin_factor_.end() &&
              sid_margin_consumed.find(this_smv_.security_id()) != sid_margin_consumed.end()) {
            total_margin_consumed =
                total_margin_consumed - sid_margin_consumed[this_smv_.security_id()] +
                std::fabs(p_prom_order_manager_->global_position() * sid_margin_factor_[this_smv_.security_id()]);
            sid_margin_consumed[this_smv_.security_id()] =
                std::fabs(p_prom_order_manager_->global_position() * sid_margin_factor_[this_smv_.security_id()]);
          }

          std::string start = "\"";
          std::string end = "\",";

          std::ostringstream tempstring;

          tempstring << "[";
          tempstring << start << secname.c_str() << end;
          tempstring << start << this_smv_.bid_size(t_level_) << end;
          tempstring << start << this_smv_.bid_order(t_level_) << end;
          tempstring << start << this_smv_.bid_price(t_level_) << end;
          tempstring << start << this_smv_.bid_int_price(t_level_) << end;
          tempstring << start << this_smv_.ask_int_price(t_level_) << end;
          tempstring << start << this_smv_.ask_price(t_level_) << end;
          tempstring << start << this_smv_.ask_order(t_level_) << end;
          tempstring << start << this_smv_.ask_size(t_level_) << end;
          tempstring << start << sid_to_best_bid_size_c << end;
          tempstring << start << sid_to_best_bid_price_c << end;
          tempstring << start << sid_to_best_ask_price_c << end;
          tempstring << start << sid_to_best_ask_size_c << end;
          tempstring << start << (int)prom_pnl_->total_pnl() << end;
          tempstring << start << prom_pnl_->get_pnl_diff(PNL_DIFF_TIME_) << end;
          tempstring << start << p_prom_order_manager_->global_position() << end;
          tempstring << start << (int)(p_prom_order_manager_->global_position() * this_smv_.mid_price()) << end;
          tempstring << start << p_prom_order_manager_->get_executed_totalsize() << end;
          tempstring << start << traded_volume_ << end;
          tempstring << start
                     << ((p_prom_order_manager_->get_executed_totalsize()) / (double)(traded_volume_ + 1)) * 100.0
                     << end;
          tempstring << start << p_prom_order_manager_->get_rejected() << end;
          tempstring << start << p_prom_order_manager_->get_exch_rejected() << end;
          tempstring << start << p_prom_order_manager_->get_exch_cxl_rejected() << end;
          tempstring << start << p_prom_order_manager_->get_cxl_rejected() << end;
          tempstring << start << p_prom_order_manager_->get_exch_replace_rejects() << end;
          tempstring << start << p_prom_order_manager_->get_ors_replace_rejects() << end;
          tempstring << start << p_prom_order_manager_->get_num_ors_messages() << end;
          tempstring << start << rejection_reason_string_.c_str() << "\"]";

          sec_id_to_data[this_sid_number] = tempstring.str();
          DumpHeader();
          outstream << "{ \"data\" : [ ";
          auto final_iter = sec_id_to_data.end();
          --final_iter;
          for (auto it = sec_id_to_data.begin(); it != sec_id_to_data.end(); ++it) {
            if (it != final_iter)
              outstream << it->second << ",";
            else
              outstream << it->second;
          }
          outstream << "]}";
          DumpTrailer();
          if (rename(temp_json_file, final_json_file)) {
            fprintf(stderr,"Error renaming json file : %s\n",temp_json_file);
          }
          chmod(final_json_file, S_IRWXU | S_IRWXG | S_IRWXO);
        }

        if (sid_to_last_update_time_[this_sid_number] == 0 ||
            watch_.tv().tv_sec - sid_to_last_update_time_[this_sid_number] > 10 * 60) {
          sid_to_last_update_time_[this_sid_number] = watch_.tv().tv_sec;
          sid_to_traded_volume_10_mins_prior_[this_sid_number] = traded_volume_;
          sid_to_position_10_mins_prior_[this_sid_number] = p_prom_order_manager_->get_executed_totalsize();
        }

        // ================================================================
        // Log this update.
        if (p_binary_log_) {
          strncpy(ba_update_struct_.symbol_, this_smv_.secname(), sizeof(ba_update_struct_.symbol_));
          ba_update_struct_.top_bid_size_ = this_smv_.bid_size(t_level_);
          ba_update_struct_.top_bid_order_ = this_smv_.bid_order(t_level_);
          ba_update_struct_.top_bid_price_ = this_smv_.bid_price(t_level_);
          ba_update_struct_.int_top_bid_price_ = this_smv_.bid_int_price(t_level_);
          ba_update_struct_.int_top_ask_price_ = this_smv_.ask_int_price(t_level_);
          ba_update_struct_.top_ask_price_ = this_smv_.ask_price(t_level_);
          ba_update_struct_.top_ask_order_ = this_smv_.ask_order(t_level_);
          ba_update_struct_.top_ask_size_ = this_smv_.ask_size(t_level_);

          ba_update_struct_.sid_to_best_bid_size_ = sid_to_best_bid_size[this_sid_number];
          ba_update_struct_.sid_to_best_bid_price_ = sid_to_best_bid_price[this_sid_number];
          ba_update_struct_.sid_to_best_ask_price_ = sid_to_best_ask_price[this_sid_number];
          ba_update_struct_.sid_to_best_ask_size_ = sid_to_best_ask_size[this_sid_number];

          ba_update_struct_.total_pnl_ = (int)prom_pnl_->total_pnl();
          ba_update_struct_.global_position_ = p_prom_order_manager_->global_position();
          ba_update_struct_.executed_ = p_prom_order_manager_->get_executed_totalsize();
          ba_update_struct_.traded_volume_ = traded_volume_;
          ba_update_struct_.rejected_ = p_prom_order_manager_->get_rejected();
          ba_update_struct_.num_ors_messages_ = p_prom_order_manager_->get_num_ors_messages();

          p_binary_log_->Write((void*)&ba_update_struct_, sizeof(ba_update_struct_));
        }
        // ================================================================

        // dbglogger_ << this_smv_.secname ( ) << "\t"  <<	      this_smv_.bid_int_price_level ( t_level_ ) <<"\t"
        // <<this_smv_.bid_size ( t_level_ )<<"\t" << this_smv_.bid_order ( t_level_) <<"\t"<<	this_smv_.bid_price (
        // t_level_ ) << "\t"<< this_smv_.bid_int_price ( t_level_ )<<"\t" <<this_smv_.ask_int_price ( t_level_ )<<"\t"
        // <<this_smv_.ask_price ( t_level_ )<<"\t"<<this_smv_.ask_order ( t_level_ )<<"\t"<<	this_smv_.ask_size (
        // t_level_ )<<"\t"<<	this_smv_.ask_int_price_level ( t_level_ ) <<"\t"<<sid_to_best_bid_size_c
        // <<"\t"<<sid_to_best_bid_price_c <<"\t"<<sid_to_best_ask_price_c<<"\t"
        // <<sid_to_best_ask_size_c<<"\t"<<prom_pnl_ -> total_pnl ( ) <<"\t"<<p_prom_order_manager_ -> global_position (
        // )<<"\t"<<	p_prom_order_manager_ -> get_executed ( ) <<"\t"<<p_prom_order_manager_ -> get_num_ors_messages
        // ( )<<"\n";
        // dbglogger_.DumpCurrentBuffer ( ) ;

      } break;
    }
  }
}

SimpleBAwithMarket* simple_ba_price_with_market_price_ = NULL;
HFSAT::ShortcodeRequestHelper* global_shc_request_helper;

void termHandler(int signum) {
  if (simple_ba_price_with_market_price_) {
    delete simple_ba_price_with_market_price_;
  }

  if (global_shc_request_helper) {
    global_shc_request_helper->RemoveAllShortcodesToListen();
    global_shc_request_helper = nullptr;
  }

  exit(0);
}

// Checks if any of the instruments are NSE symbols and if so adds NSE contract specs
bool CheckAndAddNSEDefinitions(std::vector<std::string>& t_shortcode_vec_, int trading_date) {
  bool is_nse_present_ = false;
  for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
    std::cout << " Shc:" << t_shortcode_vec_[i] << "\n";
    if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
      is_nse_present_ = true;
    }
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(trading_date).LoadNSESecurityDefinitions();
  }

  return is_nse_present_;
}

void LoadMarginValues() {
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::NSESecurityDefinitions& nse_sec_def_ =
      HFSAT::NSESecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
  std::ostringstream t_temp_oss;
  t_temp_oss << SECURITYMARGINFILEPATH << "security_margin_" << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".txt";

  std::string fname = t_temp_oss.str();

  std::ifstream mstream;
  mstream.open(fname.c_str(), std::ifstream::in);

  if (false == mstream.is_open()) {
    std::cerr << "UNABLE TO OPEN THE FILE -> " << fname << " SYSERROR : " << strerror(errno);
    exit(1);
  }

  char buffer[1024];
  while (mstream.good()) {
    mstream.getline(buffer, 1024);
    std::string linebuffer = buffer;

    if (std::string::npos != linebuffer.find("#")) continue;  // comments

    HFSAT::PerishableStringTokenizer st(buffer, 1024);
    std::vector<char const*> const& tokens = st.GetTokens();

    if (2 != tokens.size()) continue;

    if (nse_sec_def_.IsEquity(tokens[0]))
      sid_margin_factor_[sec_name_indexer_.GetIdFromString(tokens[0])] = atof(tokens[1]);
    else
      sid_margin_factor_[sec_name_indexer_.GetIdFromString(tokens[0])] = atof(tokens[1]) * 0.1;
  }

  mstream.close();
}

int main(int argc, char** argv) {
  signal(SIGINT, termHandler);
  signal(SIGSEGV, termHandler);

  std::vector<std::string> sec_list_vec;
  std::vector<const char*> exchange_symbol_vec;
  std::vector<int> shortcode_to_mds_min_time_interval;
  std::vector<int> shortcode_to_ors_min_time_interval;

  std::map<std::pair<std::string, std::string>, std::vector<PnlCheckStruct>> shortcode_timeperiod_to_pnl_check_;
  // std::map<std::pair<std::string, std::string>, int> shortcode_timeperiod_to_pnl_checkout_time_;

  if (argc < 4) {
    std::cerr << " usage : Exec Input file name, an Output file  and an OEBU INFO file containing the Symbols E.g ~/infracore_install/files/mkt_sec.txt [LOG] [Pnl "
                 "Drop Checks Filename]" << std::endl;
    exit(0);
  }

  bool are_we_filtering = false;
  std::vector < int32_t > ors_base;

  if ( argc >= 5 ) {
    are_we_filtering = true ;
    for ( int32_t i = 4 ; i < argc ; i++ ){
      ors_base.push_back( atoi(argv[i]) );
    }
  }

  std::string filename_input(argv[1]);
  strcpy(final_json_file,argv[2]);
  sprintf(temp_oebu_info,"/var/www/html/%ld_oebuinfo.json",(long)getpid());
  strcpy(final_oebu_info,argv[3]);
  sprintf(temp_json_file,"/var/www/html/%ld_data.json",(long)getpid());
  int prog_id = -100;
  HFSAT::DebugLogger dbglogger_(1024000, 1);
  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/our_extended_ba_mkt_book_nse."
                << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  int tradingdate_ = HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate();

  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  // forcing prog_id to be between -200 and -101
  prog_id = (current_time.tv_usec % 100) - 200;

  // Make Sure This Is Done Above All Other Classe's Initialization, Needed For ASX TICK Changes
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(tradingdate_, (time_t)current_time.tv_sec);

  /////////FILE INPUT OF SHORTCODES///////////
  char line[1024];
  std::ifstream sec_file_;
  sec_file_.open(filename_input.c_str(), std::ifstream::in);
  if (!sec_file_.is_open()) {
    std::cerr << filename_input << "  FILE DOESNOT EXIST " << std::endl;
    exit(-1);
  }

  while (!sec_file_.eof()) {
    bzero(line, 1024);
    sec_file_.getline(line, 1024);
    if (strstr(line, "#") || strlen(line) == 0) continue;
    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();
    if (tokens.size() < 1) {
      std::cerr << " Bad file..See #entries " << std::endl;
      exit(-1);
    }

    if (tokens.size() == 2) {
      if (strcmp(tokens[0], "QUERY_ID") == 0) {
        int temp = atoi(tokens[1]);
        // making sure that prog_id fro oebu is between -100 and -200
        if (temp <= -100 && temp >= -200) {
          prog_id = temp;
        }
        continue;
      }
    }

    std::cout << " SEC_SHORTCODE " << tokens[0] << std::endl;
    sec_list_vec.push_back(std::string(tokens[0]));

    if (tokens.size() < 3) {
      std::cout << "Assuming default minimum time interval for display values for " << tokens[0] << std::endl;
      shortcode_to_mds_min_time_interval.push_back(MARKET_DATA_MINIMUM_TIME);
      shortcode_to_ors_min_time_interval.push_back(ORS_MINIMUM_TIME);
    } else {
      shortcode_to_mds_min_time_interval.push_back(atoi(tokens[1]));
      shortcode_to_ors_min_time_interval.push_back(atoi(tokens[2]));
    }
  }
  ////////@end FILE INPUT////////////////

  bool is_nse_added = CheckAndAddNSEDefinitions(sec_list_vec, tradingdate_);

  // ================================================================
  HFSAT::BulkFileWriter* p_binary_log_ = NULL;

  // Added functionality to alternatively log data in binary.
  if (argc > 4 && !strncmp(argv[4], "LOG", strlen("LOG"))) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/our_extended_ba_mkt_book_binary_log."
                << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();

    p_binary_log_ = new HFSAT::BulkFileWriter(logfilename_.c_str(), 4 * 1024,
                                              std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
  }
// ================================================================

#if _ENABLE_PNL_ALERTS_
  if (argc > 5) {
    send_pnl_alert_ = true;
  }

  if (send_pnl_alert_) {
    std::string pnl_check_file_name_(argv[5]);
    char line[1024];
    std::ifstream pnl_check_file_;
    pnl_check_file_.open(pnl_check_file_name_.c_str(), std::ifstream::in);
    if (!pnl_check_file_.is_open()) {
      std::cerr << pnl_check_file_name_ << " FILE DOESNOT EXIST " << std::endl;
      exit(-1);
    }

    while (!pnl_check_file_.eof()) {
      bzero(line, 1024);
      pnl_check_file_.getline(line, 1024);
      if (strstr(line, "#") || strlen(line) == 0) continue;
      HFSAT::PerishableStringTokenizer st(line, 1024);
      const std::vector<const char*>& tokens = st.GetTokens();
      if (tokens.size() < 4) {
        std::cerr << " Bad pnl check file..See #entries " << std::endl;
        exit(-1);
      }
      PnlCheckStruct pnl_check_struct_;
      pnl_check_struct_.pnl_check_ = atoi(tokens[2]);
      pnl_check_struct_.pnl_checkout_time_ = atoi(tokens[3]);
      pnl_check_struct_.last_execution_time_ = 0;
      pnl_check_struct_.recent_pnl_ = 0;
      shortcode_timeperiod_to_pnl_check_[std::make_pair(tokens[0], tokens[1])].push_back(pnl_check_struct_);
    }
  }
#endif

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    exchange_symbol_vec.push_back(HFSAT::ExchangeSymbolManager::GetExchSymbol(sec_list_vec[ii]));
  }
  // dbglogger_.AddLogLevel ( BOOK_ERROR );
  // dbglogger_.AddLogLevel ( BOOK_INFO );
  // dbglogger_.AddLogLevel(PROM_OM_INFO);

  // dbglogger_.AddLogLevel ( BOOK_TEST );

  if (p_binary_log_) {
    dbglogger_ << "Logging data in binary mode.\n";
  } else {
    dbglogger_ << "Not logging data in binary mode\n";
  }
  dbglogger_.DumpCurrentBuffer();

  HFSAT::ShortcodeRequestHelper shc_req_helper(prog_id);
  global_shc_request_helper = &shc_req_helper;
  shc_req_helper.AddShortcodeListToListen(sec_list_vec);

  HFSAT::Watch watch_(dbglogger_, HFSAT::DateTime::GetCurrentIsoDateLocal());

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    sec_name_indexer_.AddString(exchange_symbol_vec[ii], sec_list_vec[ii]);
    sid_margin_consumed[ii] = 0;
  }
  LoadMarginValues();

  //==========   load file for timeout ===============//

  std::ifstream timeout_config_file_;
  timeout_config_file_.open(OEBU_MDS_ORS_TIMEOUT_CONFIG);

  if (!timeout_config_file_.is_open()) {
    std::cerr << " Cna't open oebu timeout config file : " << OEBU_MDS_ORS_TIMEOUT_CONFIG << "\n";
    exit(-1);
  }

  char timeout_buffer_[1024];

  while (timeout_config_file_.good()) {
    memset(timeout_buffer_, 0, 1024);
    timeout_config_file_.getline(timeout_buffer_, 1024);

    std::string this_line_read_ = timeout_buffer_;

    if (this_line_read_.find("#") != std::string::npos) continue;

    HFSAT::PerishableStringTokenizer st_(timeout_buffer_, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if (tokens_.size() != 3) continue;

    std::string this_shortcode_ = tokens_[0];

    std::string this_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(this_shortcode_);

    int this_shortcode_secid_ = sec_name_indexer_.GetIdFromSecname(this_exchange_symbol_.c_str());

    int ors_timeout_ = atoi(tokens_[1]);
    int mds_timeout_ = atoi(tokens_[2]);

    sid_to_ors_timeout_map_[this_shortcode_secid_] = ors_timeout_;
    sid_to_mds_timeout_map_[this_shortcode_secid_] = mds_timeout_;
  }

  timeout_config_file_.close();

  for (unsigned int this_sec_counter_ = 0; this_sec_counter_ < sec_name_indexer_.NumSecurityId(); this_sec_counter_++) {
    if (sid_to_ors_timeout_map_.find(this_sec_counter_) == sid_to_ors_timeout_map_.end()) {
      sid_to_ors_timeout_map_[this_sec_counter_] = ORS_MSG_TIMEOUT_;
    }

    if (sid_to_mds_timeout_map_.find(this_sec_counter_) == sid_to_mds_timeout_map_.end()) {
      sid_to_mds_timeout_map_[this_sec_counter_] = MDS_MSG_TIMEOUT_;
    }

    std::cerr << " Symbol : " << sec_name_indexer_.GetSecurityNameFromId(this_sec_counter_)
              << " Security Id : " << this_sec_counter_ << " Id : "
              << sec_name_indexer_.GetIdFromSecname(sec_name_indexer_.GetSecurityNameFromId(this_sec_counter_))
              << " ORS Timeout : " << ORS_MSG_TIMEOUT_ << " MDS Timeout : " << MDS_MSG_TIMEOUT_ << "\n";
  }

  //=================================================//

  HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();
  if (curr_location_ == HFSAT::kTLocM1 || curr_location_ == HFSAT::kTLocSYD || curr_location_ == HFSAT::kTLocBMF) {
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
  }
  if (curr_location_ == HFSAT::kTLocHK) {
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
  }
  if (curr_location_ == HFSAT::kTLocJPY) {
    _USE_HK_SHM_SOURCE_ = false;
    _USE_CME_LIVE_DATA_SOURCE_ = true;
    _USE_LIFFE_LIVE_DATA_SOURCE_ = true;
  }

  bool use_ose_l1_ = false;
  bool USE_COMBINED_SOURCE = true;
  bool use_nse_l1 = false;

  if (curr_location_ == HFSAT::kTLocBMF || curr_location_ == HFSAT::kTLocCFE || curr_location_ == HFSAT::kTLocTMX ||
      curr_location_ == HFSAT::kTLocM1 || curr_location_ == HFSAT::kTLocFR2 || curr_location_ == HFSAT::kTLocNSE ||
      curr_location_ == HFSAT::kTLocSYD) {
    USE_COMBINED_SOURCE = true;
  }

  if (curr_location_ == HFSAT::kTLocSPR) {
    use_nse_l1 = true;
  }

  // We use L1 price feed on Hk only. On TOK we use Combined Feed
  // After 19082016, we would be using combined feed
  //  if (curr_location_ == HFSAT::kTLocHK) {
  //    use_ose_l1_ = true;
  //  }

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  std::map<std::string, HFSAT::ORSMessageLiveSource*> shortcode_ors_live_source_map_;

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();
  ///< Unique Instance of map from shortcode to p_smv_

  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

  for (unsigned int ii = 0; ii < (unsigned int)sec_name_indexer_.NumSecurityId(); ii++) {
    HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(
        sec_list_vec[ii], HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
    int _this_sid_ = sec_name_indexer_.GetIdFromString(sec_list_vec[ii]);

    if (_this_exch_source_ == HFSAT::kExchSourceEUREX) {
      _this_exch_source_ = HFSAT::kExchSourceEOBI;
    }

    if (_this_exch_source_ == HFSAT::kExchSourceHONGKONG) {
      _this_exch_source_ = HFSAT::kExchSourceHKOMDPF;
    }

    bool set_temporary_bool_checking_if_this_is_an_indexed_book_ =
        HFSAT::CommonSimIndexedBookBool(_this_exch_source_, sec_list_vec[ii], curr_location_);

    if (_this_exch_source_ == HFSAT::kExchSourceJPY && use_ose_l1_) {
      set_temporary_bool_checking_if_this_is_an_indexed_book_ = false;
    }

    HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
        dbglogger_, watch_, sec_name_indexer_, sec_list_vec[ii], exchange_symbol_vec[ii], _this_sid_,
        _this_exch_source_, set_temporary_bool_checking_if_this_is_an_indexed_book_);
    std::cout << " SID : for SMV " << _this_sid_ << std::endl;
    sid_to_smv_ptr_map_.push_back(p_smv_);                  // add to security_id_ to SMV* map
    shortcode_smv_map_.AddEntry(sec_list_vec[ii], p_smv_);  // add to shortcode_ to SMV* map
  }

  HFSAT::IndexedCmeMarketViewManager indexed_cme_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedEobiPriceLevelMarketViewManager indexed_eobi_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedLiffePriceLevelMarketViewManager indexed_liffe_price_level_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedRtsMarketViewManager indexed_rts_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedMicexMarketViewManager indexed_micex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_ntp_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedNtpMarketViewManager indexed_puma_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::IndexedCfeMarketViewManager indexed_cfe_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::HKEXIndexedMarketViewManager hkex_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                sid_to_smv_ptr_map_);
  HFSAT::IndexedIceMarketViewManager indexed_ice_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::IndexedHKOMDPriceLevelMarketViewManager indexed_hkomd_price_level_market_view_manager(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);
  HFSAT::IndexedTmxMarketViewManager indexed_tmx_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);

  // HFSAT::
  HFSAT::OSEOrderLevelMarketViewManager ose_order_level_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                             sid_to_smv_ptr_map_);

  HFSAT::OSEL1PriceMarketViewManager ose_l1_price_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                       sid_to_smv_ptr_map_);
  HFSAT::OSEPriceFeedMarketViewManager ose_pricefeed_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                          sid_to_smv_ptr_map_);
  HFSAT::IndexedOsePriceFeedMarketViewManager indexed_ose_price_feed_market_view_manager_(
      dbglogger_, watch_, sec_name_indexer_, sid_to_smv_ptr_map_);

  HFSAT::IndexedNSEMarketViewManager indexed_nse_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                      sid_to_smv_ptr_map_);
  HFSAT::GenericL1DataMarketViewManager indexed_nse_l1_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                            sid_to_smv_ptr_map_);

  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_(
      dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(
        sec_list_vec[ii], HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
    if (_this_exch_source_ == HFSAT::kExchSourceJPY && use_ose_l1_) {
      sid_to_smv_ptr_map_[ii]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = false;
    } else {
      sid_to_smv_ptr_map_[ii]->market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = true;
      sid_to_smv_ptr_map_[ii]->InitializeSMVForIndexedBook();
    }
  }

  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CME, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CME_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EUREX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EOBI_PF,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EOBI_LS,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_eobi_price_feed_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::EUREX_LS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::ICE, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::ASX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::SGX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::LIFFE,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::LIFFE_LS,
      (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_liffe_price_level_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::RTS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_rts_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::MICEX, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_micex_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::NTP, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::BMF_EQ, (void*)((HFSAT::NTPPriceLevelGlobalListener*)(&indexed_ntp_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::OSE_CF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ose_price_feed_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::OSE_ITCH_PF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)),
      &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::OSE_L1, (void*)((HFSAT::FullBookGlobalListener*)&(ose_l1_price_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::HKOMDPF,
      (void*)((HFSAT::HKHalfBookGlobalListener*)(&indexed_hkomd_price_level_market_view_manager)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::CSM, (void*)((HFSAT::CFEPriceLevelGlobalListener*)&(indexed_cfe_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX, (void*)((HFSAT::FullBookGlobalListener*)(&indexed_tmx_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX_LS, (void*)((HFSAT::FullBookGlobalListener*)(&indexed_tmx_market_view_manager_)), &watch_);
  combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::TMX_OBF, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_ice_market_view_manager_)), &watch_);

  // Since NSE requires some files to load which may/may not be present on a given server
  if (true == is_nse_added) {
    if (use_nse_l1) {
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::NSE_L1, (void*)((HFSAT::L1DataListener*)&(indexed_nse_l1_market_view_manager_)), &watch_);
    } else {
      combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
          HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager_)), &watch_);
    }
  }

  std::map<std::string, HFSAT::PromOrderManager*> p_prom_order_manager_map;
  std::map<std::string, HFSAT::PromPNL*> p_prom_pnl_map_;
  //  std::map < std::string, HFSAT::BaseOrderManager * > p_base_order_manager_map ;

  HFSAT::BulkFileWriter bulkwriter("/spare/local/logs/alllogs/our_extended_BULK.log");

  if (USE_COMBINED_SOURCE) {
    combined_mds_messages_shm_processor_.AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY, &watch_,
                                                                        sid_to_smv_ptr_map_);
    if ( true == are_we_filtering ){
      std::cerr << "ENABLED FILTERING ON ORS WITH :" << std::endl ;
      for ( auto & itr : ors_base ) {
        std::cerr << "ORS CLIENT ID : " << itr << std::endl ;
      }
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->EnableORSFilteringAndAddORSBase( ors_base );
    }

  }

  // Event manager: display next event in OEBU
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_);
  const std::vector<HFSAT::EventLine>& events_of_the_day_ = economic_events_manager_.events_of_the_day();
  std::multimap<time_t, HFSAT::EventLine> event_time_map;
  unsigned int num_events_today_ = events_of_the_day_.size();
  for (auto i = 0u; i < num_events_today_; i++) {
    event_time_map.insert(std::make_pair(events_of_the_day_[i].event_time_, events_of_the_day_[i]));
    // event_time_map[events_of_the_day_[i].event_time_]=events_of_the_day_[i];
  }

  std::map<HFSAT::EconomicZone_t, std::vector<int>> ez_to_shortcode_map;
  std::map<HFSAT::EconomicZone_t, std::vector<int>>::iterator ez_sc_itr;
  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    std::vector<HFSAT::EconomicZone_t> zones;
    HFSAT::GetEZVecForShortcode(sec_list_vec[ii], watch_.msecs_from_midnight(), zones);

    for (unsigned int zone_it = 0; zone_it < zones.size(); zone_it++) {
      ez_sc_itr = ez_to_shortcode_map.find(zones[zone_it]);
      if (ez_sc_itr == ez_to_shortcode_map.end()) {
        std::vector<int> temp_sc;
        temp_sc.push_back(sec_name_indexer_.GetIdFromString(sec_list_vec[ii]));
        ez_to_shortcode_map[zones[zone_it]] = temp_sc;

      } else
        ez_sc_itr->second.push_back(sec_name_indexer_.GetIdFromString(sec_list_vec[ii]));
    }
  }
  SimpleBAwithMarket::event_time_map_ = event_time_map;
  SimpleBAwithMarket::ez_to_sid_map_ = ez_to_shortcode_map;
  SimpleBAwithMarket::total_sid_ = (unsigned int)sec_list_vec.size();
  // event_time_map, ez_to_shortcode_map, (unsigned int)sec_list_vec.size()

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    const unsigned int _this_sid_ = (unsigned int)sec_name_indexer_.GetIdFromString(sec_list_vec[ii]);

    std::string _this_shortcode_ = sec_list_vec[ii];
    p_prom_order_manager_map[_this_shortcode_] = HFSAT::PromOrderManager::GetUniqueInstance(
        dbglogger_, watch_, sec_name_indexer_, sec_list_vec[ii], _this_sid_, exchange_symbol_vec[ii]);

    if (USE_COMBINED_SOURCE) {
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderSequencedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfirmedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfCxlReplacedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderCanceledListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderExecutedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderRejectedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderRejectedDueToFundsListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfCxlReplaceRejectListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
    } else {
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderSequencedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderConfirmedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderConfCxlReplacedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderCanceledListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderExecutedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderRejectedListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderRejectedDueToFundsListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
      shortcode_ors_live_source_map_[sec_list_vec[ii]]->AddOrderConfCxlReplaceRejectListener(
          _this_sid_, p_prom_order_manager_map[_this_shortcode_]);
    }

    //      HFSAT::PromPNL*  this_prom_pnl =
    p_prom_pnl_map_[_this_shortcode_] =
        new HFSAT::PromPNL(dbglogger_, watch_, *p_prom_order_manager_map[_this_shortcode_],
                           *(shortcode_smv_map_.GetSecurityMarketView(_this_shortcode_)),
                           101,  // TODO {} runtime_id_
                           bulkwriter);

    simple_ba_price_with_market_price_ =
        new SimpleBAwithMarket(*(shortcode_smv_map_.GetSecurityMarketView(_this_shortcode_)), dbglogger_, watch_,
                               p_prom_order_manager_map[_this_shortcode_], _this_sid_,
                               p_prom_pnl_map_[_this_shortcode_], p_binary_log_, shortcode_timeperiod_to_pnl_check_);
    simple_ba_price_with_market_price_->market_data_minimum_time = shortcode_to_mds_min_time_interval[ii];
    simple_ba_price_with_market_price_->ors_minimum_time = shortcode_to_ors_min_time_interval[ii];

    // Dummy function to remove unused variable warning
    simple_ba_price_with_market_price_->DoNothing();
  }

  if (USE_COMBINED_SOURCE) {
    combined_mds_messages_shm_processor_.RunLiveShmSource();
  } else {
    try {
      simple_live_dispatcher_.RunLive();
    } catch (int e) {
    }
  }

  return 0;
}
