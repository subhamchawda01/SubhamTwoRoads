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

class VolumeRatioListener {
 public:
  virtual ~VolumeRatioListener(){};
  virtual void OnVolumeRatioUpdate(const unsigned int index_to_send_, const double& r_new_scaled_volume_value_) = 0;
};

class IndexedVolumeRatioListener {
 public:
  unsigned int index_to_send_;
  VolumeRatioListener* p_listener_;

  IndexedVolumeRatioListener(unsigned int& t_index_to_send_, VolumeRatioListener* t_p_listener_)
      : index_to_send_(t_index_to_send_), p_listener_(t_p_listener_) {}
};

typedef std::vector<VolumeRatioListener*> VolumeRatioListenerPtrVec;
// typedef std::vector < VolumeRatioListener * >::const_iterator VolumeRatioListenerPtrVecCIter_t ;
// typedef std::vector < VolumeRatioListener * >::iterator VolumeRatioListenerPtrVecIter_t ;
}
