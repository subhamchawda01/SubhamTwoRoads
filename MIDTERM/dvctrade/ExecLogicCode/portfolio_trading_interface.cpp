/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
**/

#include "dvctrade/ExecLogic/portfolio_trading_interface.hpp"


namespace HFSAT {
  std::vector<PortExecParamSet*> PortfolioTradingInterface::paramset_vec_;

  void PortfolioTradingInterface::CollectPortExecParamSetVec(std::vector<PortExecParamSet*>& _paramset_vec_,
								    const std::string& _paramfile_listname_) {
    std::ifstream t_paramlist_stream_;
    t_paramlist_stream_.open(_paramfile_listname_.c_str(), std::ifstream::in);
    if (t_paramlist_stream_.is_open()) {
      const int kParamFileLineBufferLen = 1024;
      char readline_buffer_[kParamFileLineBufferLen];
      bzero(readline_buffer_, kParamFileLineBufferLen);

      while (t_paramlist_stream_.good()) {
	bzero(readline_buffer_, kParamFileLineBufferLen);
	t_paramlist_stream_.getline(readline_buffer_, kParamFileLineBufferLen);
	
	std::string t_param_filename_(readline_buffer_);
	boost::trim(t_param_filename_);
	if (t_param_filename_ == "") continue;
	PortExecParamSet* t_paramset_ = new PortExecParamSet(t_param_filename_);
	_paramset_vec_.push_back(t_paramset_);
	paramset_vec_.push_back(t_paramset_);
      }
      t_paramlist_stream_.close();
    }
    if (_paramset_vec_.size() < 1) {
      std::cerr << "could not create and port exec param objects, nothing to do, exiting ..\n";
      exit(-1);
    }
  }
}
