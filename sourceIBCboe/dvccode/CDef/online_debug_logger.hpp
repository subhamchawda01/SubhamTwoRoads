/**
 \file dvccode/CDef/online_debug_logger.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */
#pragma once

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"

namespace HFSAT {

class OnlineDebugLogger : public DebugLogger {
 protected:
  HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_ptr_;
  int logging_id_;
  HFSAT::CDef::LogBuffer* log_buffer_;

 public:
  OnlineDebugLogger(size_t buf_capacity, int logging_id, size_t flush_trigger_size = 0);

  ~OnlineDebugLogger();

  void Close();

  void DumpCurrentBuffer();

  void OpenLogFile(const char* logfilename, std::ios_base::openmode open_mode);

  void TryOpeningFile();
 
  void LogQueryOrder(HFSAT::CDef::LogBuffer &buffer, char const *sec_shortcode_,char const *identifier_, ttime_t tv, TradeType_t side, bool is_eye, bool is_consec,
                     bool is_mod, bool is_new, double price, int32_t size, double theo_bid_px, double theo_ask_px,
                     double mkt_bid_px, double mkt_ask_px, double prim_bid_px, double prim_ask_px, double refprim_bid_px,
                     double refprim_ask_px, int32_t opp_sz, int32_t caos, int32_t unique_exec_id, double prev_px, int32_t curr_int_price = -1, int32_t dimer_int_price = -1, bool is_dimer = false);

  void LogQueryExec(HFSAT::CDef::LogBuffer &buffer, ttime_t tv, TradeType_t trade_type, int32_t position, int32_t caos, double trade_price,
                    double theo_bid_px, double theo_ask_px, double mkt_bid_px, double mkt_ask_px,
                    double prim_bid_px, double prim_ask_px, double refprim_bid_px, double refprim_ask_px,
                    int32_t volume, int32_t target_pos, double total_traded_val, bool fill_type);
};
}
