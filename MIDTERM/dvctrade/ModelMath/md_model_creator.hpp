/**
        \file ModelMath/md_model_creator.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#pragma once

#include <map>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/ModelMath/md_indicator_logger.hpp"

namespace HFSAT {

class MDModelCreator {
 protected:
  typedef enum {
    kMDModelCreationPhaseModelInit,
    kMDModelCreationPhasePostSubModelMath,
    kMDModelCreationPhaseModelMath,
    kMDModelCreationPhaseSubModelInit,
    kMDModelCreationPhaseSubModelMath,
    kMDModelCreationPhaseSubModelIndicatorStarted,
    kMDModelCreationPhaseSubModelEnd,
    kMDModelCreationPhaseShortCodes,
    kMDModelCreationPhaseIIndicators,
    kMDModelCreationPhaseGIndicators,
    kMDModelCreationPhaseModelEnd,
    kMDModelCreationPhaseEnd,
    kMDModelCreationPhaseMAX
  } MDModelCreationPhases_t;  ///< internal enum for Finite State Machine during creation

  // typedef enum { kGreekDelta, kGreekGamma, kGreekVega, kGreekTheta, kGreekMax } GreekIndentifier_t;

  static std::vector<SecurityMarketView*> dep_market_view_vec_;
  static std::vector<PriceType_t> baseprice_vec_;

 public:
  ~MDModelCreator() {
    dep_market_view_vec_.clear();
    baseprice_vec_.clear();
  }

  static void CollectMDModelShortCodes(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       std::string _instruction_filename_,
                                       std::vector<std::string>& _dependant_shortcodes_,
                                       std::vector<std::string>& _source_shortcodes_);

  // static CommonIndicator * GetIndicatorFromTokens ( DebugLogger & _dbglogger_, const Watch & _watch_, const
  // std::vector < const char * > & tokens_ ) ;
  static CommonIndicator* GetIndicatorFromTokens(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& tokens_, PriceType_t _basepx_pxtype_);

  // modelling :
  static MDIndicatorLogger* CreateMDIndicatorLogger(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                    // BulkFileWriter& _bulk_file_writer_,
                                                    const std::string& _instruction_filename_,
                                                    // const std::string& _output,
                                                    const unsigned int t_msecs_to_wait_to_print_again_,
                                                    const unsigned int t_num_trades_to_wait_print_again_);

  static void LinkupLoggerToOnReadySources(MDIndicatorLogger* p_md_logger_,
                                           std::vector<std::string>& shortcodes_affecting_this_model_);
};
}
