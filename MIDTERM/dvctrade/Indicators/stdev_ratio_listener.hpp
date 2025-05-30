/**
    \file Indicators/volume_ratio_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once
namespace HFSAT {

class StdevRatioListener {
 public:
  virtual ~StdevRatioListener(){};
  virtual void OnStdevRatioUpdate(const unsigned int index_to_send_, const double& r_new_scaled_volume_value_) = 0;
};

class IndexedStdevRatioListener {
 public:
  unsigned int index_to_send_;
  StdevRatioListener* p_listener_;

  IndexedStdevRatioListener(unsigned int& t_index_to_send_, StdevRatioListener* t_p_listener_)
      : index_to_send_(t_index_to_send_), p_listener_(t_p_listener_) {}
};

typedef std::vector<StdevRatioListener*> StdevRatioListenerPtrVec;
}
