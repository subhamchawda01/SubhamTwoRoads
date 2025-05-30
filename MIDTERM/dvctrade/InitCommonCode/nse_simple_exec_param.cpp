#include "dvctrade/InitCommon/nse_simple_exec_param.hpp"

namespace NSE_SIMPLEEXEC {
ParamSet::ParamSet(const std::string& _paramfilename_)  // set default param values
    : avg_sprd_(1),
      agg_sprd_factor_(1),
      instrument_(""),
      trade_cooloff_interval_(10000),     // 10 secs
      market_participation_factor_(0.1),  // percentage of traded volume
      market_participation_factor_tolerance_(0.05),
      exec_algo_(kAggOnly),
      exec_logic_run_type_(kDecideOrdersFromParamConstraints),
      max_notional_(50000000),
      max_lots_per_trade_(1),
      max_lots_(-1),
      buysell_(HFSAT::kTradeTypeBuy),
      start_time_msecs_(0),
      end_time_msecs_(0),
      yyyymmdd_(20151010) {
  ParseParamFile(_paramfilename_);
}

void ParamSet::ParseParamFile(const std::string& _paramfilename_) {
  std::ifstream paramfile_;
  paramfile_.open(_paramfilename_.c_str(), std::ifstream::in);
  if (paramfile_.is_open()) {
    const int kParamFileLineBufferLen = 1024;
    char readline_buffer_[kParamFileLineBufferLen];
    bzero(readline_buffer_, kParamFileLineBufferLen);

    while (paramfile_.good()) {
      bzero(readline_buffer_, kParamFileLineBufferLen);
      paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);
      char* param_name_ = strtok(readline_buffer_, " \t\n");
      if (param_name_ == NULL) continue;
      char* param_value_ = strtok(NULL, " \t\n");
      if (strcmp(param_name_, "AVG_SPREAD") == 0) {
        avg_sprd_ = atof(param_value_);
      } else if (strcmp(param_name_, "AGG_SPREAD_FACTOR") == 0) {
        agg_sprd_factor_ = atof(param_value_);
      } else if (strcmp(param_name_, "INSTRUMENT") == 0) {
        instrument_ = param_value_;
      } else if (strcmp(param_name_, "COOLOFF_MSECS") == 0) {
        trade_cooloff_interval_ = atoi(param_value_);
      } else if (strcmp(param_name_, "PARTICIPATION_FACTOR") == 0) {
        market_participation_factor_ = atof(param_value_);
      } else if (strcmp(param_name_, "PARTICIPATION_FACTOR_TOLERANCE") == 0) {
        market_participation_factor_tolerance_ = atof(param_value_);
      } else if (strcmp(param_name_, "EXEC_ALGO") == 0) {
        if (strcmp(param_value_, "AGGONLY") == 0) {
          exec_algo_ = kAggOnly;
        } else if (strcmp(param_value_, "PASSANDAGG") == 0) {
          exec_algo_ = kPassAndAgg;
        } else if (strcmp(param_value_, "PASSONLY") == 0) {
          exec_algo_ = kPassOnly;
        } else {
          std::cerr << "Wrong Exec Algo provided \n";
        }
      } else if (strcmp(param_name_, "EXEC_RUN_TYPE") == 0) {
        if (strcmp(param_value_, "PARAMS") == 0) {
          exec_logic_run_type_ = kDecideOrdersFromParamConstraints;
        } else if (strcmp(param_value_, "STRATEGY") == 0) {
          exec_logic_run_type_ = kGetOrdersFromStrategy;
        }
      } else if (strcmp(param_name_, "NOTIONAL") == 0) {
        max_notional_ = atof(param_value_);
      } else if (strcmp(param_name_, "MAX_LOTS") == 0) {
        max_lots_ = atoi(param_value_);
      } else if (strcmp(param_name_, "LOTS_PER_TRADE") == 0) {
        max_lots_per_trade_ = atoi(param_value_);
      } else if (strcmp(param_name_, "BUYSELL") == 0) {
        if (strcmp(param_value_, "BUY") == 0) {
          buysell_ = HFSAT::kTradeTypeBuy;
        } else if (strcmp(param_value_, "SELL") == 0) {
          buysell_ = HFSAT::kTradeTypeSell;
        } else {
          std::cerr << "Wrong TradeType provided \n";
        }
      } else if (strcmp(param_name_, "START_TIME_MSECS") == 0) {
        start_time_msecs_ = atoi(param_value_);
      } else if (strcmp(param_name_, "END_TIME_MSECS") == 0) {
        end_time_msecs_ = atoi(param_value_);
      } else if (strcmp(param_name_, "DATE") == 0) {
        yyyymmdd_ = atoi(param_value_);
      }
    }
  }
}

std::string ParamSet::ToString() {
  std::ostringstream t_retval_;
  t_retval_ << "AVG_SPREAD: " << avg_sprd_ << '\n';
  t_retval_ << "AGG_SPRD_FACTOR: " << agg_sprd_factor_ << '\n';
  t_retval_ << "INSTRUMENT: " << instrument_ << '\n';
  t_retval_ << "COOLOFF_INTERVAL: " << trade_cooloff_interval_ / 1000 << " secs\n";
  t_retval_ << "PARTICIPATION: " << market_participation_factor_ * 100.0 << " %\n";
  t_retval_ << "EXECALGO: " << (int)exec_algo_ << '\n';
  t_retval_ << "NOTIONAL: " << max_notional_ << '\n';
  t_retval_ << "START_MSECS: " << start_time_msecs_ << '\n';
  t_retval_ << "END_MSECS: " << end_time_msecs_ << '\n';
  t_retval_ << "DATE: " << yyyymmdd_ << '\n';
  return t_retval_.str();
}
}
