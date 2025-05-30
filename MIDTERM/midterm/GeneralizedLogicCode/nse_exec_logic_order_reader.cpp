#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace NSE_SIMPLEEXEC {

SimpleNseExecLogicOrderReader::SimpleNseExecLogicOrderReader(
    HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t, bool is_live_t,
    std::map<std::string, SyntheticLegInfo> leg_info_t,
    NseExecLogicHelper *exec_helper_t, std::string bkp_trade_file_t)
    : instrument_to_exec_logic_map_(),
      stratID_to_last_processed_order_time_map_(), watch_(watch_t),
      dbglogger_(dbglogger_t), exec_start_time_(-1), isLive(is_live_t),
      leg_info_(leg_info_t), is_pre_monthly_expiry_week(false),
      is_monthly_expiry_week(false),
      exec_logic_helper_(exec_helper_t),
      bkp_file_(bkp_trade_file_t){ // Intentionally iniitiazed with max value
  struct timeval tv;
  gettimeofday(&tv, NULL);
  exec_start_time_ = GMTISTTIMEDIFFOFFSET + tv.tv_sec * 1000 * 1000 * 1000; // nanosec
  int nearest_monthly_expiry_ =
      HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
          "NSE_BANKNIFTY_C0_A");
  nearest_weekly_expiry_ =
      HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
          "NSE_BANKNIFTY_C0_A_W");
  next_weekly_expiry_ =
      HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
          "NSE_BANKNIFTY_C1_A_W");

  if (nearest_weekly_expiry_ > nearest_monthly_expiry_) {
    is_monthly_expiry_week = true;
  } else if (next_weekly_expiry_ > nearest_monthly_expiry_) {
    is_pre_monthly_expiry_week = true;
  }

  //Create the map from trancheid to strat type which is needed 
  //to filter orders at time of dumping them in backup file
  tranche_id_to_strategy_.clear();

  std::ifstream tranche_file_;
  tranche_file_.open(STRAT_TRANCHE_FILE, std::ifstream::in);
  if (tranche_file_.is_open()) {
    const int kOrderFileLineBufferLen = 1024;
    char readline_buffer_[kOrderFileLineBufferLen];
    bzero(readline_buffer_, kOrderFileLineBufferLen);

    while (tranche_file_.good()) {
      bzero(readline_buffer_, kOrderFileLineBufferLen);
      tranche_file_.getline(readline_buffer_, kOrderFileLineBufferLen);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_,
                                           kOrderFileLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      // line is like "43  SSGap    midterm-dispersion  FILE    /spare/local/files/NSE/ExecutionLogs/ordersfile 45008"
      if (tokens_.size() < 6) 
        continue;
      else if (tokens_[0][0] == '#')
        continue;
      else {
        // TODO - remove logging after tests
        dbglogger_ << "Added entry in tranch <-> strat map with values " << atoi(tokens_[0]) << ' ' << tokens_[1] << '\n'; 
        tranche_id_to_strategy_[atoi(tokens_[0])] = tokens_[1];
      }
    }
  }
}

void SimpleNseExecLogicOrderReader::SubscribeNewOrders(
    std::string t_instrument_, SimpleNseExecLogic *t_exec_logic_) {
  instrument_to_exec_logic_map_.insert(
      std::make_pair(t_instrument_, t_exec_logic_));
}

