/**
 \file dvccode/CDef/online_debug_logger.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */
#include "dvccode/CDef/online_debug_logger.hpp"
#include <math.h>
namespace HFSAT {

OnlineDebugLogger::OnlineDebugLogger(size_t buf_capacity, int logging_id, size_t flush_trigger_size)
    : DebugLogger(buf_capacity, flush_trigger_size),
      client_logging_segment_initializer_ptr_(NULL),
      logging_id_(logging_id),
      log_buffer_(new HFSAT::CDef::LogBuffer()) {
  memset((void *)log_buffer_, 0, sizeof(HFSAT::CDef::LogBuffer));
  log_buffer_->content_type_ = HFSAT::CDef::UnstructuredText;
}

OnlineDebugLogger::~OnlineDebugLogger() { Close(); }

void OnlineDebugLogger::Close() {
  logfilename_.clear();
  loglevels_highervec_.clear();

  if (client_logging_segment_initializer_ptr_ != NULL) {
    DumpCurrentBuffer();
    client_logging_segment_initializer_ptr_->CleanUp();
  }

  if (NULL != in_memory_buffer_) {
    free(in_memory_buffer_);
    in_memory_buffer_ = NULL;
  }
}

void OnlineDebugLogger::DumpCurrentBuffer() {
  if (client_logging_segment_initializer_ptr_ != NULL) {
    int bytes_left = (int)front_marker_;
    int offset = 0;
    int bytes_to_write = 0;

    while (bytes_left > 0) {
      offset = front_marker_ - bytes_left;
      bytes_to_write = std::min(TEXT_BUFFER_SIZE, bytes_left);

      memcpy((void *)log_buffer_->buffer_data_.text_data_.buffer, (void *)(in_memory_buffer_ + offset), bytes_to_write);
      bytes_left -= bytes_to_write;
      log_buffer_->buffer_data_.text_data_.buffer[bytes_to_write] = '\0';
      client_logging_segment_initializer_ptr_->Log(log_buffer_);
    }
    front_marker_ = 0;
  }
}

void OnlineDebugLogger::OpenLogFile(const char *logfilename, std::ios_base::openmode open_mode) {
  if (client_logging_segment_initializer_ptr_ != NULL) {
    DumpCurrentBuffer();
    client_logging_segment_initializer_ptr_->CleanUp();
    delete client_logging_segment_initializer_ptr_;
  }

  logfilename_ = logfilename;

  TryOpeningFile();
}

void OnlineDebugLogger::TryOpeningFile() {
  // if directory does not exist create it
  FileUtils::MkdirEnclosing(logfilename_);
  size_t pos = logfilename_.rfind("/");
  std::string dir = logfilename_.substr(0, pos + 1);
  std::string filename = logfilename_.substr(pos + 1);
  client_logging_segment_initializer_ptr_ =
      new HFSAT::Utils::ClientLoggingSegmentInitializer(*this, logging_id_, dir.c_str(), filename.c_str());
}

void OnlineDebugLogger::LogQueryOrder(HFSAT::CDef::LogBuffer &buffer,char *sec_shortcode_, char *identifier_, ttime_t tv, TradeType_t side, bool is_eye,
                                      bool is_consec, bool is_mod, bool is_new, double price, int32_t size, double theo_bid_px,
                                      double theo_ask_px,
                                      double mkt_bid_px, double mkt_ask_px, double prim_bid_px, double prim_ask_px,
                                      double refprim_bid_px, double refprim_ask_px, int32_t opp_sz, int32_t caos,
                                      int32_t unique_exec_id, double prev_px, int32_t curr_int_price, int32_t dimer_int_price, bool is_dimer) {
  buffer.content_type_ = HFSAT::CDef::QueryOrder;

  HFSAT::CDef::QueryOrderStruct &order = buffer.buffer_data_.query_order_;
  strcpy(order.sec_shortcode, sec_shortcode_);
  strcpy(order.identifier_, identifier_);

  order.watch_tv_sec_ = tv.tv_sec;
  order.watch_tv_usec_ = tv.tv_usec;

  order.price_ = price;
  order.size_ = size;

  order.prev_px_ = prev_px;

  order.theo_bid_px_ = theo_bid_px;
  order.theo_ask_px_ = theo_ask_px;

  order.prim_bid_px_ = prim_bid_px;
  order.prim_ask_px_ = prim_ask_px;

  order.mkt_bid_px_ = mkt_bid_px;
  order.mkt_ask_px_ = mkt_ask_px;

   order.refprim_bid_px_ = refprim_bid_px;
  order.refprim_ask_px_ = refprim_ask_px;

   order.opp_sz_ = opp_sz;
  order.caos_ = caos;
  order.unique_exec_id_ = unique_exec_id;

   order.side_ =
      (side == (HFSAT::TradeType_t::kTradeTypeBuy)) ? 'B' : ((side == HFSAT::TradeType_t::kTradeTypeSell) ? 'S' : 'N');
  order.is_eye_ = is_eye;
  order.is_mod_ = is_mod;
  order.is_new_ = is_new;
  order.is_consec_ = is_consec;

   if (client_logging_segment_initializer_ptr_ != NULL) {
    client_logging_segment_initializer_ptr_->Log(&buffer);
   }
}

 void OnlineDebugLogger::LogQueryExec(HFSAT::CDef::LogBuffer &buffer, ttime_t tv, TradeType_t trade_type, int32_t position, int32_t caos,
                                     double trade_price, double theo_bid_px, double theo_ask_px,
                                     double mkt_bid_px, double mkt_ask_px, double primkt_bid_px,
                                     double primkt_ask_px, double refprim_bid_px, double refprim_ask_px,
                                     int32_t volume, int32_t target_pos, double total_traded_val, bool fill_type) {
  buffer.content_type_ = HFSAT::CDef::QueryExec;

  HFSAT::CDef::QueryExecStruct &exec = buffer.buffer_data_.query_exec_;
  exec.watch_tv_sec_ = tv.tv_sec;
  exec.watch_tv_usec_ = tv.tv_usec;

  exec.position_ = position;
  exec._caos_ = caos;
  exec.trade_price_ = trade_price;

  exec.theo_bid_px_ = theo_bid_px;
  exec.theo_ask_px_ = theo_ask_px;

  exec.mkt_bid_px_ = mkt_bid_px;
  exec.mkt_ask_px_ = mkt_ask_px;

  exec.primkt_bid_px_ = primkt_bid_px;
  exec.primkt_ask_px_ = primkt_ask_px;

  exec.refprim_bid_px_ = refprim_bid_px ;
  exec.refprim_ask_px_ = refprim_ask_px ;

  exec.volume_ = volume;
  exec.target_pos_ = target_pos;

  exec.total_traded_val_ = total_traded_val;
  exec.fill_type_ = fill_type;

  exec.trade_type_ = (trade_type == (HFSAT::TradeType_t::kTradeTypeBuy))
                     ? 'B'
                     : ((trade_type == HFSAT::TradeType_t::kTradeTypeSell) ? 'S' : 'N');

   if (client_logging_segment_initializer_ptr_ != NULL) {
    client_logging_segment_initializer_ptr_->Log(&buffer);
  }
}
}
