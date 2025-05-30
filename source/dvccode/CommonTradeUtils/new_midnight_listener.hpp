/**
    \file dvccode/CommonTradeUtils/new_midnight_listener.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_COMMONTRADEUTILS_NEW_MIDNIGHT_LISTENER_H
#define BASE_COMMONTRADEUTILS_NEW_MIDNIGHT_LISTENER_H
namespace HFSAT {

/// Notifies listeners of a new day event .. not used in real trading...
/// and even in historical studies not expected since we have daily files
class NewMidnightListener {
 public:
  virtual ~NewMidnightListener() {}

  virtual void OnNewMidNight() = 0;  // not expected in real trading : Watch detected new day event
};
}
#endif  // BASE_COMMONTRADEUTILS_NEW_MIDNIGHT_LISTENER_H
