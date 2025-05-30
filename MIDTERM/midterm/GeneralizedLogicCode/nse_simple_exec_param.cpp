#include "midterm/GeneralizedLogic/nse_simple_exec_param.hpp"

namespace NSE_SIMPLEEXEC {
ParamSet::ParamSet(const std::string &_paramfilename_,
                   std::string &_shortcode_) // set default param values
    : avg_sprd_(1),
      agg_sprd_factor_(1),
      instrument_(_shortcode_),
      trade_cooloff_interval_(10000),    // 10 secs
      market_participation_factor_(0.1), // percentage of traded volume
      market_participation_factor_tolerance_(0.05),
      exec_algo_(kAggOnly),
      exec_logic_run_type_(kDecideOrdersFromParamConstraints),
      moneyness_(0),
      aggress_threshold_(-1),
      max_notional_(50000000),
	  //TODO:RmCap Change here
      max_lots_per_trade_(1),
      max_lots_(-1),
      buysell_(HFSAT::kTradeTypeBuy),
      start_time_msecs_(0),
      end_time_msecs_(0),
      yyyymmdd_(20151010),
      expiry_to_look_till(0) {
  ParseParamFile(_paramfilename_);
}

void ParamSet::ParseParamFile(const std::string &_paramfilename_) {

  std::vector<std::string> tokens_;
  HFSAT::PerishableStringTokenizer::StringSplit(_paramfilename_, '\t', tokens_);
  std::cout<<"Parsing File arg: " << _paramfilename_ << std::endl;
  std::cout<<"Size: " << tokens_.size() <<" token1: " << tokens_[1].c_str() <<" token2: " 
    << tokens_[2].c_str() <<" token3: " << tokens_[3].c_str() <<" token4: " << tokens_[4].c_str()
    <<" token5: " << tokens_[5].c_str() <<" token6: " << tokens_[6].c_str() <<" token1: "
    << tokens_[7].c_str() << std::endl;
  avg_sprd_ = atof(tokens_[1].c_str());
  agg_sprd_factor_ = atof(tokens_[2].c_str());
  trade_cooloff_interval_ = atoi(tokens_[3].c_str());
  market_participation_factor_ = atof(tokens_[4].c_str());
  market_participation_factor_tolerance_ = atof(tokens_[5].c_str());
  if (tokens_[6] == "AGGONLY") {
    exec_algo_ = kAggOnly;
  } else if (tokens_[6] == "PASSONLY") {
    exec_algo_ = kPassOnly;
  } else if (tokens_[6] == "PASSANDAGG") {
    exec_algo_ = kPassAndAgg;
  } else {
    std::cerr << "ALERT -> Wrong Exec Algo Provided" << std::endl;
  }

  if (tokens_[7] == "PARAMS") {
    exec_logic_run_type_ = kDecideOrdersFromParamConstraints;
  } else if (tokens_[7] == "STRATEGY") {
    exec_logic_run_type_ = kGetOrdersFromStrategy;
  } else {
    std::cerr << "ALERT -> Wrong Exec Run Type Provided" << std::endl;
  }

  if (tokens_.size() >= 9) {
    moneyness_ = atof(tokens_[8].c_str());
  }

  if (tokens_.size() >= 10) {
    aggress_threshold_ = atof(tokens_[9].c_str());
  }

  if (tokens_.size() >= 11) {
    expiry_to_look_till = atoi(tokens_[10].c_str());
  }
}

std::string ParamSet::ToString() {
  std::ostringstream t_retval_;
  t_retval_ << "AVG_SPREAD: " << avg_sprd_ << '\n';
  t_retval_ << "AGG_SPRD_FACTOR: " << agg_sprd_factor_ << '\n';
  t_retval_ << "INSTRUMENT: " << instrument_ << '\n';
  t_retval_ << "COOLOFF_INTERVAL: " << trade_cooloff_interval_ / 1000
            << " secs\n";
  t_retval_ << "PARTICIPATION: " << market_participation_factor_ * 100.0
            << " %\n";
  t_retval_ << "EXECALGO: " << (int)exec_algo_ << '\n';
  t_retval_ << "NOTIONAL: " << max_notional_ << '\n';
  t_retval_ << "START_MSECS: " << start_time_msecs_ << '\n';
  t_retval_ << "END_MSECS: " << end_time_msecs_ << '\n';
  t_retval_ << "DATE: " << yyyymmdd_ << '\n';
  return t_retval_.str();
}
}
