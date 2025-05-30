/**
    \file Indicators/recent_scaled_volume_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once
namespace HFSAT {

class RecentScaledVolumeListener {
 public:
  virtual ~RecentScaledVolumeListener(){};
  virtual void OnScaledVolumeUpdate(const unsigned int index_to_send_, const double& r_new_scaled_volume_value_) = 0;
};

class IndexedRecentScaledVolumeListener {
 public:
  unsigned int index_to_send_;
  RecentScaledVolumeListener* p_listener_;

  IndexedRecentScaledVolumeListener(unsigned int& t_index_to_send_, RecentScaledVolumeListener* t_p_listener_)
      : index_to_send_(t_index_to_send_), p_listener_(t_p_listener_) {}
};

typedef std::vector<RecentScaledVolumeListener*> RecentScaledVolumeListenerPtrVec;
// typedef std::vector < RecentScaledVolumeListener * >::const_iterator RecentScaledVolumeListenerPtrVecCIter_t ;
// typedef std::vector < RecentScaledVolumeListener * >::iterator RecentScaledVolumeListenerPtrVecIter_t ;
}
