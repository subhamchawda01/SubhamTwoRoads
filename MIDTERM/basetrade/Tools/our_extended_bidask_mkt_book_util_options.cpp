// Simple Utility that shows Best Market Bid/Ask and our position Bid/Ask and Our PNL, traded volume,
// #ORS Messages
// @ramkris

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <signal.h>

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

#define _USE_EUREX_LIVE_DATA_SOURCE_ true

#define _AGGRESSIVE_ORDERS_THRESHOLD_ 10
#define _AGGRESSIVE_CHECK_TIMEOUT_ 300000
#define MARKET_DATA_MINIMUM_TIME 3000
#define ORS_MINIMUM_TIME 1000

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[33m"
#define NC "\x1B[37m"

#define _ENABLE_PNL_ALERTS_ false

#define ORS_MSG_TIMEOUT_ 600000
#define MDS_MSG_TIMEOUT_ 300000

// Sending voice alert for sharp pnl changes
bool send_pnl_alert_ = false;
bool _USE_LIFFE_LIVE_DATA_SOURCE_ = false;
bool _USE_CME_LIVE_DATA_SOURCE_ = false;
bool _USE_HK_SHM_SOURCE_ = false;
std::map<unsigned int, int> sid_to_ors_timeout_map_;
std::map<unsigned int, int> sid_to_mds_timeout_map_;

std::string rejection_reason_string_ = "";

struct PnlCheckStruct {
  int pnl_check_;
  int pnl_checkout_time_;
  int recent_pnl_;
  int last_execution_time_;
};

struct MetaData {
  int64_t volume;
  int64_t msg_count;
  int64_t position;
  double pnl;
  int line_num;

  double delta;
  double gamma;
  double vega;

  int64_t o_rej;
  int64_t e_rej;
  int64_t e_c_rej;
  int64_t c_rej;

  MetaData() {
    volume = 0;
    msg_count = 0;
    position = 0;
    pnl = 0;
    line_num = 0;

    delta = 0;
    gamma = 0;
    vega = 0;

    o_rej = 0;
    e_rej = 0;
    e_c_rej = 0;
    c_rej = 0;
  }
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
        underlying_(),
        line_number_(1),
        fp_(NULL),
        delta_(1.0),
        gamma_(0),
        vega_(0),
        prev_vals_(),
        option_obj_(nullptr),
        is_option_(false),
        fut_smv_(nullptr) {
    memset(&ba_update_struct_, 0, sizeof(ba_update_struct_));

    p_prom_order_manager_->AddGlobalOrderChangeListener(this);
    p_prom_order_manager_->AddGlobalOrderExecListener(this);
    p_prom_order_manager_->ManageOrdersAlso();
    this_svm_t.subscribe_tradeprints(this);
    watch_.subscribe_BigTimePeriod(this);

    std::string shortcode = this_svm_t.shortcode();
    int pos_1 = shortcode.find("_");
    int pos_2 = shortcode.find("_", pos_1 + 1);

    underlying_ = shortcode.substr(pos_1 + 1, pos_2 - pos_1 - 1);

    lot_size_ =
        HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcode, HFSAT::DateTime::GetCurrentIsoDateLocal());

    if (underlying_num_contracts_map_.find(underlying_) == underlying_num_contracts_map_.end()) {
      underlying_num_contracts_map_[underlying_] = 1;
      line_number_ = 1;
    } else {
      underlying_num_contracts_map_[underlying_] += 1;
      line_number_ = underlying_num_contracts_map_[underlying_];
    }

    if (underlying_meta_data_map_.find(underlying_) == underlying_meta_data_map_.end()) {
      underlying_meta_data_map_[underlying_] = MetaData();
    }

    if (underlying_oebu_file_map_.find(underlying_) == underlying_oebu_file_map_.end()) {
      summary_line_num_++;
      underlying_meta_data_map_[underlying_].line_num = summary_line_num_;

      std::string oebu_file_name = "/spare/local/logs/alllogs/oebu_out_" + underlying_;
      fp_ = fopen(oebu_file_name.c_str(), "w+");
      underlying_oebu_file_map_[underlying_] = fp_;
      fprintf(fp_, "\033c");
      fprintf(fp_, "OEBU main screen\n");
    } else {
      fp_ = underlying_oebu_file_map_[underlying_];
    }

    std::string summary_file_name = "/spare/local/logs/alllogs/oebu_out_summary";
    summary_file_ = fopen(summary_file_name.c_str(), "w+");

    fprintf(summary_file_, "\033c");
    fprintf(summary_file_, "OEBU Sumamry Screen\n");

    is_option_ = HFSAT::NSESecurityDefinitions::IsOption(shortcode);

    if (is_option_) {
      option_obj_ = HFSAT::OptionObject::GetUniqueInstance(dbglogger_t, watch_, shortcode);
      HFSAT::SecurityNameIndexer& indexer = HFSAT::SecurityNameIndexer::GetUniqueInstance();

      int fut_sec_id =
          indexer.GetIdFromString(HFSAT::NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(shortcode));

      //      std::cout << "OPTION: " << shortcode
      //                << " FUT: " << HFSAT::NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(shortcode)
      //                << " SecID: " << fut_sec_id << std::endl;

      if (fut_sec_id != -1) {
        fut_smv_ = HFSAT::sid_to_security_market_view_map()[fut_sec_id];
        //        std::cout << "FUT SMV: " << fut_smv_->shortcode() << std::endl;
      }
    }
  }

  ~SimpleBAwithMarket() {
    if (p_binary_log_) {
      p_binary_log_->Close();
      delete p_binary_log_;
    }
  }

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
  int market_data_minimum_time = 0;
  int ors_minimum_time = 0;

  // map to hold day's event in sorted order
  static std::multimap<time_t, HFSAT::EventLine> event_time_map_;
  static std::map<HFSAT::EconomicZone_t, std::vector<int>> ez_to_sid_map_;
  static unsigned int total_sid_;
  static unsigned int event_counter_;
  static unsigned int last_event_update_;
  static std::map<std::string, FILE*> underlying_oebu_file_map_;
  static std::map<std::string, int> underlying_num_contracts_map_;
  static std::map<std::string, MetaData> underlying_meta_data_map_;
  static FILE* summary_file_;
  static int summary_line_num_;

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

  std::string underlying_;
  int line_number_;
  FILE* fp_;

  double delta_;
  double gamma_;
  double vega_;
  MetaData prev_vals_;
  HFSAT::OptionObject* option_obj_;
  bool is_option_;
  HFSAT::SecurityMarketView* fut_smv_;
  int lot_size_;
};

