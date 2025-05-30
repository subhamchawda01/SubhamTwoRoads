/**
\file SmartOrderRoutingCode/live_pnl_writer.cpp

\author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/

#include "baseinfra/LivePnls/live_pnl_writer.hpp"

namespace HFSAT {

LivePnlWriter::LivePnlWriter(const SecurityMarketView& dol_smv, const SecurityMarketView& wdo_smv,
                             Utils::ClientLoggingSegmentInitializer* client_logging, int runtime_id)
    : client_logging_(client_logging),
      dol_buffer_(new CDef::LogBuffer()),
      wdo_buffer_(new CDef::LogBuffer()),
      spread_buffer_(new CDef::LogBuffer()),
      dol_smv_(dol_smv),
      wdo_smv_(wdo_smv),
      spread_smv_(NULL) {
  bzero(dol_secname_, 40);
  snprintf(dol_secname_, 40, "%s.%d", dol_smv.secname(), runtime_id);
  memset((void*)dol_buffer_, 0, sizeof(CDef::LogBuffer));
  dol_buffer_->content_type_ = HFSAT::CDef::QueryTrade;
  memcpy((void*)dol_buffer_->buffer_data_.query_trade_.security_name_, (void*)dol_secname_, 40);

  bzero(wdo_secname_, 40);
  snprintf(wdo_secname_, 40, "%s.%d", wdo_smv.secname(), runtime_id);
  memset((void*)wdo_buffer_, 0, sizeof(CDef::LogBuffer));
  wdo_buffer_->content_type_ = HFSAT::CDef::QueryTrade;
  memcpy((void*)wdo_buffer_->buffer_data_.query_trade_.security_name_, (void*)wdo_secname_, 40);

  bzero(spread_secname_, 40);
}

LivePnlWriter::LivePnlWriter(const SecurityMarketView& dol_smv, const SecurityMarketView& wdo_smv,
                             const SecurityMarketView& spread_smv,
                             Utils::ClientLoggingSegmentInitializer* client_logging, int runtime_id)
    : client_logging_(client_logging),
      dol_buffer_(new CDef::LogBuffer()),
      wdo_buffer_(new CDef::LogBuffer()),
      spread_buffer_(new CDef::LogBuffer()),
      dol_smv_(dol_smv),
      wdo_smv_(wdo_smv),
      spread_smv_(&spread_smv) {
  bzero(dol_secname_, 40);
  snprintf(dol_secname_, 40, "%s.%d", dol_smv.secname(), runtime_id);
  memset((void*)dol_buffer_, 0, sizeof(CDef::LogBuffer));
  dol_buffer_->content_type_ = HFSAT::CDef::QueryTrade;
  memcpy((void*)dol_buffer_->buffer_data_.query_trade_.security_name_, (void*)dol_secname_, 40);

  bzero(wdo_secname_, 40);
  snprintf(wdo_secname_, 40, "%s.%d", wdo_smv.secname(), runtime_id);
  memset((void*)wdo_buffer_, 0, sizeof(CDef::LogBuffer));
  wdo_buffer_->content_type_ = HFSAT::CDef::QueryTrade;
  memcpy((void*)wdo_buffer_->buffer_data_.query_trade_.security_name_, (void*)wdo_secname_, 40);

  bzero(spread_secname_, 40);
  snprintf(spread_secname_, 40, "%s.%d", spread_smv.secname(), runtime_id);
  memset((void*)spread_buffer_, 0, sizeof(CDef::LogBuffer));
  spread_buffer_->content_type_ = HFSAT::CDef::QueryTrade;
  memcpy((void*)spread_buffer_->buffer_data_.query_trade_.security_name_, (void*)spread_secname_, 40);
}
void LivePnlWriter::WriteTrade(int security_id, ttime_t time, int trade_size, double trade_price, int new_position,
                               double opentrade_pnl, int total_pnl, int bestbid_size, double bestbid_price,
                               double bestask_price, int bestask_size, double mult_risk, int mult_base_pnl,
                               char open_or_flat, char trade_type) {
  CDef::LogBuffer* log_buffer = NULL;
  if (security_id == (int)dol_smv_.security_id()) {
    log_buffer = dol_buffer_;
  } else if (security_id == (int)wdo_smv_.security_id()) {
    log_buffer = wdo_buffer_;
  } else if (spread_smv_ != NULL && security_id == (int)spread_smv_->security_id()) {
    log_buffer = spread_buffer_;
  } else {
    return;
  }

  log_buffer->buffer_data_.query_trade_.watch_tv_sec_ = time.tv_sec;
  log_buffer->buffer_data_.query_trade_.watch_tv_usec_ = time.tv_usec;
  log_buffer->buffer_data_.query_trade_.trade_size_ = trade_size;
  log_buffer->buffer_data_.query_trade_.trade_price_ = trade_price;
  log_buffer->buffer_data_.query_trade_.new_position_ = new_position;
  log_buffer->buffer_data_.query_trade_.open_unrealized_pnl_ = opentrade_pnl;
  log_buffer->buffer_data_.query_trade_.total_pnl_ = total_pnl;
  log_buffer->buffer_data_.query_trade_.bestbid_size_ = bestbid_size;
  log_buffer->buffer_data_.query_trade_.bestbid_price_ = bestbid_price;
  log_buffer->buffer_data_.query_trade_.bestask_price_ = bestask_price;
  log_buffer->buffer_data_.query_trade_.bestask_size_ = bestask_size;
  log_buffer->buffer_data_.query_trade_.mult_risk_ = mult_risk;
  log_buffer->buffer_data_.query_trade_.mult_base_pnl_ = mult_base_pnl;
  log_buffer->buffer_data_.query_trade_.open_or_flat_ = open_or_flat;
  log_buffer->buffer_data_.query_trade_.trade_type_ = trade_type;

  client_logging_->Log(log_buffer);
}
}
