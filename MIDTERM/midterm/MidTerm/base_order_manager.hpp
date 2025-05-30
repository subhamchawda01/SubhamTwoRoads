// =====================================================================================
//
//       Filename:  base_order_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Cre  03/18/2016 10:31:11 AM
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
#include "midterm/MidTerm/mid_term_order_routing_defines.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
// Include all algos which we want to use
#include "midterm/MidTerm/TWAP.hpp"
#include "midterm/MidTerm/VWAP.hpp"
#include "midterm/MidTerm/PVOL.hpp"
#include "midterm/MidTerm/simple_trend_execution_for_basket.hpp"

namespace MIDTERM {
struct ScheduleStatus {
  int lots;
  ExecutionLogic *obj;
  bool has_object;
  bool is_executing;
  std::vector<std::pair<int, double>> executions;
  int lotsize;
  double last_traded_price;
  double commission_per_unit;
  // Stringent check to call EndOfTradeAnalysis() only when all orders executed
  // and reply received from exchange
  int orders_sent;
  int orders_received;

  ScheduleStatus() {
    lots = 0;
    has_object = false;
    is_executing = false;
    last_traded_price = 0;
    commission_per_unit = 0;
  }
};

// This is the initial class which handles orders from live/sim
// through OnClientRequest() and does netting
// Also responsible for injecting orders to algo
// Also does the logging to tradefile/portfoliofile etc.
class BaseOrderManager : public HFSAT::Utils::TCPServerSocketListener,
                         public HFSAT::SecurityMarketViewChangeListener,
                         public HFSAT::TimePeriodListener {
public:
  HFSAT::Lock mkt_lock_;
  Mode operating_mode_;
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map_;
  char *packet_buffer_;
  std::string curr_buffer_;
  bool is_ready_;
  int timeperiod_counter_;
  int logperiod_counter_;
  int synthetic_product_counter_;
  map<key_t, CurrentPortfolioStatus>
      portfolio_; // Uniquely mapped to <strategy,sec_id>
  map<string, ScheduleStatus> raw_orders_;
  map<string, double> last_traded_price_map_;
  BaseTrader &trader_;
  BaseAlgoManager &algo_manager_;

public:
  BaseOrderManager(Mode mode, HFSAT::DebugLogger &dbglogger,
                   HFSAT::Watch &watch, BaseTrader &trader,
                   BaseAlgoManager &algo_manager)
      : mkt_lock_(), operating_mode_(mode), dbglogger_(dbglogger),
        watch_(watch),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        sid_to_smv_ptr_map_(HFSAT::sid_to_security_market_view_map()),
        packet_buffer_(new char[NSE_MIDTERM_DATA_BUFFER_LENGTH]),
        curr_buffer_(std::move(curr_buffer_)), is_ready_(false),
        timeperiod_counter_(0), logperiod_counter_(0),
        synthetic_product_counter_(0), trader_(trader),
        algo_manager_(algo_manager) {}

  void Initialize(Mode mode) {
    // Simply return when in logger mode
    if (Mode::kNSELoggerMode == mode)
      return;
    // Open logger file in Server mode, not in SIM mode
    if (Mode::kNSEServerMode == mode) {
      std::ostringstream t_temp_oss;
      t_temp_oss << MEDIUM_TERM_LOG_PATH << "Order_Server_Logs/activity_"
                 << HFSAT::DateTime::GetCurrentIsoDateLocal() << ".log";
      dbglogger_.OpenLogFile(t_temp_oss.str().c_str(), std::ofstream::app);
      dbglogger_ << "Opened LogFile in Append Mode \n";
      dbglogger_.DumpCurrentBuffer();
    }
    // Read execution parameters from config containing exec params
    dbglogger_ << "Reading execution parameters from config..\n";
    ReadExecutionParametersFromCfg();
    dbglogger_ << "Done..." << DBGLOG_ENDL_FLUSH;

    // Get commissions, lotsizes and populate portfolio
    dbglogger_
        << "Getting commissions and lotsizes for products we're trading...\n";
    typedef map<string, ScheduleStatus>::iterator it_;
    for (it_ iterator = raw_orders_.begin(); iterator != raw_orders_.end();
         iterator++) {
      string shortcode_string = iterator->first;

      if (!raw_orders_[shortcode_string].has_object)
        continue;

      // Get commission
      vector<string> shortcodes_ =
          GetShortcodesFromProductRepresentation(shortcode_string);
      for (auto shortcode_ : shortcodes_) {
        // Get aggregate commission value
        raw_orders_[shortcode_string].commission_per_unit +=
            HFSAT::NSESecurityDefinitions::GetNSECommission(shortcode_);
        // Initialize map which contains the last traded price of each shortcode
        // Add logic here to initialize the actual last traded price of a
        // product, rather than 0
        if (last_traded_price_map_.find(shortcode_) ==
            last_traded_price_map_.end()) {
          double price_ =
              HFSAT::NSESecurityDefinitions::GetLastClose(shortcode_);
          last_traded_price_map_.insert(make_pair(shortcode_, price_));
        }
      }

      // Get lotsize, currently support trading in single lot size strategy
      int32_t today_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
      // Possible to pick any product to get the lotsize
      raw_orders_[shortcode_string].lotsize =
          HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcodes_[0],
                                                              today_);
      dbglogger_ << shortcode_string << "\t"
                 << raw_orders_[shortcode_string].lotsize << "\n";
    }
    dbglogger_ << "Done..." << DBGLOG_ENDL_FLUSH;

    // Also get the positions from the file where we dump whenever we are idle
    LoadPortfolioMap(MEDIUM_TERM_LOG_PATH);
    LoadPortfolioMap(MEDIUM_TERM_OPTION_LOG_PATH);
    PrintPortfolio();
    // Subscribe to time which listens once every second
    watch_.subscribe_BigTimePeriod(this);
  }