std::multimap<time_t, HFSAT::EventLine> SimpleBAwithMarket::event_time_map_;
std::map<HFSAT::EconomicZone_t, std::vector<int>> SimpleBAwithMarket::ez_to_sid_map_;
unsigned int SimpleBAwithMarket::total_sid_ = 0;
unsigned int SimpleBAwithMarket::event_counter_ = 0;
unsigned int SimpleBAwithMarket::last_event_update_ = 0;
std::map<std::string, FILE*> SimpleBAwithMarket::underlying_oebu_file_map_;
std::map<std::string, int> SimpleBAwithMarket::underlying_num_contracts_map_;
std::map<std::string, MetaData> SimpleBAwithMarket::underlying_meta_data_map_;
FILE* SimpleBAwithMarket::summary_file_ = nullptr;
int SimpleBAwithMarket::summary_line_num_ = 0;

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

  if (is_option_) {
    if (fut_smv_ != nullptr) {
      double fut_px = fut_smv_->mid_price();
      double opt_px = this_smv_.mid_price();
      if (fut_px > 0 && opt_px > 0) {
        option_obj_->ComputeGreeks(fut_px, opt_px);
        delta_ = option_obj_->greeks_.delta_;
        gamma_ = option_obj_->greeks_.gamma_;
        vega_ = option_obj_->greeks_.vega_;
      }

      //      std::cout << "Computing Greeks: " << this_smv_.shortcode() << " FUTPx: " << fut_px << " : MyPx: " <<
      //      opt_px
      //                << " Delta: " << delta_ << std::endl;
    }
  }
}

void SimpleBAwithMarket::OnGlobalOrderExec(const unsigned int _security_id_, const HFSAT::TradeType_t _buysell_,
                                           const int _size_, const double _trade_px_) {
  sid_to_last_ors_message_capture_time_[_security_id_] = watch_.msecs_from_midnight();
  DefLoop(_security_id_);
}

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

