#pragma once
#include "midterm/GeneralizedLogic/nse_synthetic_leg_exec_logic.hpp"
#include "nse_execution_listener.hpp"
#include <numeric>
#include <stddef.h>
#define INVALID_PRICE 0
#define INVALID_ORDERID -1
#define EPSILON 0.0001

namespace NSE_SIMPLEEXEC {

struct LegInfo {
  int order_id_;
  int seek_position_; // position that we are seeking, can be +1 or -1. This
                      // order will be in the mkt
  int live_position_; // position that has already been acquired by this
                      // security
  int req_position_;  // position that we want to acquire in the security
  double curr_px_;    // last traded px
  bool
      can_place_; // order can be sent to exec logic only if this is set to true
  bool has_live_orders_;  // true if live orders present in the mkt
  bool pos_check_failed_; // true if position constraint not letting us place
                          // order on this
  bool px_check_failed_;  // true if price constraint not letting us place order
                          // on this

  LegInfo() {
    order_id_ = INVALID_ORDERID;
    seek_position_ = 0;
    live_position_ = 0;
    req_position_ = 0;
    curr_px_ = INVALID_PRICE;
    can_place_ = false;
    has_live_orders_ = false;
    pos_check_failed_ = false;
    px_check_failed_ = false;
  }

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "ORDER_ID:			" << order_id_ << std::endl;
    t_temp_oss << "SEEK POSITION:		" << seek_position_
               << std::endl;
    t_temp_oss << "LIVE POSITION:		" << live_position_
               << std::endl;
    t_temp_oss << "REQD POSITION:		" << req_position_ << std::endl;
    t_temp_oss << "CURR PX:				" << curr_px_
               << std::endl;
    return t_temp_oss.str();
  }
};

