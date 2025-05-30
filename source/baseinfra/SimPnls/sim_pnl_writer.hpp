/**
    \file SmartOrderRouting/sim_pnl_writer.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/pnl_writer.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

namespace HFSAT {

class SimPnlWriter : public PnlWriter {
 private:
  BulkFileWriter& trades_writer_;
  const SecurityMarketView& dol_smv_;
  const SecurityMarketView& wdo_smv_;
  const SecurityMarketView* spread_smv_;
  char dol_secname_[1024];
  char wdo_secname_[1024];
  char spread_secname_[1024];

 public:
  SimPnlWriter(const SecurityMarketView& dol_smv, const SecurityMarketView& wdo_smv, BulkFileWriter& trades_writer,
               int runtime_id);

  SimPnlWriter(const SecurityMarketView& dol_smv, const SecurityMarketView& wdo_smv,
               const SecurityMarketView& spread_smv_, BulkFileWriter& trades_writer, int runtime_id);

  void WriteTrade(int security_id, ttime_t time, int trade_size, double trade_price, int new_position,
                  double opentrade_pnl, int total_pnl, int bestbid_size, double bestbid_price, double bestask_price,
                  int bestask_size, double mult_risk, int mult_base_pnl, char open_or_flat, char trade_type);
};
}