void SimpleBAwithMarket::DefLoop(unsigned int this_sid_number, const int _trade_px_,
                                 const HFSAT::TradeType_t _buysell_) {
  fprintf(fp_, "\033[1;1H");

  memset(&ba_update_struct_, 0, sizeof(ba_update_struct_));

  strncpy(ba_update_struct_.time_string_, watch_.time_string(), sizeof(ba_update_struct_.time_string_));
  fprintf(fp_, "Time: %s\n", ba_update_struct_.time_string_);

  fprintf(fp_,
          "%*s %5s %3s %15s %7s X %7s %15s %3s %5s  %9s %6s %6s %5s %8s %6s %4s %4s %6s %9s %7s %6s %6s %6s %6s %6s "
          "%6s\n",
          16, "SYM", "BS", "BO", "BP", "BIP", "AIP", "AP", "AO", "AS", "S_BS", "S_BP", "S_AP", "S_AS", "T_PNL", "POS",
          "AggB", "AggS", "TV", "MV", "% v/V", "O_REJ", "E_REJ", "E_C_REJ", "C_REJ", "ORS", "RejRes");

  int current_time = watch_.tv().tv_sec;
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

  //  printf("\033[%d;1H", 4 + this_sid_number);  // move to 4+this_sid_numberth line

  fprintf(fp_, "\033[%d;1H", 4 + line_number_);

  int m_m_levels = std::min(max_levels_, std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()));
  int t_level_ = 0;  // Max Bid and Min Ask
  if (m_m_levels >= 1 && std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()) >= 1) {
    char sid_to_best_bid_size_c[20] = {'\0'};
    char sid_to_best_bid_price_c[20] = {'\0'};
    char sid_to_best_ask_price_c[20] = {'\0'};
    char sid_to_best_ask_size_c[20] = {'\0'};
    // char sid_to_mkt_ask_price_c[11] ={'\0'};
    // char sid_to_mkt_bid_price_c[11] ={'\0'};

    if (sid_to_best_bid_size[this_sid_number] == DEFAULT_VAL) {
      sprintf(sid_to_best_bid_size_c, "%s", "--");
    } else {
      sprintf(sid_to_best_bid_size_c, "%d", sid_to_best_bid_size[this_sid_number] / lot_size_);
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
      sprintf(sid_to_best_ask_size_c, "%d", sid_to_best_ask_size[this_sid_number] / lot_size_);
    }

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

    fprintf(fp_,
            "%s%*s %5d %3d %15.7lf %7d X %7d %15.7lf %3d %5d  %9s %6s %6s %5s %8d %6d %4d %4d %6d %9lu "
            "%7.2lf %6d "
            "%6d %6d %6d %6d %6s\n",
            this_color_str_.c_str(),
            16,  // 13 Width for Symbol
            secname.c_str(),
            // this_smv_.bid_int_price_level ( t_level_ ),
            this_smv_.bid_size(t_level_) / lot_size_, this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_),
            // sid_to_mkt_bid_price_c,
            this_smv_.bid_int_price(t_level_), this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_),
            // sid_to_mkt_ask_price_c,
            this_smv_.ask_order(t_level_), this_smv_.ask_size(t_level_) / lot_size_,
            // this_smv_.ask_int_price_level ( t_level_ ) ,
            sid_to_best_bid_size_c, sid_to_best_bid_price_c, sid_to_best_ask_price_c, sid_to_best_ask_size_c,
            (int)prom_pnl_->total_pnl(), p_prom_order_manager_->global_position() / lot_size_,
            sid_to_aggressive_buy_in_5_mins_[this_sid_number], sid_to_aggressive_sell_in_5_mins_[this_sid_number],
            p_prom_order_manager_->get_executed_totalsize() / lot_size_, traded_volume_ / lot_size_,
            ((p_prom_order_manager_->get_executed_totalsize() - sid_to_position_10_mins_prior_[this_sid_number]) /
             (double)(traded_volume_ - sid_to_traded_volume_10_mins_prior_[this_sid_number] + 1)) *
                100.0,
            p_prom_order_manager_->get_rejected(), p_prom_order_manager_->get_exch_rejected(),
            p_prom_order_manager_->get_exch_cxl_rejected(), p_prom_order_manager_->get_cxl_rejected(),
            p_prom_order_manager_->get_num_ors_messages(), rejection_reason_string_.c_str());

    int summary_line_num = underlying_num_contracts_map_[underlying_] + 6;

    int orig_msg_count = underlying_meta_data_map_[underlying_].msg_count;

    fprintf(fp_, "\033[%d;1H", summary_line_num);
    underlying_meta_data_map_[underlying_].msg_count -= prev_vals_.msg_count;
    underlying_meta_data_map_[underlying_].position -= prev_vals_.position;
    underlying_meta_data_map_[underlying_].pnl -= prev_vals_.pnl;
    underlying_meta_data_map_[underlying_].volume -= prev_vals_.volume;
    underlying_meta_data_map_[underlying_].delta -= prev_vals_.delta;
    underlying_meta_data_map_[underlying_].gamma -= prev_vals_.gamma;
    underlying_meta_data_map_[underlying_].vega -= prev_vals_.vega;
    underlying_meta_data_map_[underlying_].o_rej -= prev_vals_.o_rej;
    underlying_meta_data_map_[underlying_].e_rej -= prev_vals_.e_rej;
    underlying_meta_data_map_[underlying_].e_c_rej -= prev_vals_.e_c_rej;
    underlying_meta_data_map_[underlying_].c_rej -= prev_vals_.c_rej;

    prev_vals_.msg_count = p_prom_order_manager_->get_num_ors_messages();
    prev_vals_.position = abs(p_prom_order_manager_->global_position());
    prev_vals_.pnl = prom_pnl_->total_pnl();
    prev_vals_.volume = fabs(delta_) * abs(p_prom_order_manager_->get_executed_totalsize());
    prev_vals_.delta = delta_ * p_prom_order_manager_->global_position();
    prev_vals_.gamma = gamma_ * p_prom_order_manager_->global_position();
    prev_vals_.vega = vega_ * p_prom_order_manager_->global_position();

    prev_vals_.o_rej = p_prom_order_manager_->get_rejected();
    prev_vals_.e_rej = p_prom_order_manager_->get_exch_rejected();
    prev_vals_.e_c_rej = p_prom_order_manager_->get_exch_cxl_rejected();
    prev_vals_.c_rej = p_prom_order_manager_->get_cxl_rejected();

    underlying_meta_data_map_[underlying_].msg_count += prev_vals_.msg_count;
    underlying_meta_data_map_[underlying_].position += prev_vals_.position;
    underlying_meta_data_map_[underlying_].pnl += prev_vals_.pnl;
    underlying_meta_data_map_[underlying_].volume += prev_vals_.volume;
    underlying_meta_data_map_[underlying_].delta += prev_vals_.delta;
    underlying_meta_data_map_[underlying_].gamma += prev_vals_.gamma;
    underlying_meta_data_map_[underlying_].vega += prev_vals_.vega;
    underlying_meta_data_map_[underlying_].o_rej += prev_vals_.o_rej;
    underlying_meta_data_map_[underlying_].e_rej += prev_vals_.e_rej;
    underlying_meta_data_map_[underlying_].e_c_rej += prev_vals_.e_c_rej;
    underlying_meta_data_map_[underlying_].c_rej += prev_vals_.c_rej;

    int final_msg_count = underlying_meta_data_map_[underlying_].msg_count;

    fprintf(fp_, "%*s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s\n", 16, "UNDERLYING", "PNL", "POS", "TV", "MSG_COUNT",
            "DELTA", "GAMMA", "VEGA", "O_REJ", "E_REJ", "E_C_REJ", "C_REJ");
    fprintf(fp_, "\033[%d;1H", summary_line_num + 1);
    fprintf(fp_, "%*s %7d %7ld %7ld %7ld %7.2f %7.2f %7.2f %7ld %7ld %7ld %7ld\n", 16, underlying_.c_str(),
            (int)underlying_meta_data_map_[underlying_].pnl,
            underlying_meta_data_map_[underlying_].position / lot_size_,
            underlying_meta_data_map_[underlying_].volume / lot_size_, underlying_meta_data_map_[underlying_].msg_count,
            underlying_meta_data_map_[underlying_].delta / lot_size_,
            underlying_meta_data_map_[underlying_].gamma / lot_size_,
            underlying_meta_data_map_[underlying_].vega / lot_size_, underlying_meta_data_map_[underlying_].o_rej,
            underlying_meta_data_map_[underlying_].e_rej, underlying_meta_data_map_[underlying_].e_c_rej,
            underlying_meta_data_map_[underlying_].c_rej);

    fflush(fp_);

    if (final_msg_count != orig_msg_count) {
      fprintf(summary_file_, "\033[1;1H");
      fprintf(summary_file_, "Time: %s\n", ba_update_struct_.time_string_);
      fprintf(summary_file_, "\033[2;1H");
      fprintf(summary_file_, "%s%*s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s\n", this_color_str_.c_str(), 16,
              "UNDERLYING", "PNL", "POS", "TV", "MSG_COUNT", "DELTA", "GAMMA", "VEGA", "O_REJ", "E_REJ", "E_C_REJ",
              "C_REJ");
      fprintf(summary_file_, "\033[%d;1H", underlying_meta_data_map_[underlying_].line_num + 2);
      fprintf(summary_file_, "%s%*s %7d %7ld %7ld %7ld %7.2f %7.2f %7.2f %7ld %7ld %7ld %7ld\n",
              this_color_str_.c_str(), 16, underlying_.c_str(), (int)underlying_meta_data_map_[underlying_].pnl,
              underlying_meta_data_map_[underlying_].position / lot_size_,
              underlying_meta_data_map_[underlying_].volume / lot_size_,
              underlying_meta_data_map_[underlying_].msg_count,
              underlying_meta_data_map_[underlying_].delta / lot_size_,
              underlying_meta_data_map_[underlying_].gamma / lot_size_,
              underlying_meta_data_map_[underlying_].vega / lot_size_, underlying_meta_data_map_[underlying_].o_rej,
              underlying_meta_data_map_[underlying_].e_rej, underlying_meta_data_map_[underlying_].e_c_rej,
              underlying_meta_data_map_[underlying_].c_rej);
      fflush(summary_file_);
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
bool CheckAndAddNSEDefinitions(std::vector<std::string>& t_shortcode_vec_) {
  bool is_nse_present_ = false;
  for (auto i = 0u; i < t_shortcode_vec_.size(); i++) {
    std::cout << " Shc:" << t_shortcode_vec_[i] << "\n";
    if (strncmp(t_shortcode_vec_[i].c_str(), "NSE_", 4) == 0) {
      is_nse_present_ = true;
    }
  }
  if (is_nse_present_) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())
        .LoadNSESecurityDefinitions();
  }

  return is_nse_present_;
}

