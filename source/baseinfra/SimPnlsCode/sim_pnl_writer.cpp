/**
    \file SmartOrderRoutingCode/sim_pnl_writer.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include "baseinfra/SimPnls/sim_pnl_writer.hpp"

namespace HFSAT {

SimPnlWriter::SimPnlWriter(const SecurityMarketView& dol_smv, const SecurityMarketView& wdo_smv,
                           BulkFileWriter& trades_writer, int runtime_id)
    : trades_writer_(trades_writer), dol_smv_(dol_smv), wdo_smv_(wdo_smv), spread_smv_(NULL) {
  bzero(dol_secname_, 24);
  snprintf(dol_secname_, 24, "%s.%d", dol_smv.secname(), runtime_id);

  bzero(wdo_secname_, 24);
  snprintf(wdo_secname_, 24, "%s.%d", wdo_smv.secname(), runtime_id);

  bzero(spread_secname_, 24);
}

SimPnlWriter::SimPnlWriter(const SecurityMarketView& dol_smv, const SecurityMarketView& wdo_smv,
                           const SecurityMarketView& spread_smv, BulkFileWriter& trades_writer, int runtime_id)
    : trades_writer_(trades_writer), dol_smv_(dol_smv), wdo_smv_(wdo_smv), spread_smv_(&spread_smv) {
  bzero(dol_secname_, 24);
  snprintf(dol_secname_, 24, "%s.%d", dol_smv.secname(), runtime_id);

  bzero(wdo_secname_, 24);
  snprintf(wdo_secname_, 24, "%s.%d", wdo_smv.secname(), runtime_id);

  bzero(spread_secname_, 24);
  snprintf(spread_secname_, 24, "%s.%d", spread_smv.secname(), runtime_id);
}

void SimPnlWriter::WriteTrade(int security_id, ttime_t time, int trade_size, double trade_price, int new_position,
                              double opentrade_pnl, int total_pnl, int bestbid_size, double bestbid_price,
                              double bestask_price, int bestask_size, double mult_risk, int mult_base_pnl,
                              char open_or_flat, char trade_type) {
  char* secname = dol_secname_;
  if (security_id == (int)wdo_smv_.security_id()) {
    secname = wdo_secname_;
  } else if (spread_smv_ != NULL && security_id == (int)spread_smv_->security_id()) {
    secname = spread_secname_;
  }

  char buf[1024] = {0};
  sprintf(buf, "%10d.%06d %s %s %c %4d %.7f %4d %8d %8d [ %5d %f X %f %5d ] %.3f %d", time.tv_sec, time.tv_usec,
          (open_or_flat == 'O' ? "OPEN" : "FLAT"), secname, trade_type, abs(trade_size), trade_price, new_position,
          (int)opentrade_pnl, total_pnl, bestbid_size, bestbid_price, bestask_price, bestask_size, mult_risk,
          mult_base_pnl);
  trades_writer_ << buf << '\n';
  trades_writer_.CheckToFlushBuffer();
}
}