// If order is complex/general we create class on the fly
void SimpleNseExecLogicOrderReader::NotifyOrderListeners(
    std::string t_shortcode_, std::string order_id_, int order_size_,
    double ref_px_) {
  std::vector<std::string> tokens_;
  HFSAT::PerishableStringTokenizer::StringSplit(t_shortcode_, '%', tokens_);
  int num_sec_ = std::atoi(tokens_[1].c_str());
  std::vector<std::string> shortcodes_mft_;
  HFSAT::PerishableStringTokenizer::StringSplit(tokens_[2], '|',
                                                shortcodes_mft_);
  std::vector<std::string> shortcodes_;

  for (auto shc : shortcodes_mft_) {
    // Case when this is a future
    std::string this_shc_;
    std::cout<< "SHORTCODE: " << shc << " STD " << std::endl;
    if (HFSAT::NSESecurityDefinitions::IsShortcode(shc)) {
      this_shc_ = shc;
      std::cout<< "SHORTCODEEXIST: " << shc << " STD " << this_shc_ << std::endl;
    } else {
      std::vector<std::string> shc_tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(shc, '_', shc_tokens_);
      // For options tokens size must be greater than 3
      // we reach here if the fut shortcode is invalid ..
      if ( shc_tokens_.size() < 4 ) {
           dbglogger_
               << "Order Not Read by exec. Order Id: "<<order_id_
               << DBGLOG_ENDL_FLUSH;
           dbglogger_
               << "ERROR/ALERT -> Unhandled " << shc << " in NotifyOrderListeners, not processing this further...\n";
 	   return;

      }
      std::string ticker_ = shc_tokens_[0];
      char underlying_[50];
      strcpy(underlying_, ticker_.c_str());
      std::string type_ = shc_tokens_[1];
      double strike_ = std::atof(shc_tokens_[2].c_str());
      std::string expiry_ = shc_tokens_[3];
      int exp_num = atoi(expiry_.c_str());
      // Check whether input shortcode corresponds to _W
      bool is_weekly = (shc_tokens_.size() == 5) ? true : false;
      // If monthly option, skip this
      if (is_weekly) {
        // In pre-monthly expiry case : For WOPT_0 -> 0_W, WOPT_1 -> 0
        if (is_pre_monthly_expiry_week) {
          if (exp_num == 1) {
            is_weekly = false;
            exp_num = 0;
          }
        }
        // In monthly expiry case : For WOPT_0 -> 0, WOPT_1 -> 0_W
        else if (is_monthly_expiry_week) {
          if (exp_num == 0) {
            is_weekly = false;
          }
          if (exp_num == 1) {
            exp_num = 0;
          }
        }
      }
      int expiry_dt_ =
          HFSAT::NSESecurityDefinitions::GetExpiryFromContractNumber(
              exp_num, HFSAT::NSEInstrumentType_t::NSE_STKOPT, underlying_);
      if( is_weekly == true && (is_pre_monthly_expiry_week == false && exp_num==1)){ 
        expiry_dt_ = next_weekly_expiry_; is_weekly =false;
      }
      if( is_weekly == true && (exp_num == 0 && is_monthly_expiry_week == true)){
        expiry_dt_ = nearest_weekly_expiry_; is_weekly =false;
      }
      std::cout<<"EPX DT: " << expiry_dt_ << " " << exp_num << std::endl; 
      std::cout << "TICK " << ticker_ <<" EPXIR "<< expiry_dt_ << " STK "<<  strike_ << std::endl;
      if (type_ == "CE") {
        this_shc_ = HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(
            ticker_, expiry_dt_, strike_, true);
      } else if (type_ == "PE") {
        this_shc_ = HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(
            ticker_, expiry_dt_, strike_, false);
      } else {
    	  dbglogger_
    	              << "Order Not Read by exec. Order Id: "<<order_id_
    	              << DBGLOG_ENDL_FLUSH;
        dbglogger_
            << "ERROR/ALERT -> Unhandled " << shc << " in NotifyOrderListeners, not processing this further...\n";
        return;
      }
      if (is_weekly) {
        this_shc_ += "_W";
      }
      std::cout<<"SHORTCODE END: " << this_shc_ <<std::endl;
    }

    // Case when it is too OTM
    if (!HFSAT::NSESecurityDefinitions::IsShortcode(this_shc_)) {
    	dbglogger_
    	    	              << "Order Not Read by exec. Order Id: "<<order_id_
    	    	              << DBGLOG_ENDL_FLUSH;
      dbglogger_ << "ERROR/ALERT -> Shortcode " << this_shc_
                 << " cannot be handled hence ignoring" << DBGLOG_ENDL_FLUSH;
      return;
    }

    dbglogger_ << "Processed " << shc << " to " << this_shc_ << '\n';
    shortcodes_.push_back(this_shc_);
  }

  // Create classes
  std::vector<NSE_SIMPLEEXEC::NseSyntheticLegExecLogic *> legs_;
  for (int i = 0; i < num_sec_; i++) {
    if (leg_info_.find(shortcodes_[i]) == leg_info_.end()) {
    	dbglogger_
    	    	              << "Order Not Read by exec. Order Id: "<<order_id_
    	    	              << DBGLOG_ENDL_FLUSH;
      dbglogger_
          << "ERROR/ALERT -> Shortcode " << shortcodes_[i]
          << " cannot be handled hence ignoring, maybe it is a PARAM issue"
          << DBGLOG_ENDL_FLUSH;
      return;
    }

    SyntheticLegInfo info_ = leg_info_[shortcodes_[i]];
    // Fetch SMV
    HFSAT::SecurityMarketView *temp_smv_ = info_.smv_;
    NSE_SIMPLEEXEC::ParamSet *temp_param_ = info_.param_;
    HFSAT::NSELoggedMessageFileSource *temp_filesource_ = info_.filesource_;
    HFSAT::SmartOrderManager *smart_order_manager_ = NULL;
    NSE_SIMPLEEXEC::SimpleNseExecLogic *exec_logic_ = NULL;

    // Case when this is the first time we are trading this shortcode
    if (synthetic_execs_.find(shortcodes_[i]) == synthetic_execs_.end()) {
      exec_logic_ =
          exec_logic_helper_->Setup(smart_order_manager_, shortcodes_[i],
                                    temp_smv_, temp_param_, temp_filesource_);
      // Also add to synthetic_execs_
      synthetic_execs_[shortcodes_[i]] = exec_logic_;
    } else {
      exec_logic_ = synthetic_execs_[shortcodes_[i]];
    }

    legs_.push_back(
        dynamic_cast<NSE_SIMPLEEXEC::NseSyntheticLegExecLogic *>(exec_logic_));
    std::cout << "Created synthetic generalized leg for -> " << shortcodes_[i]
              << ", " << shortcodes_mft_[i] << std::endl;
  }

  NSE_SIMPLEEXEC::NseGeneralizedExecutionLogic *exec_ =
      new NSE_SIMPLEEXEC::NseGeneralizedExecutionLogic(
          dbglogger_, watch_, order_id_, t_shortcode_, legs_, NULL, tranche_id_to_strategy_, bkp_file_);
  NseExecutionListenerManager::GetUniqueInstance()
      .SubscribeNseExecutionListener(exec_);
  dbglogger_
            << "Order Read by exec. Order Id: "<<order_id_
            << DBGLOG_ENDL_FLUSH;
  watch_.subscribe_BigTimePeriod(exec_);
  live_generalized_execs_[order_id_] = exec_;
}