// This class will be created for each complex order, and will be destroyed once
// order is executed
// TODO::We might leave this class in stale mode too, in that case we should
// free up unused memory
class NseGeneralizedExecutionLogic : public HFSAT::TimePeriodListener,
                                     public NseExecutionListener {
public:
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  std::string order_id_;
  std::string
      complex_instrument_; // Contains the complete string representation
  int num_sec_; // Contains the number of shortcodes we have to look at
  std::vector<std::string>
      shortcodes_; // Contains all the shortcodes we might be trading
  std::vector<NseSyntheticLegExecLogic *> legs_; // Contains the respective
                                                 // order handler classes - exec
                                                 // logic classes
  std::vector<std::string> px_conditions_;       // px conditions
  std::vector<std::string> pos_conditions_;      // pos conditions
  int max_time_to_execute_;
  int entry_time_;
  std::map<std::string, LegInfo> complex_info_;
  bool can_exit_; // true when order executed
  std::map<std::string, std::vector<double>>
      execution_prices_; // Contains each execution price per shortcode
  //
  std::map<int, std::string> tranche_to_strat_type_;
  std::string bkp_trades_file_;
  std::vector<std::string> IGNORE_STRATS_FOR_CARRYOVER;
public:
  NseGeneralizedExecutionLogic(HFSAT::DebugLogger &dbglogger_t,
                               HFSAT::Watch &watch_t, std::string order_id_t,
                               std::string inst_complex_t,
                               std::vector<NseSyntheticLegExecLogic *> legs_t,
                               ParamSet *t_param_,
			       std::map<int, std::string>& tranche_to_strat_type_t, std::string bkp_trades_file_t)
      : dbglogger_(dbglogger_t), watch_(watch_t),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        order_id_(order_id_t), complex_instrument_(inst_complex_t),
        legs_(legs_t), max_time_to_execute_(100000),
        entry_time_(watch_.tv().tv_sec), can_exit_(false),
        tranche_to_strat_type_(tranche_to_strat_type_t),
	bkp_trades_file_(bkp_trades_file_t) {
    // PARSING LOGIC FOR ORDER
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(complex_instrument_, '%',
                                                  tokens_);
    // Number of securities
    num_sec_ = std::atoi(tokens_[1].c_str());
    // List of securities
    HFSAT::PerishableStringTokenizer::StringSplit(tokens_[2], '|', shortcodes_);
    // tokens_[ 2 ] contains positions
    std::vector<std::string> positions_;
    HFSAT::PerishableStringTokenizer::StringSplit(tokens_[3], '|', positions_);
    for (int i = 0; i < num_sec_; i++) {
      LegInfo temp_leg_ = LegInfo();
      temp_leg_.req_position_ = std::atoi(positions_[i].c_str());
      complex_info_.insert(std::make_pair(shortcodes_[i], temp_leg_));
      execution_prices_.insert(
          std::make_pair(shortcodes_[i], std::vector<double>()));
    }
    // List of position conditions
    if (tokens_.size() >= 5) {
      HFSAT::PerishableStringTokenizer::StringSplit(tokens_[4], '|',
                                                    pos_conditions_);
    }
    // List of price conditions
    if (tokens_.size() >= 6) {
      HFSAT::PerishableStringTokenizer::StringSplit(tokens_[5], '|',
                                                    px_conditions_);
    }
    // Time check
    if (tokens_.size() >= 7) {
      max_time_to_execute_ = std::atof(tokens_[6].c_str());
      dbglogger_ << watch_.tv() << " Max time to execute for order id "
		 << order_id_ << " is " << max_time_to_execute_ << '\n';
    }
    // Sanity conditions
    if (shortcodes_.size() != (std::vector<std::string>::size_type)num_sec_) {
      dbglogger_ << "ERROR -> Number of securities mismatch...\n";
      exit(1);
    }

    for (auto leg : legs_) {
      // leg->SetAggressiveParam();
      leg->SetPassAggParam();
      leg->SetAggSpreadFactor(0.6);
    }

    //populate strat types whose orders should not be carried over
    IGNORE_STRATS_FOR_CARRYOVER.clear();
    IGNORE_STRATS_FOR_CARRYOVER.push_back("Pullback");
    IGNORE_STRATS_FOR_CARRYOVER.push_back("MeanReversion");
    IGNORE_STRATS_FOR_CARRYOVER.push_back("NewWeeklySG");  // SG is added to this list since it cancels all its unexecuted orders at 9:17 
    IGNORE_STRATS_FOR_CARRYOVER.push_back("NewMonthlySG"); // We will put in a more nuanced check for SG later to exclude certain orders ( eg future hedges )
    IGNORE_STRATS_FOR_CARRYOVER.push_back("NRB");
  }

  // TODO::Decide how to destruct, or stale
  ~NseGeneralizedExecutionLogic() {  }

  void ModifyConstraints(std::string general_shc_) {
    if (can_exit_) {
      std::cerr << "ALERT -> Cannot modify constraints since all positions "
                   "have been executed"
                << std::endl;
      dbglogger_ << "ALERT -> Cannot modify constraints since all positions "
                    "have been executed"
                 << '\n';
    } else {
      std::cout << "Modify Order Received for " << complex_instrument_
                << std::endl;
      dbglogger_ << "Modify Order Received for " << complex_instrument_ << '\n';
    }

    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(general_shc_, '%', tokens_);

    // Number of securities
    int temp_num_sec_ = std::atoi(tokens_[1].c_str());
    if (temp_num_sec_ != num_sec_) {
      std::cerr << "ALERT/ERROR -> Cannot modify since number of securities "
                   "don't match"
                << std::endl;
      return;
    }

    // List of securities
    std::vector<std::string> temp_shortcodes_;
    HFSAT::PerishableStringTokenizer::StringSplit(tokens_[2], '|',
                                                  temp_shortcodes_);
    // Sanity conditions
    if (temp_shortcodes_.size() != (std::vector<std::string>::size_type)temp_num_sec_) {
      std::cerr
          << "ALERT/ERROR -> Cannot modify since number of securities mismatch"
          << std::endl;
      return;
    }
    for (int i = 0; i < num_sec_; i++) {
      if (temp_shortcodes_[i] != shortcodes_[i]) {
        std::cerr
            << "ALERT/ERROR -> Cannot modify since shortcodes not in same order"
            << std::endl;
        return;
      }
    }

    // WE CANNOT MODIFY POSITION

    // Modify Position constraints
    pos_conditions_.clear();
    HFSAT::PerishableStringTokenizer::StringSplit(tokens_[4], '|',
                                                  pos_conditions_);
    // Modify Price conditions
    if (tokens_.size() == 6) {
      px_conditions_.clear();
      HFSAT::PerishableStringTokenizer::StringSplit(tokens_[5], '|',
                                                    px_conditions_);
    }
  }

  // We support <=, <. ==, >, >=
  // Also support mod condition on LHS
  // Can add more paradigms here later on
  bool CheckCondition(double lhs_, Comparator_t comparator_, double rhs_) {
    bool result_ = false;
    switch (comparator_) {
    case (Comparator_t::kLesserEquals): {
      if (lhs_ <= rhs_ + EPSILON) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kLesser): {
      if (lhs_ < rhs_) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kEquals): {
      if (std::abs(lhs_ - rhs_) <= EPSILON) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kGreater): {
      if (lhs_ > rhs_) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kGreaterEquals): {
      if (lhs_ >= rhs_ - EPSILON) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kAbsLesserEquals): {
      if (std::abs(lhs_) <= rhs_ + EPSILON) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kAbsLesser): {
      if (std::abs(lhs_) < rhs_) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kAbsGreater): {
      if (std::abs(lhs_) > rhs_) {
        result_ = true;
      }
    } break;
    case (Comparator_t::kAbsGreaterEquals):
    default: {
      if (std::abs(lhs_) >= rhs_ - EPSILON) {
        result_ = true;
      }
    } break;
    }
    return result_;
  }

  Comparator_t GetComparatorFromString(std::string comparator_) {
    Comparator_t comp_;

    if (strcmp(comparator_.c_str(), "geq") == 0)
      comp_ = Comparator_t::kGreaterEquals;
    else if (strcmp(comparator_.c_str(), "g") == 0)
      comp_ = Comparator_t::kGreater;
    else if (strcmp(comparator_.c_str(), "eq") == 0)
      comp_ = Comparator_t::kEquals;
    else if (strcmp(comparator_.c_str(), "leq") == 0)
      comp_ = Comparator_t::kLesserEquals;
    else if (strcmp(comparator_.c_str(), "l") == 0)
      comp_ = Comparator_t::kLesser;
    else if (strcmp(comparator_.c_str(), "abs_geq") == 0)
      comp_ = Comparator_t::kAbsGreaterEquals;
    else if (strcmp(comparator_.c_str(), "abs_g") == 0)
      comp_ = Comparator_t::kAbsGreater;
    else if (strcmp(comparator_.c_str(), "abs_leq") == 0)
      comp_ = Comparator_t::kAbsLesserEquals;
    else if (strcmp(comparator_.c_str(), "abs_l") == 0)
      comp_ = Comparator_t::kAbsLesser;
    else {
      dbglogger_ << "ERROR -> UNRECOGNIZED COMPARATOR" << DBGLOG_ENDL_FLUSH;
      comp_ = Comparator_t::kError;
    }

    return comp_;
  }

  // Basically we try to seek required position for each shortcode and if
  // seeking makes
  // position check fail, we set pos_check_failed_ true for that shortcode
  void CheckPositionConstraint() {
    // Check position conditions
    for (auto constraint_ : pos_conditions_) {
      std::vector<std::string> tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(constraint_, '?', tokens_);

      // Sanity check - comparator and threshold are extra
      if (tokens_.size() != (std::vector<std::string>::size_type)(num_sec_ + 2)) {
        dbglogger_ << "ERROR -> Position constraint cannot be handled...\n";
        continue;
      }

      double rhs_ = std::atof(tokens_[tokens_.size() - 1].c_str());

      // For LHS, we need to piece-wise multiply the live positions of each
      // shortcode with the coefficient
      double lhs_ = 0;
      for (int i = 0; i < num_sec_; i++) {
        double coefficient_ = std::atof(tokens_[i].c_str());
        int curr_;
        curr_ = complex_info_[shortcodes_[i]].live_position_;
        lhs_ += coefficient_ * curr_;
      }

      // Comparator can be eq, leq, geq, l, g,
      std::string comparator_ = tokens_[tokens_.size() - 2];
      Comparator_t comp_ = GetComparatorFromString(comparator_);
      if (comp_ == Comparator_t::kError)
        continue;

      for (int i = 0; i < num_sec_; i++) {
        std::string shortcode_ = shortcodes_[i];
        // continue when the security has already achieved the required position
        if (complex_info_[shortcode_].req_position_ ==
            complex_info_[shortcode_].live_position_)
          continue;

        int seek_pos_ = ((complex_info_[shortcode_].req_position_ -
                          complex_info_[shortcode_].live_position_) > 0)
                            ? 1
                            : -1;
        double temp_lhs_ = lhs_;
        temp_lhs_ += std::atof(tokens_[i].c_str()) * seek_pos_;

        if (!CheckCondition(temp_lhs_, comp_, rhs_)) {
          complex_info_[shortcode_].pos_check_failed_ = true;
          dbglogger_ << "Position Check Failed for order id : "<<order_id_<<" At Time " << watch_.tv().tv_sec
                     << "." << watch_.tv().tv_usec << " For " << shortcode_
                     << ", Condition Was -> " << constraint_ << '\n';
        }
      }
    }

    for (auto shc_ : shortcodes_) {
      if (!complex_info_[shc_].pos_check_failed_ &&
          complex_info_[shc_].req_position_ !=
              complex_info_[shc_].live_position_ &&
          complex_info_[shc_].seek_position_ == 0) {
        dbglogger_ << "Position Check Successful for order id : "<<order_id_<<" At Time " << watch_.tv().tv_sec
                   << "." << watch_.tv().tv_usec << " For " << shc_ << '\n';
      }
    }
  }

  void CheckPriceConstraint() {
    // Check price constraints
    for (auto constraint_ : px_conditions_) {
      std::vector<std::string> tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(constraint_, '?', tokens_);

      // Sanity check - comparator and threshold are extra
      if (tokens_.size() != (std::vector<std::string>::size_type)(num_sec_ + 2)) {
        dbglogger_ << "ERROR -> Position constraint cannot be handled...\n";
        continue;
      }

      double rhs_ = std::atof(tokens_[tokens_.size() - 1].c_str());

      // For LHS, we need to piece-wise multiply the last traded price of each
      // shortcode with the coefficient
      double lhs_ = 0;
      bool obtained_VWAP_for_all = true;
      for (int i = 0; i < num_sec_; i++) {
        double coefficient_ = std::atof(tokens_[i].c_str());
        // Check the average price if we execute the rest of the orders at the
        // curr_ price
        double eff_px_ = CalculateVWAP(shortcodes_[i]);
        // If price is 0 i.e no VWAP obtained, set the 
        if(eff_px_ <= 0 ){
            dbglogger_ << "No VWAP obtained yet for : " << shortcodes_[i];
            obtained_VWAP_for_all = false;
        }
        lhs_ += coefficient_ * eff_px_;
      }
      
      // Comparator can be eq, leq, geq, l, g,
      std::string comparator_ = tokens_[tokens_.size() - 2];
      Comparator_t comp_ = GetComparatorFromString(comparator_);
      if (comp_ == Comparator_t::kError)
        continue;

      if (!obtained_VWAP_for_all || !CheckCondition(lhs_, comp_, rhs_)) {
        for (int i = 0; i < num_sec_; i++) {
          if (std::atof(tokens_[i].c_str()) != 0) {
            complex_info_[shortcodes_[i]].px_check_failed_ = true;
            dbglogger_ << "Price Check Failed for order id : "<<order_id_<<" At Time " << watch_.tv().tv_sec
                       << "." << watch_.tv().tv_usec << " For "
                       << shortcodes_[i] << ", Condition Was -> " << constraint_
                       << ", LHS == " << lhs_ << ", RHS == " << rhs_ << '\n';
          }
        }
      }
    }
    for (auto shc_ : shortcodes_) {
      if (!complex_info_[shc_].px_check_failed_ &&
          complex_info_[shc_].req_position_ !=
              complex_info_[shc_].live_position_) {
        dbglogger_ << "Price Check Successful At Time " << "for order id : "<<order_id_<<" At Time "<<watch_.tv().tv_sec
                   << "." << watch_.tv().tv_usec << " For " << shc_ << '\n';
      }
    }
  }

  // function which sets can_exit_ to true when all orders have been executed
  bool CanExit() {
    for (auto shc_ : shortcodes_) {
      if (complex_info_[shc_].live_position_ !=
              complex_info_[shc_].req_position_ ||
          complex_info_[shc_].has_live_orders_)
        return false;
    }
    SetExit();
    return true;
  }

  // Function sets can_exit_ after cancelling all live orders
  void SetExit(std::string cancel_order_id_ = "") {
    // In case there is a cancel order id -> Cancel all live orders if they
    // exist
    if (cancel_order_id_ != "") {
      for (int i = 0; i < num_sec_; i++) {
        if (legs_[i]->HasOrderID(order_id_)) {
          // Send an order of opposite size
          legs_[i]->RequestCancel();
          legs_[i]->strat_orders_to_be_removed_.push_back(order_id_);
          legs_[i]->CheckAndNetOrder();
        }
      }
    }
    can_exit_ = true;
    return;
  }

  bool MarketClosing() {

    if (can_exit_)
      return true;

    int _hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTime(watch_.tv().tv_sec);
    // Get minutes from midnight
    _hhmm_ = ((_hhmm_ / 100) * 60) + (_hhmm_ % 100) + 330;
    // Dump positions
    if (_hhmm_ >= (15 * 60 + 29)) {
      dbglogger_ << "ALERT -> Market closing, hence dumping orders" << '\n';
      // Cancel all live orders
      for (int i = 0; i < num_sec_; i++) {
        if (complex_info_[shortcodes_[i]].seek_position_ != 0) {
          legs_[i]->CancelAllOrders();
          // We are not seeking anymore
          complex_info_[shortcodes_[i]].seek_position_ = 0;
        }
      }

      // Dump the unexecuted orders to a file
      // Generate the string
      std::ostringstream str_;
      str_ << "COMPLEX%" << num_sec_ << "%";
      // Add shortcodes to the string
      str_ << shortcodes_[0];
      if (num_sec_ > 1) {
        for (int i = 1; i < num_sec_; i++) {
          str_ << "|" << shortcodes_[i];
        }
      }
      str_ << "%";
      // Add positions
      str_ << complex_info_[shortcodes_[0]].req_position_ -
                  complex_info_[shortcodes_[0]].live_position_;
      if (num_sec_ > 1) {
        for (int i = 1; i < num_sec_; i++) {
          str_ << "|"
               << complex_info_[shortcodes_[i]].req_position_ -
                      complex_info_[shortcodes_[i]].live_position_;
        }
      }
      str_ << "%";
      // Add pos constraints
      if (pos_conditions_.size() > 0) {
        str_ << pos_conditions_[0];
        if (pos_conditions_.size() > 1) {
          for (std::vector<std::string>::size_type i = 1; i < pos_conditions_.size(); i++) {
            str_ << "|" << pos_conditions_[i];
          }
        }
      }
      str_ << "%";
      // Add px constraints
      if (px_conditions_.size() > 0) {
        str_ << px_conditions_[0];
        if (px_conditions_.size() > 1) {
          for (std::vector<std::string>::size_type i = 1; i < px_conditions_.size(); i++) {
            str_ << "|" << px_conditions_[i];
          }
        }
      }
      //this is NULL in case of tcp version
      if ( bkp_trades_file_ == "" ) {
	DBGLOG_CLASS_FUNC_LINE_FATAL
	   << "Not dumping processed orders as we are running tcp version"
	   << DBGLOG_ENDL_FLUSH;
	DBGLOG_DUMP;
        SetExit();
        return true;
      }
      std::ofstream bkp_trades_ostream;
      bkp_trades_ostream.open(bkp_trades_file_.c_str(), std::ofstream::app);
      if (!bkp_trades_ostream.is_open()) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "UNABLE TO OPEN THE TRADES FILE TO DUMP EXECUTIONS.."
            << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
      
      int pos = order_id_.find("_");
      //carry over orders of selected sets of strategies
      int tranche_id_ = atoi(order_id_.substr(pos+1,2).c_str());
      if ((tranche_to_strat_type_.find(tranche_id_) != tranche_to_strat_type_.end()) &&
	  (std::find(IGNORE_STRATS_FOR_CARRYOVER.begin(), IGNORE_STRATS_FOR_CARRYOVER.end(),
		     tranche_to_strat_type_[tranche_id_]) == IGNORE_STRATS_FOR_CARRYOVER.end())) {
        //use single string to flush to file in order to minimize risk of interleaved strings 
        //across different instances writing to same file in live
        std::ostringstream t_temp_oss_2;
	t_temp_oss_2 << order_id_.substr(0,pos) << '\t'
	             << str_.str() << "\t0\t"<< tranche_id_ <<"\t"
		     << "-1"
		     << "\tEntry" << std::endl;
	bkp_trades_ostream << t_temp_oss_2.str().c_str();
	bkp_trades_ostream.flush();
	bkp_trades_ostream.close();
      }
      
      SetExit();
      return true;
    }
    return false;
  }

  // For a shortcode, look at executeed and unexecuted positions and see what
  // price we would execute at effectively if rest of the orders done at current
  // px
  double CalculateVWAP(std::string shc) {
    std::vector<double> &old_prices_ = execution_prices_[shc];
    double curr_px_ = complex_info_[shc].curr_px_;
    int position_to_execute_ =
        complex_info_[shc].req_position_ - complex_info_[shc].live_position_;
    double eff_px_ = 0;
    if (complex_info_[shc].req_position_ != 0) {
      eff_px_ = (std::accumulate(old_prices_.begin(), old_prices_.end(), 0.0) +
                 std::abs(position_to_execute_) * curr_px_) /
                (std::abs(position_to_execute_) + old_prices_.size());
    }
    return eff_px_;
  }

  // Populate map of execution prices whenever we receive any execution
  void OnExec(std::string _order_id_, int _traded_qty_, double _price_) {
    if (_order_id_.find(order_id_) != std::string::npos) {
      int temp_count_ = 0;
      unsigned int i = 0;
      for ( ; i < _order_id_.length(); i++) {
        if (_order_id_[i] == '_') {
          temp_count_++;
          if (temp_count_ == 2)
            break;
        }
      }

      std::string shc_ = _order_id_.substr(i + 1);
      execution_prices_[shc_].push_back(_price_);
    }
  }

  void OnTimePeriodUpdate(const int num_pages_to_add_) {

    if (can_exit_)
      return;

    // Set can_place_ to false for all
    for (int i = 0; i < num_sec_; i++) {
      complex_info_[shortcodes_[i]].can_place_ = false;
      complex_info_[shortcodes_[i]].pos_check_failed_ = false;
      complex_info_[shortcodes_[i]].px_check_failed_ = false;
      // Update prices
      complex_info_[shortcodes_[i]].curr_px_ = legs_[i]->last_traded_px_;
      // Check if live orders present
      complex_info_[shortcodes_[i]].has_live_orders_ =
          legs_[i]->HasOrderID(order_id_);
      // Case when we were seeking position, which might have been executed by
      // now.. Transfer this to live position
      if (complex_info_[shortcodes_[i]].seek_position_ != 0 &&
          !complex_info_[shortcodes_[i]].has_live_orders_) {
        complex_info_[shortcodes_[i]].live_position_ +=
            complex_info_[shortcodes_[i]].seek_position_;
        complex_info_[shortcodes_[i]].seek_position_ = 0;
      }
    }

    int _hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTime(watch_.tv().tv_sec);
    // Get minutes from midnight
    _hhmm_ = ((_hhmm_ / 100) * 60) + (_hhmm_ % 100) + 330;
    // Dump positions
    // Aggro from 1528 to 1529
    if (_hhmm_ >= (15 * 60 + 28)) {
      for (auto leg : legs_) {
        leg->SetAggressiveParam();
        leg->SetAggSpreadFactor(1);
      }
    }

    /*
    // Code for Passive on both legs, aggress when one gets hit
    if ( std::abs( complex_info_[ shortcodes_[ 0 ] ].live_position_ ) >
    std::abs( complex_info_[ shortcodes_[ 1 ]
    ].live_position_ ) ) {
      legs_[ 1 ]->SetAggressiveParam();
      legs_[ 0 ]->SetPassiveParam();
    }
    else if ( std::abs( complex_info_[ shortcodes_[ 0 ] ].live_position_ ) <
    std::abs( complex_info_[ shortcodes_[ 1 ]
    ].live_position_ ) ) {
      legs_[ 0 ]->SetAggressiveParam();
      legs_[ 1 ]->SetPassiveParam();
    }
    else {
      legs_[ 0 ]->SetPassiveParam();
      legs_[ 1 ]->SetPassiveParam();
    }
    */

    // Check if all positions have been executed, stale state
    // Delete from outside if required
    if (CanExit())
      return;

    if (!HFSAT::IsItSimulationServer() && MarketClosing()) {
      SetExit();
      return;
    }

    CheckPositionConstraint();
    CheckPriceConstraint();

    // Place just one order at a time, also make sure we don't place order if
    // order is already live for security
    for (auto shc_ : shortcodes_) {
      //change semantics - cancel order after max_time_to_execute_
      if (watch_.tv().tv_sec - entry_time_ > max_time_to_execute_) {
        complex_info_[shc_].can_place_ = false;
        SetExit(order_id_);
        dbglogger_ << "Triggering Cancel of order " << order_id_
                   << " since it has exceeded max time at " << watch_.tv() << '\n';
        break;
      }
      if (complex_info_[shc_].req_position_ !=
              complex_info_[shc_].live_position_ &&
          complex_info_[shc_].seek_position_ == 0) {
        if (!complex_info_[shc_].pos_check_failed_ &&
            !complex_info_[shc_].px_check_failed_) {
          complex_info_[shc_].can_place_ = true;
          dbglogger_ << "Trying to see if we can place order in mkt at time "
                     << watch_.tv().tv_sec << "." << watch_.tv().tv_usec
                     << " for " << shc_ << '\n';
          break;
        }
      }
    }

    // Place orders
    for (int i = 0; i < num_sec_; i++) {
      std::string shortcode_ = shortcodes_[i];
      std::string order_id_place_ = order_id_ + "_" + shortcode_;

      if (complex_info_[shortcode_].can_place_) {
        int seek_pos_ = ((complex_info_[shortcode_].req_position_ -
                          complex_info_[shortcode_].live_position_) > 0)
                            ? 1
                            : -1;
        // Another check, we don't want to place multiple orders at once
        if (!legs_[i]->HasOrderID(order_id_place_)) {
          dbglogger_ << "Placing order on " << shortcode_ << " for an order of "
                     << seek_pos_ << " lots, px is "
                     << complex_info_[shortcode_].curr_px_ << '\n';
          std::cout << "Placing order on " << shortcode_ << " for an order of "
                    << seek_pos_ << " lots, px is "
                    << complex_info_[shortcode_].curr_px_ << std::endl;
          // Add this live order to seek_position_, once added we transfer this
          // to live_position_
          bool order_placed_ =
              legs_[i]->OnNewOrderFromStrategy(order_id_place_, seek_pos_);
          if (order_placed_) {
            complex_info_[shortcode_].seek_position_ = seek_pos_;
          }
        }
      }
    }
  }
};
}
