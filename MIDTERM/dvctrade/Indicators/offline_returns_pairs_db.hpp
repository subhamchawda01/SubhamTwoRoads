/**
    \file Indicators/offline_returns_lrdb.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_OFFLINE_RETURNS_PAIRS_DB_H
#define BASE_INDICATORS_OFFLINE_RETURNS_PAIRS_DB_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

namespace HFSAT {
// class to maintain a map of source_portfolio, dep_portfolio for each time zone
// to return the offline computed regression coefficient

typedef enum { kRTZPreGMT0, kRTZPreGMT6, kRTZPreGMT12, kRTZPreGMT18, kRTZMAX } ReturnsDBTimeZone;

struct ReturnsInfo {
  std::vector<double> returns_coeff_vec_;
  std::vector<std::string> shortcode_vec_;

  ReturnsInfo() : returns_coeff_vec_(), shortcode_vec_() {}

  ReturnsInfo(const std::vector<double> _ret_vec_, const std::vector<std::string> _shc_vec_)
      : returns_coeff_vec_(_ret_vec_), shortcode_vec_(_shc_vec_) {}
};

typedef std::map<std::string, ReturnsInfo> ShortcodeReturnsInfoMap;
typedef std::map<std::string, ReturnsInfo>::iterator ShortcodeReturnsInfoMapIter;

typedef std::vector<ShortcodeReturnsInfoMap> ShortcodeReturnsInfoMapVec;

class OfflineReturnsPairsDB {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::string lrdbfile_base_dir_;
  ReturnsDBTimeZone returns_db_time_zone_now_;
  unsigned int lasttznowset_;
  ShortcodeReturnsInfoMapVec codes_to_lrinfo_map_vec_;

  // functions
 protected:
  OfflineReturnsPairsDB(DebugLogger& _dbglogger_, const Watch& _watch_);

 public:
  static OfflineReturnsPairsDB& GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_) {
    static OfflineReturnsPairsDB uniqueinstance_(_dbglogger_, _watch_);
    return uniqueinstance_;
  }

  static void CollectSourceShortcodes(const std::string& _dep_shc_,
                                      std::vector<std::string>& t_shortcodes_affecting_this_indicator_);
  std::string GetSourcePortfolioForShortcode(const std::string dep_shc_);
  std::vector<std::string> GetSourcesForShortcode(const std::string& dep_shc_);

 public:
  ~OfflineReturnsPairsDB() {}

  bool ReturnsCoeffPresent(const std::string& t_dep_shortcode_, const std::string& t_indep_portfolio_shortcode_);
  ReturnsInfo GetReturnsCoeff(std::string _dep_shortcode_, std::string _indep_portfolio_shortcode_);

 protected:
  void ReloadDB();
  void ReadDBFile(ReturnsDBTimeZone _lrdb_time_zone_, std::string _lrdb_filename_);
  static std::string GetReturnsDBFullFileName(int _intdate_, ReturnsDBTimeZone _lrdb_time_zone_);
  std::string GetDefaultReturnsDBFullFileName(ReturnsDBTimeZone _lrdb_time_zone_);

  void SetReturnsDBTimeZoneNow();
  void ConstructFileName(const std::string base_dir_, std::string filename_);
};
}

#endif  // BASE_INDICATORS_OFFLINE_RETURNS_LRDB_H
