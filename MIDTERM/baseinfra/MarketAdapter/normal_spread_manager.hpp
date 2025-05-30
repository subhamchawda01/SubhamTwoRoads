/**
   \file MarketAdapter/normal_spread_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MARKETADAPTER_NORMAL_SPREAD_MANAGER_H
#define BASE_MARKETADAPTER_NORMAL_SPREAD_MANAGER_H

#include <vector>
#include <string>
#include <map>

namespace HFSAT {

/// \brief Class to read the normal spread increments info from a file per product
/// The class is instantiated by date. Typically we expect only one day's data to be
/// used in one instance
class NormalSpreadManager {
 protected:
  const int yyyymmdd_;
  std::string t_normal_spread_info_filename_;

  std::map<std::string, double> shortcode_normal_spread_increment_map_;

  NormalSpreadManager(const int t_yyyymmdd_);

 public:
  /// Only called internally by NormalSpreadManager::GetNormalSpreadIncrements
  static NormalSpreadManager* GetUniqueInstance(const int t_yyyymmdd_);

  ~NormalSpreadManager() {}

  /// Called by SecurityMarketView during contruction
  static double GetNormalSpreadIncrements(const int t_yyyymmdd_, const std::string& t_shortcode_);

 protected:
  /// Called by contructor to load the data for this day
  void LoadNormalSpreadInfoFile();

  double t_GetNormalSpreadIncrements(const std::string& t_shortcode_);
};
}

#endif
