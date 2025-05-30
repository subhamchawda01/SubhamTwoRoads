/**
   \file Indicators/historical_stdev_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_HISTORICAL_STDEV_MANAGER_H
#define BASE_INDICATORS_HISTORICAL_STDEV_MANAGER_H

#include "dvccode/CDef/error_utils.hpp"

namespace HFSAT {

/// Used to store 120 second price change stdev information per product
class HistoricalStdevManager {
 protected:
  std::map<std::string, double> shortcode_stdev_map_;

  HistoricalStdevManager() { LoadStdevInfoFile(); }

 public:
  static HistoricalStdevManager &GetUniqueInstance() {
    static HistoricalStdevManager uniqueinstance_;
    return uniqueinstance_;
  }

  ~HistoricalStdevManager() {}

  const double GetStdev(const std::string &r_shortcode_) const {
    std::map<std::string, double>::const_iterator _citer_ = shortcode_stdev_map_.find(r_shortcode_);
    if (_citer_ != shortcode_stdev_map_.end()) {
      return _citer_->second;
    }
    ExitVerbose(kHistoricalStdevManagerMissingShortcodeFromMap, r_shortcode_.c_str());
    return 10000;
  }

 protected:
  /// Reads a stdev info file of the form
  /// shortcode stdev_120s
  void LoadStdevInfoFile();

  void LoadFromFile(const std::string &, std::map<std::string, unsigned int> &, unsigned int);
};
}
#endif  // BASE_INDICATORS_HISTORICAL_STDEV_MANAGER_H