  BaseOrderManager(BaseOrderManager const &disabled_copy_constructor) = delete;

public:
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnClientRequest(int32_t client_fd, char *buffer, uint32_t const &length);
  void SendOrderRequest();

  void ReadExecutionParametersFromCfg();
  void InitializeVanillaAlgo(const string &, const string &, const int &,
                             const int &, const int &, const int &,
                             const double &);
  void LoadPortfolioMap(string);
  void OnTradePrint(unsigned int const security_id,
                    HFSAT::TradePrintInfo const &trade_print_info,
                    HFSAT::MarketUpdateInfo const &market_update_info);
  void OnMarketUpdate(unsigned int const security_id,
                      HFSAT::MarketUpdateInfo const &market_update_info){};
  void ProcessInitialization(string, string, double, int, int, char, double);
  void ProcessFreshOrder(string, string, double, int, int, char, double);
  void ProcessStopLossUpdate(string, string, double, int, double);
  void ProcessFutureRollover(string, string, double, int, int, char);
  void GetOrdersAfterNetting();
  int GetTotalVolumeChangeOfPortfolio(string);
  void OnEndOfTradeAnalysis(string, vector<pair<int, double>>);
  void LogTradeFile(int, string, string, int, int, double, double, int, double,
                    string);
  void LogPositionFile(int, string, string, int, double, double, double);
  void CompleteLog();
  void PrintPortfolio();
  void PrintOrders();
  double GetLastTradedPriceForShortcode(string &);
  string GetLogPathFromStrategy(string &);
  string GetTickerFromShortcode(string &);

  static int GetStepSize(string product);
  static int GetStepRatio(string product);
  static vector<string>
  GetShortcodesFromProductRepresentation(string internal_product_code);
  vector<int>
  GetSecurityIDsFromProductRepresentation(string internal_product_code);
  static string GetATMCall(string &);
  static string GetATMPut(string);

  void MarketLock() { mkt_lock_.LockMutex(); }
  void MarketUnLock() { mkt_lock_.UnlockMutex(); }
  void Sleep(int n) { sleep(n); }
};
}
