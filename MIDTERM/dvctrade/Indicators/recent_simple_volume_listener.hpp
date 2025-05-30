/**
    \file Indicators/recent_simple_volume_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once
namespace HFSAT {

/// Common interface extended by all classes listening to RecentSimpleVolumeMeasure
/// to listen to changes in online computed stdev of the product
class RecentSimpleVolumeListener {
 public:
  virtual ~RecentSimpleVolumeListener(){};
  virtual void OnVolumeUpdate(unsigned int index_to_send_, double r_new_volume_value_) = 0;
};

class IndexedRecentSimpleVolumeListener {
 public:
  unsigned int index_to_send_;
  RecentSimpleVolumeListener* p_listener_;

  IndexedRecentSimpleVolumeListener(unsigned int& t_index_to_send_, RecentSimpleVolumeListener* t_p_listener_)
      : index_to_send_(t_index_to_send_), p_listener_(t_p_listener_) {}
};

typedef std::vector<RecentSimpleVolumeListener*> RecentSimpleVolumeListenerPtrVec;
}
