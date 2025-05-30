/**
    \file Indicators/offline_returns_lrdb.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_OFFLINE_RETURNS_RLRDB_H
#define BASE_INDICATORS_OFFLINE_RETURNS_RLRDB_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

typedef std::map<std::string, LRInfo> ShortcodeLRInfoMap;
typedef std::map<std::string, LRInfo>::iterator ShortcodeLRInfoMapIter;

typedef std::vector<ShortcodeLRInfoMap> ShortcodeLRInfoMapVec;

class OfflineReturnsRetLRDB {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::string lrdbfile_base_dir_;
  const char dep_indep_sep_;
  unsigned int lrdb_session_now_;
  const std::string& dep_shortcode_;
  const std::string& dep_exchange_;
  unsigned int lasttznowset_;
  std::vector<unsigned int> lrdb_exchange_sessions_;
  std::vector<char*> lrdb_exchange_sessions_start_end_vec_;
  ShortcodeLRInfoMapVec codes_to_lrinfo_map_vec_;

  // functions
 protected:
  OfflineReturnsRetLRDB(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& _dep_shortcode_);

 public:
  static OfflineReturnsRetLRDB& GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::string& _dep_shortcode_) {
    static OfflineReturnsRetLRDB uniqueinstance_(_dbglogger_, _watch_, _dep_shortcode_);
    return uniqueinstance_;
  }

 public:
  ~OfflineReturnsRetLRDB() {}

  bool LRCoeffPresent(const std::string& t_dep_shortcode_, const std::string& t_indep_portfolio_shortcode_) const;
  LRInfo GetLRCoeff(std::string _dep_shortcode_, std::string _indep_portfolio_shortcode_);

 protected:
  void ReadDBFile(const unsigned int _lrdb_session_, std::string _lrdb_filename_);
  void ReloadDB();
  void PatchLRDB();
  std::string GetLRDBFullFileName(int _intdate_, const char* _lrdb_session_start_end_);
  std::string GetDefaultLRDBFullFileName(const char* _lrdb_session_start_end_);

  void SetLRDBSessionNow();
  void GetLRDBExchangeSessions();
};
}

#endif  // BASE_INDICATORS_OFFLINE_RETURNS_LRDB_H
