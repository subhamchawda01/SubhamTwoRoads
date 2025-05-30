/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/InitCommon/port_exec_paramset.hpp"
#include <math.h> // fabs

namespace HFSAT {
  PortExecParamSet::PortExecParamSet(const std::string& _paramfilename_)
    : instrument_(""),
      trade_cooloff_interval_(1000),
      exec_algo_(kPassOnly),
      l1perc_limit_(0.2),
      max_lots_(-1),
      max_position_(-1),
      max_order_lots_(-1),
      max_order_size_(-1),
      use_nonbest_support_(false) {
    ParseParamFile(_paramfilename_);
  }
  void PortExecParamSet::ParseParamFile(const std::string& _paramfilename_) {
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
	if (strcmp(param_name_, "INSTRUMENT") == 0) {
	  instrument_ = param_value_;
	} else if (strcmp(param_name_, "COOLOFF_MSECS") == 0) {
	  trade_cooloff_interval_ = atoi(param_value_);
	}	else if (strcmp(param_name_, "EXEC_ALGO") == 0) {
	  if (strcmp(param_value_, "AGGONLY") == 0) {
	    exec_algo_ = kAggOnly;
	  } else if (strcmp(param_value_, "PASSANDAGG") == 0) {
	    exec_algo_ = kPassAndAgg;
	  } else if (strcmp(param_value_, "PASSONLY") == 0) {
	    exec_algo_ = kPassOnly;
	  } else {
	    std::cerr << "Wrong Exec Algo provided using passiveonly\n";
	  }
	} else if (strcmp(param_name_, "L1PERC_LIMIT") == 0) {
	  l1perc_limit_ = atof(param_value_);
	} else if (strcmp(param_name_, "MAX_LOTS") == 0) {
	  max_lots_ = atoi(param_value_);
	} else if (strcmp(param_name_, "MAX_ORDER_LOTS") == 0) {
	  max_order_lots_ = atoi(param_value_);
	} else if (strcmp(param_name_, "USE_NONBEST_SUPPORT") == 0) {
	  use_nonbest_support_ = (atoi(param_value_) > 0 ? true : false);
	}
	paramfile_.close();
      }
    }
  }

  void PortExecParamSet::scale_lot_values(const int _min_order_size_) {
    max_order_size_ = max_order_lots_ * _min_order_size_;
    max_position_ = max_lots_ * _min_order_size_;
  }

  std::string PortExecParamSet::ToString() {
    std::ostringstream t_retval_;
    t_retval_ << "INSTRUMENT: " << instrument_ << '\n';
    t_retval_ << "COOLOFF_INTERVAL: " << trade_cooloff_interval_ << " msecs\n";

    t_retval_ <<  "EXECUTION_TYPE" << ((exec_algo_ == kAggOnly) ? "AggOnly" : ((exec_algo_ == kPassOnly) ? "PassiveOnly" : "PassiveAndAgg"));

    t_retval_ << "L1PERC_LIMIT: " << l1perc_limit_ * 100.0 << " %\n";

    t_retval_ << "MAX_LOTS: " << max_lots_ << '\n';
    t_retval_ << "MAX_ORDER_LOTS: " << max_order_lots_ << '\n';

    t_retval_ << "USE_SUPPORTING_ORDERS: "  << (use_nonbest_support_ ? "Yes" : "No") << '\n';
    return t_retval_.str();
  }

}
 