void SimpleNseExecLogicOrderReader::NotifyCancelListeners(
    std::string order_id_, std::string order_tag_) {
  if (live_generalized_execs_.find(order_tag_) !=
      live_generalized_execs_.end()) {
    live_generalized_execs_[order_tag_]->SetExit(order_id_);
  } else {
    dbglogger_
        << "ALERT/ERROR -> Cancel requested for an order that does not exist "
	<< "Order ID: " << order_id_ << " Order Tag" << order_tag_ << "\n";
    std::cerr
        << "ALERT/ERROR -> Cancel requested for an order that does not exist "
        << "Order ID: " << order_id_ << " Order Tag" << order_tag_ << std::endl;
    return;
 }
}

void SimpleNseExecLogicOrderReader::NotifyForceCancelListeners(
    std::string instrument_, std::string order_id_, std::string order_tag_){
  std::string t_instrument_ = GenerateShortCodeVA(instrument_);
    if (synthetic_execs_.find(t_instrument_) != synthetic_execs_.end()) {
    // exist
        NSE_SIMPLEEXEC::NseSyntheticLegExecLogic *nse_syhtehtic_leg = dynamic_cast<NSE_SIMPLEEXEC::NseSyntheticLegExecLogic*>(synthetic_execs_[t_instrument_]);

        if (nse_syhtehtic_leg != NULL) {

          if (nse_syhtehtic_leg->HasOrderID(order_tag_)) {
            // Send an order of opposite size
            dbglogger_ << "FORCE CANCEL ,Exist Sending Cancel for ordertag: " << order_tag_ << "\n";
            nse_syhtehtic_leg->RequestCancel();
            nse_syhtehtic_leg->strat_orders_to_be_removed_.push_back(order_tag_);
            dbglogger_ << "NETTING AND CHECKING ORDER " << "\n";
            nse_syhtehtic_leg->CheckAndNetOrder();
    // can_exit_ = true;
            // Faking cancel ORS Cancel reply
            dbglogger_ << "Sending Fake Ors cancel response: " << order_tag_ << "\n";
            nse_syhtehtic_leg->FakeOrsCancel();
            dbglogger_ << "Should be Cancled by now: " << order_tag_ << "\n";
          }
          else {
            dbglogger_ <<
              "ALERT/ERROR -> ForceCancelRequest ORDERTAG doesnot exist " << order_tag_ << "\n";
          }
        } else {
            dbglogger_ << "ALERT/ERROR -> NotifyForceCancelListeners Derived Pointer NuLL" << "\n";
            return;
        }
    } else {
      dbglogger_
        << "ALERT/ERROR -> ForceCancelRequest For ShortCode that doesn't exist "
        << "Instrument: " << t_instrument_ << " Order Tag" << order_tag_ << " ID: " << order_id_ << "\n";
      return;
  }
}