int main(int argc, char** argv) {
  signal(SIGINT, termHandler);
  signal(SIGSEGV, termHandler);

  std::vector<std::string> sec_list_vec;
  std::vector<std::string> underlying_list;
  std::vector<const char*> exchange_symbol_vec;
  std::vector<int> shortcode_to_mds_min_time_interval;
  std::vector<int> shortcode_to_ors_min_time_interval;

  std::map<std::pair<std::string, std::string>, std::vector<PnlCheckStruct>> shortcode_timeperiod_to_pnl_check_;
  // std::map<std::pair<std::string, std::string>, int> shortcode_timeperiod_to_pnl_checkout_time_;

  if (argc < 2) {
    std::cerr << " usage : Input file name containing the Symbols E.g ~/infracore_install/files/mkt_sec.txt [LOG] [Pnl "
                 "Drop Checks Filename]" << std::endl;
    exit(0);
  }

  std::string filename_input(argv[1]);

  int prog_id = -100;

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();

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

    std::cout << " UNDERLYING " << tokens[0] << std::endl;

    std::string underlying = std::string(tokens[0]);
    HFSAT::VectorUtils::UniqueVectorAdd(underlying_list, underlying);

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

  HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal()).LoadNSESecurityDefinitions();
  bool is_nse_added = true;

  for (auto underlying : underlying_list) {
    std::cout << "\n\nUnderlying: " << underlying << std::endl;
    std::string fut = "NSE_" + underlying + "_FUT0";
    sec_list_vec.push_back(fut);

    std::vector<std::string> vec = HFSAT::NSESecurityDefinitions::GetAllOptionShortcodesForUnderlying(underlying);

    for (auto shc : vec) {
      std::cout << shc << " , ";
    }
    sec_list_vec.insert(sec_list_vec.end(), vec.begin(), vec.end());
  }

  // ================================================================
  HFSAT::BulkFileWriter* p_binary_log_ = NULL;

  // Added functionality to alternatively log data in binary.
  if (argc > 2 && !strncmp(argv[2], "LOG", strlen("LOG"))) {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/our_extended_ba_mkt_book_binary_log."
                << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();

    p_binary_log_ = new HFSAT::BulkFileWriter(logfilename_.c_str(), 4 * 1024,
                                              std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
  }
  // ================================================================

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    exchange_symbol_vec.push_back(HFSAT::ExchangeSymbolManager::GetExchSymbol(sec_list_vec[ii]));
  }

  HFSAT::DebugLogger dbglogger_(1024000, 1);
  // setup DebugLogger
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/our_extended_ba_mkt_book." << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  if (p_binary_log_) {
    dbglogger_ << "Logging data in binary mode.\n";
  } else {
    dbglogger_ << "Not logging data in binary mode\n";
  }
  dbglogger_.DumpCurrentBuffer();

  HFSAT::Watch watch_(dbglogger_, tradingdate_);

  HFSAT::ShortcodeRequestHelper shc_req_helper(prog_id);
  global_shc_request_helper = &shc_req_helper;
  shc_req_helper.AddShortcodeListToListen(sec_list_vec);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++)
    sec_name_indexer_.AddString(exchange_symbol_vec[ii], sec_list_vec[ii]);

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

  if (curr_location_ == HFSAT::kTLocBMF || curr_location_ == HFSAT::kTLocCFE || curr_location_ == HFSAT::kTLocTMX ||
      curr_location_ == HFSAT::kTLocM1 || curr_location_ == HFSAT::kTLocFR2 || curr_location_ == HFSAT::kTLocNSE ||
      curr_location_ == HFSAT::kTLocSYD) {
    USE_COMBINED_SOURCE = true;
  }

  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;

  std::map<std::string, HFSAT::ORSMessageLiveSource*> shortcode_ors_live_source_map_;

  HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map_ = HFSAT::sid_to_security_market_view_map();
  ///< Unique Instance of map from shortcode to p_smv_

  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();

  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    HFSAT::ExchSource_t _this_exch_source_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(sec_list_vec[ii], HFSAT::DateTime::GetCurrentIsoDateLocal());
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

  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_(
      dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer);

  for (unsigned int ii = 0; ii < (unsigned int)sec_list_vec.size(); ii++) {
    HFSAT::ExchSource_t _this_exch_source_ =
        HFSAT::SecurityDefinitions::GetContractExchSource(sec_list_vec[ii], HFSAT::DateTime::GetCurrentIsoDateLocal());
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
      HFSAT::MDS_MSG::RTS, (void*)((HFSAT::PriceLevelGlobalListener*)(&indexed_cme_market_view_manager_)), &watch_);
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

  // Since NSE requires some files to load which may/may not be present on a given server
  if (true == is_nse_added) {
    combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
        HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager_)), &watch_);
  }

  std::map<std::string, HFSAT::PromOrderManager*> p_prom_order_manager_map;
  std::map<std::string, HFSAT::PromPNL*> p_prom_pnl_map_;
  //  std::map < std::string, HFSAT::BaseOrderManager * > p_base_order_manager_map ;

  HFSAT::BulkFileWriter bulkwriter("/spare/local/logs/alllogs/our_extended_BULK.log");

  if (USE_COMBINED_SOURCE) {
    combined_mds_messages_shm_processor_.AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY, &watch_,
                                                                        sid_to_smv_ptr_map_);
  }

  // Event manager: display next event in OEBU
  HFSAT::EconomicEventsManager economic_events_manager_(dbglogger_, watch_);
  const std::vector<HFSAT::EventLine>& events_of_the_day_ = economic_events_manager_.events_of_the_day();
  std::multimap<time_t, HFSAT::EventLine> event_time_map;
  unsigned int num_events_today_ = events_of_the_day_.size();
  for (auto i = 0u; i < num_events_today_; i++) {
    event_time_map.insert(std::make_pair(events_of_the_day_[i].event_time_, events_of_the_day_[i]));
    // event_time_map[events_of_the_day_[i].event_time_]=events_of_the_day_[i];
    // printf("Event: %s\n",events_of_the_day_[i].event_text_);
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
