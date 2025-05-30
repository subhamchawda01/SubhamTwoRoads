// =====================================================================================
//
//       Filename:  base_order_manager.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  Thursday 18 March 2016 02:50:58  GMT
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
#include "midterm/MidTerm/base_order_manager.hpp"
#include "midterm/MidTerm/base_algo_manager.hpp"
namespace MIDTERM {

// This function is to process trades being sent from the strategy
// Function will be called only at the end of a 15-min bar for, eg
// Will run n times if there are n trades
// Order could be a buy/sell order or an update in the stoploss
void BaseOrderManager::OnClientRequest(int32_t client_fd, char *buffer,
                                       uint32_t const &length) {
  // Use lock so that a request from client is completely processed
  MarketLock();

  int i = 0;
  while (true) {
    // Read more into buffer till we encounter ^
    // int i = 0;
    if (buffer[i] == NULL)
      break;
    while (true) {
      curr_buffer_ += buffer[i];
      i++;
      if (buffer[i] == '^') {
        is_ready_ = true;
        i++;
        break;
      }
    }
    // Still not ready we need to return and wait for next OnClientRequest to be
    // called
    // This is done because we might have received incomplete packets
    if (!is_ready_)
      continue;

    // If we reach here the buffer is ready we have a complete trade
    dbglogger_ << "-----PRINTING TRADE FROM STRATEGY-------\n";
    dbglogger_ << curr_buffer_ << DBGLOG_ENDL_FLUSH;

    std::string trade_ = curr_buffer_;
    curr_buffer_ = "";
    is_ready_ = false;

    std::vector<string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(trade_, '|', tokens_);

    // order info from strategy
    // this is the strategy name, like NIFTY_BOLLINGER_15
    string strategy_ = tokens_[0];
    string instrument_ = tokens_[1];
    string inst_type_ = tokens_[2];
    int time_ = watch_.tv().tv_sec;
    // this is actually the magnitude of the position according the the strategy
    // this is in terms of number of lots
    int size_ = atoi(tokens_[4].c_str());
    string exp_num_ = tokens_[5];
    string strike_ = tokens_[6];
    double ref_px_ = atof(tokens_[7].c_str());
    double stop_ = atof(tokens_[9].c_str());
    char side_ = tokens_[10].c_str()[0];

    string shortcode_string =
        "NSE_" + instrument_ + "_" + inst_type_ + exp_num_;
    if (inst_type_ == "STRADDLE") {
      shortcode_string += "_" + strike_;
    }

    // Check if fresh order for buy/sell or an update in stoploss
    if (side_ == 'I')
      ProcessInitialization(shortcode_string, strategy_, stop_, size_, time_,
                            side_, ref_px_);
    else if (side_ == 'B' || side_ == 'S')
      ProcessFreshOrder(shortcode_string, strategy_, stop_, size_, time_, side_,
                        ref_px_);
    else if (side_ == 'U')
      ProcessStopLossUpdate(shortcode_string, strategy_, stop_, size_, ref_px_);
    else if (side_ == 'F')
      ProcessFutureRollover(shortcode_string, strategy_, stop_, size_, time_,
                            side_);
    else {
      dbglogger_ << "ERROR" << DBGLOG_ENDL_FLUSH;
      DBGLOG_CLASS_FUNC_LINE_FATAL << " CANNOT UNDERSTAND SIDE FROM STRATEGY.."
                                   << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
  MarketUnLock();

  // wait till requests from each client processed
  // we could have had logic so that netting is called only once
  // but now gets called n times for n client requests but does not matter
  Sleep(3);

  // portfolio_ is ready at this point
  // Do netting
  MarketLock();
  GetOrdersAfterNetting();
  MarketUnLock();
}

// initialization
// currently the size_ is always positive
// in the case of Initialization it could be +ve/-ve
void BaseOrderManager::ProcessInitialization(string shortcode_string,
                                             string strategy, double stop_,
                                             int size_, int time_, char side_,
                                             double ref_px_) {
  key_t key_ = make_pair(strategy, shortcode_string);
  if (portfolio_.find(key_) != portfolio_.end()) {
    dbglogger_ << "ORDER FOUND IN PORTFOLIO WHEN INITIALIZING..."
               << DBGLOG_ENDL_FLUSH;
    CurrentPortfolioStatus curr_portfolio_ = portfolio_[key_];
    curr_portfolio_.stoploss_price = stop_;
    curr_portfolio_.has_stoploss = (stop_ <= INVALID_PRICE ? false : true);
    curr_portfolio_.sent_time = time_;
    curr_portfolio_.position = size_;
    curr_portfolio_.orders_to_place = size_ - portfolio_[key_].position;
    curr_portfolio_.updated = false;
    curr_portfolio_.init_price =
        (ref_px_ <= INVALID_PRICE)
            ? raw_orders_[shortcode_string].last_traded_price
            : ref_px_;
    portfolio_[key_] = curr_portfolio_;
  } else {
    dbglogger_ << "DID NOT FIND THE ORDER IN PORTFOLIO WHILE INITIALIZING..."
               << DBGLOG_ENDL_FLUSH;
    CurrentPortfolioStatus curr_portfolio_;
    curr_portfolio_.stoploss_price = stop_;
    curr_portfolio_.has_stoploss = (stop_ <= INVALID_PRICE ? false : true);
    curr_portfolio_.sent_time = time_;
    curr_portfolio_.position = size_;
    curr_portfolio_.init_position = 0;
    curr_portfolio_.orders_to_place = size_;
    curr_portfolio_.updated = false;
    curr_portfolio_.init_price =
        (ref_px_ <= INVALID_PRICE)
            ? raw_orders_[shortcode_string].last_traded_price
            : ref_px_;
    portfolio_.insert(make_pair(key_, curr_portfolio_));
  }
  DBGLOG_DUMP;
}

// buy/sell order
void BaseOrderManager::ProcessFreshOrder(string shortcode_string,
                                         string strategy, double stop_,
                                         int size_, int time_, char side_,
                                         double ref_px_) {
  // Doing this as a sanity means
  if (size_ < 0) {
    size_ = abs(size_);
    DBGLOG_CLASS_FUNC_LINE_FATAL << "ALERT -> ORDER SIZE IS NEGATIVE.."
                                 << DBGLOG_ENDL_FLUSH;
  }

  // portfolio_ contains the entire mapping from <strategy,sec_id> to
  // CurrentPortfolioStatus
  key_t key_ = make_pair(strategy, shortcode_string);

  // portfolio_ contains the key_
  // just check that the orders to place is initially 0
  // this is because we dump orders to place to raw_orders_
  // old portfolio position set to 0 when stop loss triggered
  // in case of flip, no stop loss so simply execute diff in position
  if (portfolio_.find(key_) != portfolio_.end()) {
    CurrentPortfolioStatus old_portfolio_ = portfolio_[key_];
    // get the position as +ve, -ve depending on bought/sold according to
    // strategy
    if (side_ == 'B')
      old_portfolio_.position = size_;
    else
      old_portfolio_.position = 0 - size_;
    // stoploss price
    old_portfolio_.stoploss_price = stop_;
    // stoploss price set to <=0 by default when no stoploss provided
    if (stop_ <= INVALID_PRICE)
      old_portfolio_.has_stoploss = false;
    else
      old_portfolio_.has_stoploss = true;
    // time at which order sent, or recieved by this exec
    old_portfolio_.sent_time = time_;
    // This is the price of the market when the trade was requested from
    // strategy
    old_portfolio_.init_price =
        (ref_px_ <= INVALID_PRICE)
            ? raw_orders_[shortcode_string].last_traded_price
            : ref_px_;
    old_portfolio_.updated = false;

    if (old_portfolio_.orders_to_place != 0) {
      dbglogger_ << "ALERT -> Orders to place is not 0..." << DBGLOG_ENDL_FLUSH;
    }

    // Handle fresh orders when stoploss is executing
    if (old_portfolio_.stoploss_executing) {
      int timestamp_ = watch_.tv().tv_sec;
      int exec_time = timestamp_ - old_portfolio_.sent_time;
      dbglogger_ << "ALERT: LOG TO TRADEFILE WILL BE INACCURATE -> PLS FIX AS "
                    "STOPLOSS WAS EXECUTING"
                 << DBGLOG_ENDL_FLUSH;
      LogTradeFile(timestamp_, strategy, shortcode_string,
                   old_portfolio_.init_position, 0,
                   raw_orders_[shortcode_string].last_traded_price,
                   raw_orders_[shortcode_string].last_traded_price, exec_time,
                   0, "TRIGGER");
      old_portfolio_.init_position = 0;
      old_portfolio_.stoploss_executing = false;
    }
    old_portfolio_.orders_to_place =
        old_portfolio_.position - old_portfolio_.init_position;
    portfolio_[key_] = old_portfolio_;

  }
  // case when fresh entry to map
  // should never happen as we poll strategy position when we start
  // so the portfolio should ideally be already present
  else {
    dbglogger_ << "NO PORTFOLIO MAPPING IN PROCESSFRESHORDER..."
               << DBGLOG_ENDL_FLUSH;
    dbglogger_ << "KEY: ( " << key_.first << ", " << key_.second << " )"
               << "\n";

    DBGLOG_CLASS_FUNC_LINE_FATAL << "OLD PORTFOLIO DOES NOT HAVE A MAPPING FOR "
                                    "THE ORDER IN PROCESS FRESH ORDER.. ( "
                                 << strategy << ", " << shortcode_string << " )"
                                 << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    dbglogger_ << "ADDING ORDER IN PORTFOLIO EVEN WHEN THIS IS NOT THE IDEAL "
                  "THING TO DO..."
               << DBGLOG_ENDL_FLUSH;
    CurrentPortfolioStatus curr_portfolio_;
    curr_portfolio_.stoploss_price = stop_;
    curr_portfolio_.has_stoploss = (stop_ <= INVALID_PRICE ? false : true);
    curr_portfolio_.sent_time = time_;
    curr_portfolio_.position = (side_ == 'B') ? size_ : -size_;
    curr_portfolio_.init_position = 0;
    curr_portfolio_.orders_to_place = curr_portfolio_.position;
    curr_portfolio_.updated = false;
    curr_portfolio_.init_price =
        (ref_px_ <= INVALID_PRICE)
            ? raw_orders_[shortcode_string].last_traded_price
            : ref_px_;
    portfolio_.insert(make_pair(key_, curr_portfolio_));
  }
}

// update order
void BaseOrderManager::ProcessStopLossUpdate(string shortcode_string,
                                             string strategy, double stop_,
                                             int size_, double ref_px_) {
  key_t key_ = make_pair(strategy, shortcode_string);
  if (portfolio_.find(key_) != portfolio_.end()) {
    if (std::abs(portfolio_[key_].position) == std::abs(size_)) {
      portfolio_[key_].updated = true;
      portfolio_[key_].stoploss_price = stop_;
      portfolio_[key_].has_stoploss = ((stop_ <= INVALID_PRICE) ? false : true);
    } else {
      dbglogger_ << "ERROR IN STOPLOSS UPDATE..." << DBGLOG_ENDL_FLUSH;
      DBGLOG_CLASS_FUNC_LINE_FATAL
          << "Encountered a tuple with incorrect(maybe 0) position when an "
             "update stoploss order arrived.."
          << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  } else {
    dbglogger_ << "ERROR IN STOPLOSS UPDATE NO PORTFOLIO MAPPING..."
               << DBGLOG_ENDL_FLUSH;
    dbglogger_ << "KEY: ( " << key_.first << ", " << key_.second << " )"
               << "\n";
    PrintPortfolio();
    DBGLOG_CLASS_FUNC_LINE_FATAL << "NO PORTFOLIO MAPPING WHEN A STOP LOSS "
                                    "UPDATE ORDER HAS ARRIVED... ( "
                                 << strategy << " " << shortcode_string << " )"
                                 << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
}

// This function must poll its status with old shortcode
// Function should receive 'F' to enter this
// Strategy must poll its position when doing the future rollover
// Equivalent position will be taken in the next future
void BaseOrderManager::ProcessFutureRollover(string shortcode_string,
                                             string strategy, double stop_,
                                             int size_, int time_, char side_) {

  key_t key_ = make_pair(strategy, shortcode_string);

  // get the shortcode for the expiring future(FUT0)
  string old_shortcode = shortcode_string;
  // get the shortcode for the next future(FUT1)
  string new_shortcode =
      old_shortcode.substr(0, old_shortcode.size() - 1) + "1";

  if (portfolio_.find(key_) != portfolio_.end()) {
    dbglogger_ << "Processing Future Rollover for: " << old_shortcode
               << " to: " << new_shortcode << '\n';
    // unwind position in old and take equal position in new
    CurrentPortfolioStatus old_portfolio_ = portfolio_[key_];
    CurrentPortfolioStatus new_portfolio_; // For the next expiry

    dbglogger_ << "Editing the old portfolio...\n";
    // Make old portfolio semi dead
    old_portfolio_.stoploss_price = -1;
    old_portfolio_.init_price = raw_orders_[shortcode_string].last_traded_price;
    old_portfolio_.has_stoploss = false;
    old_portfolio_.sent_time = time_;
    old_portfolio_.position = 0;
    old_portfolio_.updated = false;
    old_portfolio_.type = "ROLLOVER";
    portfolio_[key_] = old_portfolio_;
    dbglogger_ << "DONE...\n";

    dbglogger_ << "Editing the new portfolio...\n";
    // New portfolio
    new_portfolio_.NAV = 0;
    new_portfolio_.Realized_PNL = 0;
    new_portfolio_.stoploss_price = stop_;
    new_portfolio_.init_price = raw_orders_[shortcode_string].last_traded_price;
    new_portfolio_.has_stoploss = ((stop_ <= INVALID_PRICE) ? false : true);
    new_portfolio_.sent_time = time_;
    new_portfolio_.position = size_;
    new_portfolio_.orders_to_place = 0;
    new_portfolio_.updated = false;
    new_portfolio_.type = "ROLLOVER";
    dbglogger_ << "DONE...\n";

    // Place into raw_orders_ for execution
    raw_orders_[old_shortcode].lots -= old_portfolio_.init_position;
    raw_orders_[new_shortcode].lots += new_portfolio_.position;
    PrintPortfolio();

    // can happen when this is not the first ever rollover
    if (portfolio_.find(make_pair(strategy, new_shortcode)) != portfolio_.end())
      portfolio_[make_pair(strategy, new_shortcode)] = new_portfolio_;
    else
      portfolio_.insert(
          make_pair(make_pair(strategy, new_shortcode), new_portfolio_));

  } else {
    dbglogger_ << "ALERT -> ERROR IN FUT ROLLOVER NO PORTFOLIO MAPPING..."
               << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
}

// net all orders for a particular sec_id
void BaseOrderManager::GetOrdersAfterNetting() {
  // dbglogger_ << "---------PRINTING OVERALL PORTFOLIO-------" <<
  // DBGLOG_ENDL_FLUSH;
  // PrintPortfolio();
  // dbglogger_ << "-------------------DONE----------------cd bas	---" <<
  // DBGLOG_ENDL_FLUSH;
  typedef map<key_t, CurrentPortfolioStatus>::iterator it_;
  for (it_ iterator = portfolio_.begin(); iterator != portfolio_.end();
       iterator++) {
    // iterator->first is the pair of <strategy,sec_id>
    string shortcode_string = iterator->first.second;
    CurrentPortfolioStatus curr_portfolio_ = iterator->second;
    // Don't net orders that have been updated
    if (curr_portfolio_.updated || (curr_portfolio_.orders_to_place == 0))
      continue;

    dbglogger_ << "--PRINTING PORTFOLIO TRADE BEFORE NETTING--\n";
    dbglogger_ << "STRATEGY\t" << iterator->first.first << "\n";
    dbglogger_ << "SHORTCODE\t" << iterator->first.second << "\n";
    dbglogger_ << iterator->second.ToString();
    dbglogger_ << "-------------------DONE-------------------"
               << DBGLOG_ENDL_FLUSH;
    // place the orders into raw_order_
    // orders to place for the curr portfolio becomes 0
    raw_orders_[shortcode_string].lots += curr_portfolio_.orders_to_place;
    portfolio_[iterator->first].orders_to_place = 0;
    portfolio_[iterator->first].type = "MARKET";
  }

  SendOrderRequest();
}

// Function to log every minute to the portfolio file
void BaseOrderManager::OnTimePeriodUpdate(const int num_pages_to_add_) {
  logperiod_counter_++;
  if (logperiod_counter_ == LOGPERIOD_LISTENER_SIZE) {
    CompleteLog();
    logperiod_counter_ = 0;
  }
};

// Simply injects the order into the algo
void BaseOrderManager::SendOrderRequest() {
  typedef map<string, ScheduleStatus>::iterator it_;
  for (it_ iterator = raw_orders_.begin(); iterator != raw_orders_.end();
       iterator++) {
    string shortcode_string = iterator->first;
    if (!raw_orders_[shortcode_string].has_object)
      continue;
    if (raw_orders_[shortcode_string].lots != 0) {
      raw_orders_[shortcode_string].obj->AddOrder(
          raw_orders_[shortcode_string].lots);
      raw_orders_[shortcode_string].lots = 0;
      dbglogger_ << "ORDERS SENT FROM ORDER MANAGER FOR: " << shortcode_string
                 << DBGLOG_ENDL_FLUSH;
    }
  }
};

// Read exec params from config
// Also creates an algo instance per shortcode
void BaseOrderManager::ReadExecutionParametersFromCfg() {
  // Read from Config
  string cfgPath = string(MEDIUM_TERM_LOG_PATH) + "NSE_execution_logic.cfg";
  dbglogger_ << cfgPath << "\n";
  ifstream execfile;
  execfile.open(cfgPath.c_str());
  if (!execfile.is_open()) {
    DBGLOG_CLASS_FUNC_LINE_FATAL
        << "FAILED TO OPEN EXECUTION CONFIG FOR READING : " << cfgPath
        << " ERROR : " << strerror(errno) << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }

  string line;
  while (getline(execfile, line)) {
    if (line.empty())
      continue;
    if (line.substr(0, 1) == "#")
      continue;

    dbglogger_ << line << "\n";
    vector<string> tokens;
    HFSAT::PerishableStringTokenizer::StringSplit(line, ' ', tokens);

    string shortcode_string = tokens[0];
    string execution_algorithm = tokens[1];
    // SHOULD BE POSSIBLE TO ADD SOME SANITY CHECKS ON THE VALUES OF THE
    // FOLLOWING? - RK
    double participation_rate = atof(tokens[2].c_str());
    int max_time_to_execute = atoi(tokens[3].c_str());
    int min_size_to_execute = atoi(tokens[4].c_str());
    int execution_interval = atoi(tokens[5].c_str());

    if (execution_algorithm == "TWAP" || execution_algorithm == "VWAP" ||
        execution_algorithm == "PVOL") {

      int lotsize_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(
          GetShortcodesFromProductRepresentation(shortcode_string)[0],
          HFSAT::DateTime::GetCurrentIsoDateLocal());
      InitializeVanillaAlgo(shortcode_string, execution_algorithm, lotsize_,
                            max_time_to_execute, min_size_to_execute,
                            execution_interval, participation_rate);

    } else if (execution_algorithm == "Simple_Trend_Execution_For_Basket") {

      vector<string> shortcodes =
          GetShortcodesFromProductRepresentation(shortcode_string);
      for (auto i : shortcodes) {
        std::vector<string> tokens_;
        HFSAT::PerishableStringTokenizer::StringSplit(i, '_', tokens_);
        // tokens like [ NSE, BANKNIFTY, C0, I9, M2(optional) ]
        int num_ = atoi(tokens[3].substr(tokens[3].size() - 1, 1).c_str());
        if (num_ >= 5) {
          dbglogger_ << "--------------------WARNING--------------------\n";
          dbglogger_ << "Shortcode " << i << " found while parsing "
                     << shortcode_string << "...Will not be subscribed...\n";
          dbglogger_ << "===============================================\n";
          // Continue, we can't subscribe to the shortcode
          continue;
        }
        dbglogger_ << "For " << shortcode_string << ": " << '\t' << i << '\t'
                   << sec_name_indexer_.GetIdFromString(i) << DBGLOG_ENDL_FLUSH;
      }
      int lotsize_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(
          GetShortcodesFromProductRepresentation(shortcode_string)[0],
          HFSAT::DateTime::GetCurrentIsoDateLocal());
      raw_orders_.insert(make_pair(shortcode_string, ScheduleStatus()));
      raw_orders_[shortcode_string].obj = new SimpleTrendExecutionForBasket(
          sid_to_smv_ptr_map_, dbglogger_, watch_, shortcode_string, shortcodes,
          lotsize_, HFSAT::kPriceTypeMktSizeWPrice, max_time_to_execute,
          min_size_to_execute, execution_interval, trader_, algo_manager_,
          operating_mode_);
      raw_orders_[shortcode_string].has_object = true;
      for (auto i : shortcodes) {
        sid_to_smv_ptr_map_[sec_name_indexer_.GetIdFromString(i)]
            ->subscribe_L1_Only(raw_orders_[shortcode_string].obj);
        sid_to_smv_ptr_map_[sec_name_indexer_.GetIdFromString(i)]
            ->subscribe_price_type(raw_orders_[shortcode_string].obj,
                                   HFSAT::kPriceTypeMktSizeWPrice);
      }
      algo_manager_.SubscribeOrderListener(raw_orders_[shortcode_string].obj);
      dbglogger_ << "Added: " << shortcode_string << "\n";
      dbglogger_ << "===\n";
    } else {
      DBGLOG_CLASS_FUNC_LINE_FATAL << "EXECUTION ALGO NOT SUPPORTED.. "
                                   << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }
  }
  // End of Read From Config
}

// Modular function to initialize vanilla product traders
void BaseOrderManager::InitializeVanillaAlgo(
    const string &shortcode_string, const string &type, const int &lotsize,
    const int &max_time_to_execute, const int &min_size_to_execute,
    const int &execution_interval, const double &participation_rate) {
  raw_orders_.insert(make_pair(shortcode_string, ScheduleStatus()));

  if (type == "TWAP") {
    raw_orders_[shortcode_string].obj =
        new TWAP(dbglogger_, watch_, shortcode_string, lotsize,
                 max_time_to_execute, min_size_to_execute, execution_interval,
                 trader_, algo_manager_, operating_mode_);
  } else if (type == "VWAP") {
    raw_orders_[shortcode_string].obj = new VWAP(
        dbglogger_, watch_, shortcode_string, lotsize, max_time_to_execute,
        execution_interval, trader_, algo_manager_, operating_mode_);
  } else if (type == "PVOL") {
    raw_orders_[shortcode_string].obj = new PVOL(
        dbglogger_, watch_, shortcode_string, lotsize, participation_rate,
        execution_interval, trader_, algo_manager_, operating_mode_);
  }

  raw_orders_[shortcode_string].has_object = true;
  // Subscribe algo to L1
  sid_to_smv_ptr_map_[sec_name_indexer_.GetIdFromString(shortcode_string)]
      ->subscribe_L1_Only(raw_orders_[shortcode_string].obj);
  // Subscribe algo to order listener
  algo_manager_.SubscribeOrderListener(raw_orders_[shortcode_string].obj);
  dbglogger_ << "Added: " << shortcode_string << "\n";
  dbglogger_ << "===\n";
}

// Reconcile the portfolio_ from dumped data
void BaseOrderManager::LoadPortfolioMap(string path) {
  string reconPath = path + string("reconciliation_file");
  ifstream reconfile_;
  reconfile_.open(reconPath.c_str());
  string line;
  while (getline(reconfile_, line)) {
    if (line.substr(0, 1) == "#")
      continue;
    if (line.empty())
      continue;
    vector<string> tokens;
    HFSAT::PerishableStringTokenizer::StringSplit(line, '|', tokens);

    string strategy_ = tokens[0];
    string shortcode_string = tokens[1];
    CurrentPortfolioStatus curr_portfolio_;
    curr_portfolio_.NAV = atof(tokens[5].c_str());
    curr_portfolio_.Realized_PNL = atof(tokens[6].c_str());
    curr_portfolio_.stoploss_price = atof(tokens[3].c_str());
    curr_portfolio_.last_execution_price = atof(tokens[4].c_str());
    curr_portfolio_.has_stoploss =
        curr_portfolio_.stoploss_price > 0 ? true : false;
    curr_portfolio_.sent_time = -1;
    curr_portfolio_.position = atoi(tokens[2].c_str());
    curr_portfolio_.init_position = curr_portfolio_.position;
    curr_portfolio_.orders_to_place = 0;
    curr_portfolio_.updated = false;

    key_t key_ = make_pair(strategy_, shortcode_string);
    if (portfolio_.find(key_) != portfolio_.end()) {
      portfolio_[key_] = curr_portfolio_;
    } else {
      portfolio_.insert(make_pair(key_, curr_portfolio_));
    }
  }
}

// Get total volume change of a portfolio by security
int BaseOrderManager::GetTotalVolumeChangeOfPortfolio(string shortcode_string) {
  typedef map<key_t, CurrentPortfolioStatus>::iterator it_;
  int total_vol_portfolio = 0;
  for (it_ iterator = portfolio_.begin(); iterator != portfolio_.end();
       iterator++) {
    key_t key_ = iterator->first;
    if (key_.second != shortcode_string)
      continue;
    total_vol_portfolio +=
        abs(portfolio_[key_].position - portfolio_[key_].init_position);
  }
  return total_vol_portfolio;
}

// This function is called when no executions are happening
// simply find the average price of each executions
// and calculate the VWAP
void BaseOrderManager::OnEndOfTradeAnalysis(
    string shortcode_string, vector<pair<int, double>> executions_) {
  int timestamp_ = watch_.tv().tv_sec;
  dbglogger_ << "End of trade analysis for: " << shortcode_string << "\n";

  double numerator = 0;
  double denominator = 0;
  std::vector<std::pair<int, double>> execs_ = executions_;
  if (execs_.size() == 0) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "NO EXECUTIONS FOR CALCULATING NAV!!"
                                 << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }

  for (uint32_t i = 0; i < execs_.size(); i++) {
    // dbglogger_ << "Execution: " << execs_[i].first << "\t" <<
    // execs_[i].second << DBGLOG_ENDL_FLUSH;
    numerator += double(execs_[i].first) * execs_[i].second;
    denominator += double(execs_[i].first);
  }

  double avg_price = numerator / denominator;

  // Volume executed * avg_price * comm factor
  double total_commissions =
      denominator * double(raw_orders_[shortcode_string].lotsize) * avg_price *
      (raw_orders_[shortcode_string].commission_per_unit);
  // std::abs sum of all position changes
  int total_volume_change = GetTotalVolumeChangeOfPortfolio(shortcode_string);

  // Now need to assign commissions in correct proportions
  typedef map<key_t, CurrentPortfolioStatus>::iterator it_;
  for (it_ iterator = portfolio_.begin(); iterator != portfolio_.end();
       iterator++) {
    key_t key_ = iterator->first;
    if (key_.second != shortcode_string)
      continue;
    int open_ = portfolio_[key_].init_position;
    int curr_ = portfolio_[key_].position;
    int traded_lots_ = curr_ - open_;
    // Assign commissions according to ratio of absolute vol executed
    double trade_commission =
        total_commissions *
        (double(std::abs(traded_lots_)) / (double(total_volume_change)));

    if (traded_lots_ == 0)
      continue; // Unhandled case when netting led to 0 executions. Handled as a
                // degenerate case
    // Case when our position increases in absolute terms
    else if (abs(curr_) > abs(open_) && curr_ * open_ >= 0) {
      portfolio_[key_].last_execution_price =
          double(open_) * portfolio_[key_].last_execution_price +
          double(traded_lots_) * avg_price;
      portfolio_[key_].last_execution_price /= double(curr_);
      // Need to subtract commission even when position is increasing
      portfolio_[key_].Realized_PNL -= trade_commission;
    }
    // Case when our position decreases but we don't actually flip
    else if (abs(curr_) < abs(open_) && curr_ * open_ >= 0) {
      int side_ = (open_ > 0) ? 1 : -1;
      portfolio_[key_].Realized_PNL +=
          double(side_ * abs(traded_lots_)) *
              double(raw_orders_[shortcode_string].lotsize) *
              (avg_price - portfolio_[key_].last_execution_price) -
          trade_commission;
      if (curr_ == 0) {
        portfolio_[key_].last_execution_price = 0;
      }
    }
    // Case when there is a flip in the position
    else if (curr_ * open_ < 0) {
      int side_ = (open_ > 0) ? 1 : -1;
      portfolio_[key_].Realized_PNL +=
          double(side_ * abs(open_)) *
              double(raw_orders_[shortcode_string].lotsize) *
              (avg_price - portfolio_[key_].last_execution_price) -
          trade_commission;
      portfolio_[key_].last_execution_price = avg_price;
    }

    // Only done when stoploss hit at open
    // In that case OnTradePrint not called so the init price is logged as 0
    if (portfolio_[key_].init_price == 0) {
      dbglogger_ << "CAUTION: init_price was set to 0...\n";
      portfolio_[key_].init_price = avg_price;
    }
    dbglogger_ << "Time is: " << watch_.tv().tv_sec << "\n";
    dbglogger_ << "Logging tradefile for: ( " << key_.first << ", "
               << shortcode_string << " )\n";

    int exec_time = timestamp_ - portfolio_[key_].sent_time;
    // Trade file contains only trades and can be used to calculate PNL
    double comm_by_lotsize =
        trade_commission / double(raw_orders_[shortcode_string].lotsize);
    LogTradeFile(timestamp_, key_.first, shortcode_string,
                 portfolio_[key_].init_position, portfolio_[key_].position,
                 portfolio_[key_].init_price, avg_price, exec_time,
                 comm_by_lotsize, portfolio_[key_].type);
    dbglogger_ << "Done...\n";

    dbglogger_ << "Dumping for reconciliation: ( " << key_.first << ", "
               << shortcode_string << " )\n";
    // STRATEGY|SHORTCODE|QUANTITY(LOTS)|STOP|LAST_EXECUTED_PRICE|NAV_INIT|Realized_PNL
    // Need to do this as we don't want to overcrowd trade/portfolio files with
    // stoploss/last_execution_price etc.
    ofstream file_;
    string outputpath =
        GetLogPathFromStrategy(key_.first) + string("reconciliation_file");
    file_.open(outputpath.c_str(), ios_base::app);
    file_ << key_.first << "|" << shortcode_string << "|"
          << portfolio_[key_].position << "|" << portfolio_[key_].stoploss_price
          << "|" << portfolio_[key_].last_execution_price << "|"
          << portfolio_[key_].NAV << "|" << portfolio_[key_].Realized_PNL
          << "\n";
    dbglogger_ << "Done...\n";

    portfolio_[key_].init_position = portfolio_[key_].position;
    portfolio_[key_].stoploss_executing = false;
  }
  raw_orders_[shortcode_string].is_executing = false;
  raw_orders_[shortcode_string].executions.clear();
}

void BaseOrderManager::LogTradeFile(int timestamp_, string strategy_,
                                    string shortcode_, int init_position_,
                                    int position_, double init_price_,
                                    double execution_price_,
                                    int execution_time_, double commission_,
                                    string type_) {
  ofstream file_;
  string outputpath = GetLogPathFromStrategy(strategy_) + string("tradefile");
  file_.open(outputpath.c_str(), ios_base::app);
  string side_ = ((position_ >= init_position_) ? "BUY" : "SELL");
  int qty_ = std::abs(position_ - init_position_);

  file_ << timestamp_ << "\t" << strategy_ << "\t" << shortcode_ << "\t"
        << side_ << "\t" << qty_ << "\t" << init_price_ << "\t"
        << execution_price_ << "\t" << execution_time_ << "\t" << commission_
        << "\t" << type_ << "\n";
}

// Function to log in readable fmt
void BaseOrderManager::LogPositionFile(int timestamp_, string strategy_,
                                       string shortcode_, int position_,
                                       double NAV_, double Realized_PNL_,
                                       double Unrealized_PNL_) {
  ofstream file_;
  ///
  string outputpath =
      GetLogPathFromStrategy(strategy_) + string("portfoliofile");
  file_.open(outputpath.c_str(), ios_base::app);
  const char *fmt_ = "%10d\t%20s\t%20s\t%4d\t%10.1f\t%10.1f\t%10.1f\n";

  char buffer_[200];
  snprintf(buffer_, sizeof(buffer_), fmt_, timestamp_, strategy_.c_str(),
           shortcode_.c_str(), position_, NAV_, Realized_PNL_, Unrealized_PNL_);
  file_ << buffer_ << "\n";
}

string BaseOrderManager::GetLogPathFromStrategy(string &strategy) {
  if (strategy.find("Shortgamma") != string::npos) {
    return MEDIUM_TERM_OPTION_LOG_PATH;
  }
  return MEDIUM_TERM_LOG_PATH;
}

// TODO:: INCOMPLETE FUNC IN CASE WE NEED MORE ACCURATE LAST CLOSE
double BaseOrderManager::GetLastTradedPriceForShortcode(string &shortcode) {
  std::ifstream bardata_file_;
  std::string filename_;
  std::string ticker_ = GetTickerFromShortcode(shortcode);
  // Case when we want option data
  if (HFSAT::NSESecurityDefinitions::IsOption(shortcode)) {
    filename_ = NSE_MIDTERM_OPTION_BARDATA_PATH + ticker_;
  } else {
    filename_ = NSE_MIDTERM_BARDATA_PATH + ticker_;
  }
  bardata_file_.open(filename_.c_str());

  // Now parse the file
  std::string line_;
  while (getline(bardata_file_, line_)) {

    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(line_, '\t', tokens_);

    std::vector<std::string> t_;
    HFSAT::PerishableStringTokenizer::StringSplit(tokens_[1], '_', t_);

    double strike_ = 0;
    std::string exp_;

    if (t_[0] != ticker_ || std::atof(t_[2].c_str()) != strike_ ||
        t_[3] != exp_) {
      continue;
    }
  }
  return 0;
}

std::string BaseOrderManager::GetTickerFromShortcode(std::string &shortcode) {
  std::vector<std::string> tokens;
  HFSAT::PerishableStringTokenizer::StringSplit(shortcode, '_', tokens);
  return tokens[1];
}

// Function called every minute on OnTimeUpdate()
void BaseOrderManager::CompleteLog() {
  typedef map<key_t, CurrentPortfolioStatus>::iterator it_;
  int _hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTime(watch_.tv().tv_sec);
  // Get minutes from midnight
  _hhmm_ = ((_hhmm_ / 100) * 60) + (_hhmm_ % 100) + 330;
  // Don't log after 1530 IST
  if (_hhmm_ >= (15 * 60 + 30)) {
    return;
  }

  for (it_ iterator = portfolio_.begin(); iterator != portfolio_.end();
       iterator++) {
    key_t key_ = iterator->first;
    CurrentPortfolioStatus curr_portfolio_ = iterator->second;
    int timestamp_ = watch_.tv().tv_sec;
    string shortcode_string_ = key_.second;
    double Unrealized_PNL = double(curr_portfolio_.init_position) *
                            double(raw_orders_[key_.second].lotsize) *
                            (raw_orders_[key_.second].last_traded_price -
                             curr_portfolio_.last_execution_price);
    LogPositionFile(timestamp_, key_.first, shortcode_string_,
                    curr_portfolio_.position, curr_portfolio_.NAV,
                    curr_portfolio_.Realized_PNL, Unrealized_PNL);
  }
}

// listen to each trade and check if a stoploss order gets triggered as a result
void BaseOrderManager::OnTradePrint(
    unsigned int const security_id,
    HFSAT::TradePrintInfo const &trade_print_info,
    HFSAT::MarketUpdateInfo const &market_update_info) {

  MarketLock();
  // Update market
  if (trade_print_info.trade_price_ <= 0) {
    dbglogger_ << "Received a price of : " << trade_print_info.trade_price_
               << " for shortcode : "
               << sec_name_indexer_.GetShortcodeFromId(security_id)
               << " , not processing..";
    MarketUnLock();
    return;
  }
  OnMarketUpdate(security_id, market_update_info);
  string shortcode_ = sec_name_indexer_.GetShortcodeFromId(security_id);
  double price_ = trade_print_info.trade_price_;

  last_traded_price_map_[shortcode_] = price_;

  typedef map<string, ScheduleStatus>::iterator itX_;
  for (itX_ iterator = raw_orders_.begin(); iterator != raw_orders_.end();
       iterator++) {
    string shortcode_string = iterator->first;
    if (!raw_orders_[shortcode_string].has_object)
      continue;
    vector<string> shortcodes_ =
        GetShortcodesFromProductRepresentation(shortcode_string);
    raw_orders_[shortcode_string].last_traded_price = 0;
    for (auto i : shortcodes_) {
      raw_orders_[shortcode_string].last_traded_price +=
          last_traded_price_map_[i];
    }
  }
  int _hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTime(watch_.tv().tv_sec);
  // Get minutes from midnight
  _hhmm_ = ((_hhmm_ / 100) * 60) + (_hhmm_ % 100) + 330;

  // Don't check for stoploss after 1525 IST
  // Also call unlock
  if (_hhmm_ >= (15 * 60 + 25)) {
    MarketUnLock();
    return;
  }

  typedef map<key_t, CurrentPortfolioStatus>::iterator it_;
  for (it_ iterator = portfolio_.begin(); iterator != portfolio_.end();
       iterator++) {
    int sec_id = sec_name_indexer_.GetIdFromString(iterator->first.second);
    CurrentPortfolioStatus curr_portfolio_ = iterator->second;

    // can put in one if condition also but keeping separate
    if ((unsigned(sec_id) == security_id && curr_portfolio_.position > 0 &&
         curr_portfolio_.has_stoploss &&
         price_ < curr_portfolio_.stoploss_price) ||
        (unsigned(sec_id) == security_id && curr_portfolio_.position < 0 &&
         curr_portfolio_.has_stoploss &&
         price_ > curr_portfolio_.stoploss_price)) {
      dbglogger_ << "Stoploss hit for: " << iterator->first.first
                 << " at the stoploss of: " << curr_portfolio_.stoploss_price
                 << ". Position was: " << curr_portfolio_.position
                 << ". Time was: " << watch_.tv().tv_sec << DBGLOG_ENDL_FLUSH;

      // Trade file contains only trades and can be used to calculate PNL
      // Lets not complicate further by looking at last_traded_price etc. This
      // is just for logging puposes and should be
      // good enough
      if (raw_orders_[shortcode_].is_executing &&
          (curr_portfolio_.position != curr_portfolio_.init_position)) {
        int timestamp_ = watch_.tv().tv_sec;
        int exec_time = timestamp_ - curr_portfolio_.sent_time;
        string shortcode_ = sec_name_indexer_.GetShortcodeFromId(sec_id);
        LogTradeFile(timestamp_, iterator->first.first, shortcode_,
                     curr_portfolio_.init_position, curr_portfolio_.position,
                     curr_portfolio_.init_price, price_, exec_time, 0,
                     "MARKET");
        portfolio_[iterator->first].init_position = curr_portfolio_.position;
      }

      raw_orders_[shortcode_].lots -= curr_portfolio_.position;
      SendOrderRequest();
      portfolio_[iterator->first].stoploss_executing = true;
      portfolio_[iterator->first].position = 0;
      portfolio_[iterator->first].sent_time = watch_.tv().tv_sec;
      portfolio_[iterator->first].updated = false;
      // Update the price when the order is going into the execution queue
      portfolio_[iterator->first].init_price =
          raw_orders_[shortcode_].last_traded_price;
      portfolio_[iterator->first].type = "TRIGGER";
    }
  }
  MarketUnLock();
}

void BaseOrderManager::PrintPortfolio() {
  typedef map<key_t, CurrentPortfolioStatus>::iterator it_;
  for (it_ iterator = portfolio_.begin(); iterator != portfolio_.end();
       iterator++) {
    string shortcode_string = iterator->first.second;
    CurrentPortfolioStatus curr_portfolio_ = iterator->second;
    dbglogger_ << iterator->first.first << "\n";
    dbglogger_ << shortcode_string << "\n";
    dbglogger_ << curr_portfolio_.ToString() << "\n";
  }
}

void BaseOrderManager::PrintOrders() {
  for (const auto &any : raw_orders_)
    if (any.second.lots != 0)
      dbglogger_ << any.first << "\t" << raw_orders_[any.first].lots << "\n";
}

string BaseOrderManager::GetATMCall(string &ticker) {
  return ("NSE_" + ticker + "_C0" + "_A");
}
string BaseOrderManager::GetATMPut(string ticker) {
  return ("NSE_" + ticker + "_P0" + "_A");
}

int BaseOrderManager::GetStepSize(string product) {
  if (product == "NIFTY") {
    return 50;
  } else if (product == "BANKNIFTY") {
    return 100;
  } else if (product == "FINNIFTY") {
    return 100;
  } else if (product == "MIDCPNIFTY") {
    return 100;
  }



  return -1;
}

int BaseOrderManager::GetStepRatio(string product) {
  if (product == "NIFTY") {
    return 2;
  } else if (product == "BANKNIFTY") {
    return 5;
  } else if (product == "FINNIFTY") {
    return 1;
  } else if (product == "MIDCPNIFTY") {
    return 1;
  }
  return -1;
}

// Gets the shortcodes from the strategy representation of a product
vector<string> BaseOrderManager::GetShortcodesFromProductRepresentation(
    string internal_product_code) {
  vector<string> shortcodes_;
  if (HFSAT::NSESecurityDefinitions::IsShortcode(internal_product_code)) {
    shortcodes_.push_back(internal_product_code);
  } else {
    // NSE_NIFTY_STRADDLE<0/1/2>_STRIKE
    vector<string> tokens;
    HFSAT::PerishableStringTokenizer::StringSplit(internal_product_code, '_',
                                                  tokens);
    string ticker = tokens[1];
    int strike_ = atoi(tokens[3].c_str());
    string short_ = GetATMCall(ticker);
    // We will always get the correct strike since ATM call is used in HFT
    // framework
    int atm_strike_ =
        HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(short_);
    string call_type_;
    string put_type_;

    int step_ratio_ = GetStepRatio(ticker);
    int step_size_ = GetStepSize(ticker);

    double depth_ =
        double(strike_ - atm_strike_) / double(step_ratio_ * step_size_);

    if (abs(depth_) == 0) {

      call_type_ = "A";
      put_type_ = "A";

    } else {

      call_type_ = (depth_ > 0) ? "O" : "I";
      put_type_ = (depth_ > 0) ? "I" : "O";

      call_type_ += to_string(int(floor(abs(depth_))));
      put_type_ += to_string(int(floor(abs(depth_))));

      if (abs(depth_) != floor(abs(depth_))) {
        double step_fraction_ = abs(depth_) - floor(abs(depth_));
        step_fraction_ *= step_ratio_;
        call_type_ += "_M" + to_string(int(round(step_fraction_)));
        put_type_ += "_M" + to_string(int(round(step_fraction_)));
      }
    }

    if (tokens[2].find("STRADDLE") != string::npos) {
      string shortcode_1_ = "NSE_" + ticker + "_C0_" + call_type_;
      string shortcode_2_ = "NSE_" + ticker + "_P0_" + put_type_;
      shortcodes_.push_back(shortcode_1_);
      shortcodes_.push_back(shortcode_2_);
    }
    // No handling for other type of synthetic products yet...
  }
  return shortcodes_;
}

vector<int> BaseOrderManager::GetSecurityIDsFromProductRepresentation(
    string internal_product_code) {
  vector<int> sec_ids_;
  vector<string> shortcodes_ =
      GetShortcodesFromProductRepresentation(internal_product_code);
  for (auto shortcode_ : shortcodes_) {
    sec_ids_.push_back(sec_name_indexer_.GetIdFromString(shortcode_));
  }
  return sec_ids_;
}
}