std::string SimpleNseExecLogicOrderReader::GenerateShortCodeVA(std::string shc){
  std::string this_shc_;
    if (HFSAT::NSESecurityDefinitions::IsShortcode(shc)) {
      this_shc_ = shc;
    } else {
      std::vector<std::string> shc_tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(shc, '_', shc_tokens_);
      // For options tokens size must be greater than 3
      // we reach here if the fut shortcode is invalid ..
      if ( shc_tokens_.size() < 4 ) {
           dbglogger_ << "ERROR/ALERT -> Unhandled SIZE " << shc << "...\n";
 	   return this_shc_;
      }
      std::string ticker_ = shc_tokens_[0];
      char underlying_[50];
      strcpy(underlying_, ticker_.c_str());
      std::string type_ = shc_tokens_[1];
      double strike_ = std::atof(shc_tokens_[2].c_str());
      std::string expiry_ = shc_tokens_[3];
      int exp_num = atoi(expiry_.c_str());
      // Check whether input shortcode corresponds to _W
      bool is_weekly = (shc_tokens_.size() == 5) ? true : false;
      // If monthly option, skip this
      if (is_weekly) {
        // In pre-monthly expiry case : For WOPT_0 -> 0_W, WOPT_1 -> 0
        if (is_pre_monthly_expiry_week) {
          if (exp_num == 1) {
            is_weekly = false;
            exp_num = 0;
          }
        }
        // In monthly expiry case : For WOPT_0 -> 0, WOPT_1 -> 0_W
        else if (is_monthly_expiry_week) {
          if (exp_num == 0) {
            is_weekly = false;
          }
          if (exp_num == 1) {
            exp_num = 0;
          }
        }
      }
      int expiry_dt_ =
          HFSAT::NSESecurityDefinitions::GetExpiryFromContractNumber(
              exp_num, HFSAT::NSEInstrumentType_t::NSE_STKOPT, underlying_);
      
      if (type_ == "CE") {
        this_shc_ = HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(
            ticker_, expiry_dt_, strike_, true);
      } else if (type_ == "PE") {
        this_shc_ = HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(
            ticker_, expiry_dt_, strike_, false);
      } else {
    	  dbglogger_
    	              << "ALERT/ERROR -> Option not valid " << shc << "...\n" << DBGLOG_ENDL_FLUSH;
        return this_shc_;
      }
      if (is_weekly) {
        this_shc_ += "_W";
      }
    }
    return this_shc_;
}

void SimpleNseExecLogicOrderReader::NotifyModifyListeners(
    std::string t_instrument_, std::string order_tag_) {
  if (live_generalized_execs_.find(order_tag_) !=
      live_generalized_execs_.end()) {
    live_generalized_execs_[order_tag_]->ModifyConstraints(t_instrument_);
  } else {
    dbglogger_
        << "ALERT/ERROR -> Modify requested for an order that does not exist "
        << "Instrument: " << t_instrument_ << " Order Tag" << order_tag_ << "\n";	
    std::cerr
        << "ALERT/ERROR -> Modify requested for an order that does not exist "
	<< "Instrument: " << t_instrument_ << " Order Tag" << order_tag_ << std::endl;
    return;
  }
}
}
