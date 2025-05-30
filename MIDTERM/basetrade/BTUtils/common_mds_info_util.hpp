/**
    \file Tools/common_mds_info_util.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

 */

#pragma once

#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#define DELTA_TIME_IN_SECS 900

struct CommonMdsInfo {
  double l1_event_count_;
  double sum_l1_szs_;
  double num_trades_;
  double traded_volume_;
  double high_trade_price_;
  double low_trade_price_;
  double open_trade_price_;
  double close_trade_price_;
  double sum_sz_wt_trade_price_;
  std::vector<unsigned int> exch_mapped_volume_;
  std::vector<unsigned int> exch_mapped_trades_;
  std::vector<unsigned int> exch_total_volumes_;
  std::vector<double> exch_mapped_stdev_;
  std::vector<int> exch_data_timestamps_;

  CommonMdsInfo() {
    l1_event_count_ = 0.0;
    sum_l1_szs_ = 0.0;
    num_trades_ = 0.0;
    traded_volume_ = 0.0;
    high_trade_price_ = 0.0;
    low_trade_price_ = 9999999999.0;
    open_trade_price_ = 0.0;
    close_trade_price_ = 0.0;
    sum_sz_wt_trade_price_ = 0.0;
  }
};

class CommonMdsInfoUtil {
 public:
  CommonMdsInfoUtil(std::string shc_, int input_date_, int begin_secs_from_midnight, int end_secs_from_midnight,
                    int period = DELTA_TIME_IN_SECS);

  void Compute();
  void SetL1Mode(bool is_l1_mode);

  double GetL1EventCount();
  double GetL1AvgSize();
  double GetNumTrades();
  double GetVolumeTraded();
  double GetAvgTradeSize();
  double GetHighTradePrice();
  double GetLowTradePrice();
  double GetOpenTradePrice();
  double GetCloseTradePrice();
  double GetAvgTradePrice();

  std::vector<unsigned int> GetMappedTrades();
  std::vector<unsigned int> GetMappedVolume();
  std::vector<unsigned int> GetMappedTotalVolume();
  std::vector<double> GetMappedStdev();

  std::vector<int> GetTimeStamps();
  int GetStartTime();

  const char *GetExchangeSymbol();
  HFSAT::ExchSource_t GetExchangeSrc();

 private:
  void Initialize(int begin_secs_from_midnight, int end_secs_from_midnight);
  void HandleFirstEvent(int tv_sec, int tv_usec);
  void UpdateDatainFrame(int tv_sec);
  void FlushLeftOverData(int tv_sec);

  std::string shortcode_ = "";
  bool is_bmf_equity_;
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  const char *t_exchange_symbol_;
  HFSAT::TradingLocation_t trading_location_file_read_;
  HFSAT::BulkFileReader bulk_file_reader_;
  HFSAT::ExchSource_t exch_src_;
  CommonMdsInfo *trade_info_;
  int period_ = 0;
  int periodic_traded_volume_ = 0;
  int periodic_num_trades_ = 0;
  double periodic_stdev_ = 0;
  bool first_time_;
  int start_time_ = 0;
  struct timeval next_delta_time_event;
  bool is_hk_equity_;
  double sum_price_ = 0;
  double sum2_price_ = 0;
  bool is_l1_mode_;
};
