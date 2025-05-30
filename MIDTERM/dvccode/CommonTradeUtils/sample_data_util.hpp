// =====================================================================================
//
//       Filename:  sample_data_util.hpp
//
//    Description:  Utility Functions for fetching the Sample Data values from the last 30 days
//
//        Version:  1.0
//        Created:  01/14/2016 04:31:28 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

// 60days features data should be present.{ matrix rows=timeperiod columns=dates }
// average for this period, rowavg -> vector GetAvgForPeriod {reference input}
// average for any period, rowavg(columnavg) -> double GetAvgForPeriod {return_value}
// average for day, rowavg(sumcolumn) -> double GetAvgForDay

#ifndef BASE_COMMONTRADEUTILS_SAMPLEDATAUTILS_H
#define BASE_COMMONTRADEUTILS_SAMPLEDATAUTILS_H

#include <string>
#include <vector>
#include <stdlib.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/holiday_manager_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_utils.hpp"

#define SAMPLEDATA_DIR "/spare/local/Features"
#define MAX_LOOKBACK 60  // else use the static value
#define LIVE_SAMPLEDATA_DIR "/spare/local/logs/datalogs"

namespace HFSAT {
namespace SampleDataUtil {

void SetLiveTrading(bool t_livetrading_ = true);
int GetSlotFromMfm(int mfm_);
int GetSlotFromHHMM(int hhmm_);
std::string GetSampleFileName(std::string feature_tag_);

void set_global_start_tz(std::string tz_hhmm_);
void set_global_end_tz(std::string tz_hhmm_);
void GetTZDiff(int date_, int current_date_);
int GetTZDiffStartTime(int date_);
int GetTZDiffEndTime(int date_);
void ClearSampleDataUtilsVars();

int LoadSampleFile(std::string shortcode_, int date_, std::string feature_tag_, std::map<int, double> &feature_values_);

void LoadSamplesForDay(std::string shortcode_, int date_, int start_mfm_, int end_mfm_, std::string feature_tag_,
                       std::map<int, double> &feature_values_);

/// this or any period, the key in feature_avg map is 15 min index of the day
double GetAvgForPeriod(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                       std::string feature_tag_, std::map<int, double> &feature_avg_, bool exit_on_error_ = true);

/// Get Avg Sample Value over All Samples and All Days
double GetAvgForPeriod(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                       std::string feature_tag_, bool exit_on_error_ = true);
double GetAvgForPeriod(std::string shortcode_, int date_, int numdays_, std::string feature_tag_,
                       bool exit_on_error_ = true);

/// Get Percentile for a Feature
double GetPercentileForPeriod(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                              std::string feature_tag_, double percentile_, bool exit_on_error_ = false);

/// Get Avg Value of the Feature Sum For each day
double GetAvgForDayTotal(std::string shortcode_, int date_, int numdays_, int start_mfm_, int end_mfm_,
                         std::string feature_tag_, bool exit_on_error_ = false);

/// Get the value of the last slot before the mentioned date
double GetLastSampleBeforeDate(std::string shortcode_, int date_, std::string feature_tag_, int numdays_ = 10,
                               bool exit_on_error_ = false);
}
}

#endif  // BASE_COMMONTRADEUTILS_SAMPLEDATAUTILS_H
