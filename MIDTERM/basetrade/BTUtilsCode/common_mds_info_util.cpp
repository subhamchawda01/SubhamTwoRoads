#include "basetrade/BTUtils/common_mds_info_util.hpp"

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkstocks_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/nse_l1_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/ose_pf_logged_message_filesource.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"

CommonMdsInfoUtil::CommonMdsInfoUtil(std::string shc_, int input_date_, int begin_secs_from_midnight,
                                     int end_secs_from_midnight, int period)
    : shortcode_(shc_), input_date_(input_date_), period_(period) {
  Initialize(begin_secs_from_midnight, end_secs_from_midnight);
}

double CommonMdsInfoUtil::GetL1EventCount() { return trade_info_->l1_event_count_; }

double CommonMdsInfoUtil::GetL1AvgSize() {
  return ((trade_info_->l1_event_count_ > 0) ? trade_info_->sum_l1_szs_ / trade_info_->l1_event_count_ : 0);
}
double CommonMdsInfoUtil::GetNumTrades() { return trade_info_->num_trades_; }

double CommonMdsInfoUtil::GetVolumeTraded() { return trade_info_->traded_volume_; }

std::vector<unsigned int> CommonMdsInfoUtil::GetMappedTrades() { return trade_info_->exch_mapped_trades_; }

std::vector<unsigned int> CommonMdsInfoUtil::GetMappedVolume() { return trade_info_->exch_mapped_volume_; }

std::vector<double> CommonMdsInfoUtil::GetMappedStdev() { return trade_info_->exch_mapped_stdev_; }

std::vector<unsigned int> CommonMdsInfoUtil::GetMappedTotalVolume() { return trade_info_->exch_total_volumes_; }

std::vector<int> CommonMdsInfoUtil::GetTimeStamps() { return trade_info_->exch_data_timestamps_; }

int CommonMdsInfoUtil::GetStartTime() { return start_time_; }

double CommonMdsInfoUtil::GetAvgTradeSize() {
  return ((trade_info_->num_trades_ > 0) ? (trade_info_->traded_volume_ / trade_info_->num_trades_) : 0);
}

double CommonMdsInfoUtil::GetHighTradePrice() { return trade_info_->high_trade_price_; }
double CommonMdsInfoUtil::GetLowTradePrice() { return trade_info_->low_trade_price_; }
double CommonMdsInfoUtil::GetOpenTradePrice() { return trade_info_->open_trade_price_; }
double CommonMdsInfoUtil::GetCloseTradePrice() { return trade_info_->close_trade_price_; }
double CommonMdsInfoUtil::GetAvgTradePrice() {
  return (trade_info_->traded_volume_ > 0 ? (double)trade_info_->sum_sz_wt_trade_price_ / trade_info_->traded_volume_
                                          : 0);
}
const char *CommonMdsInfoUtil::GetExchangeSymbol() { return t_exchange_symbol_; }

HFSAT::ExchSource_t CommonMdsInfoUtil::GetExchangeSrc() { return exch_src_; }

void CommonMdsInfoUtil::SetL1Mode(bool is_l1_mode) { is_l1_mode_ = is_l1_mode; }

void CommonMdsInfoUtil::Initialize(int begin_secs_from_midnight, int end_secs_from_midnight) {
  if (begin_secs_from_midnight > 0 && begin_secs_from_midnight < 24 * 60 * 60) {
    begin_secs_from_midnight_ = begin_secs_from_midnight;
  }

  if (end_secs_from_midnight > 0 && end_secs_from_midnight < 24 * 60 * 60) {
    end_secs_from_midnight_ = end_secs_from_midnight;
  }

  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  }

  if (strncmp(shortcode_.c_str(), "HK_", 3) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadHKStocksSecurityDefinitions();
    is_hk_equity_ = true;
  } else {
    is_hk_equity_ = false;
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  trading_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);  // initialize to primary location

  is_bmf_equity_ = (shortcode_ == std::string(t_exchange_symbol_));

  if (shortcode_.find("DI1") != std::string::npos) {
    is_bmf_equity_ = false;
  }

  if ((exch_src_ == HFSAT::kExchSourceHONGKONG && trading_location_file_read_ == HFSAT::kTLocHK &&
       (input_date_ >= 20141204)) ||
      ((exch_src_ == HFSAT::kExchSourceHONGKONG) && (trading_location_file_read_ == HFSAT::kTLocJPY) &&
       (input_date_ >= 20150121))) {
    exch_src_ = HFSAT::kExchSourceHKOMDCPF;
  }

  trade_info_ = new CommonMdsInfo;

  first_time_ = true;
  next_delta_time_event.tv_sec = 0;
  next_delta_time_event.tv_usec = 0;
}

void CommonMdsInfoUtil::HandleFirstEvent(int tv_sec, int tv_usec) {
  if (first_time_ && tv_sec > 0) {
    if (tv_sec <= 0.0) {
      return;
    }  // to avoid intermediate msgs
    next_delta_time_event.tv_sec = HFSAT::MathUtils::GetFlooredMultipleOf(tv_sec, period_) + period_;
    next_delta_time_event.tv_usec = 0;

    start_time_ = tv_sec;
    first_time_ = false;
  }
}

void CommonMdsInfoUtil::UpdateDatainFrame(int tv_sec) {
  if (tv_sec >= next_delta_time_event.tv_sec) {
    if (periodic_traded_volume_ > 0 && periodic_num_trades_ > 0) {
      periodic_stdev_ =
          sqrt(std::max(0.0, sum2_price_ / periodic_num_trades_ -
                                 (sum_price_ * sum_price_) / (periodic_num_trades_ * periodic_num_trades_)));
      trade_info_->exch_mapped_volume_.push_back(periodic_traded_volume_);
      trade_info_->exch_mapped_trades_.push_back(periodic_num_trades_);
      trade_info_->exch_total_volumes_.push_back(trade_info_->traded_volume_);
      trade_info_->exch_mapped_stdev_.push_back(periodic_stdev_);

      trade_info_->exch_data_timestamps_.push_back(next_delta_time_event.tv_sec);
    }
    next_delta_time_event.tv_sec +=
        HFSAT::MathUtils::GetFlooredMultipleOf(tv_sec - next_delta_time_event.tv_sec, period_) + period_;

    periodic_traded_volume_ = 0;
    periodic_num_trades_ = 0;
    periodic_stdev_ = 0;
    sum_price_ = 0;
    sum2_price_ = 0;
  }
}

void CommonMdsInfoUtil::FlushLeftOverData(int tv_sec) {
  if (periodic_traded_volume_ > 0 && periodic_num_trades_ > 0) {
    periodic_stdev_ =
        sqrt(std::max(0.0, sum2_price_ / periodic_num_trades_ -
                               (sum_price_ * sum_price_) / (periodic_num_trades_ * periodic_num_trades_)));
    trade_info_->exch_mapped_volume_.push_back(periodic_traded_volume_);
    trade_info_->exch_mapped_trades_.push_back(periodic_num_trades_);
    trade_info_->exch_total_volumes_.push_back(trade_info_->traded_volume_);
    trade_info_->exch_mapped_stdev_.push_back(periodic_stdev_);

    trade_info_->exch_data_timestamps_.push_back(tv_sec);
  }
}

void CommonMdsInfoUtil::Compute() {
  int bid_sz_ = 0;
  int ask_sz_ = 0;

  switch (exch_src_) {
    case HFSAT::kExchSourceCME: {
      std::string t_cme_filename_ =
          HFSAT::CMELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      CME_MDS::CMECommonStruct next_event_;
      bulk_file_reader_.open(t_cme_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));

        if (available_len_ >= sizeof(CME_MDS::CMECommonStruct)) {  // data not found in file

          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == CME_MDS::CME_DELTA) {
            if (next_event_.data_.cme_dels_.level_ == 1 &&
                next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.cme_dels_.type_ == 0) {
                bid_sz_ = next_event_.data_.cme_dels_.size_;
              } else {
                ask_sz_ = next_event_.data_.cme_dels_.size_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          } else if (next_event_.msg_ == CME_MDS::CME_TRADE) {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.cme_trds_.trd_qty_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.cme_trds_.trd_qty_ * next_event_.data_.cme_trds_.trd_px_;
              trade_info_->high_trade_price_ = ((next_event_.data_.cme_trds_.trd_px_ > trade_info_->high_trade_price_)
                                                    ? next_event_.data_.cme_trds_.trd_px_
                                                    : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ = ((next_event_.data_.cme_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                   ? next_event_.data_.cme_trds_.trd_px_
                                                   : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.cme_trds_.trd_px_
                                                                                : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.cme_trds_.trd_px_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.cme_trds_.trd_px_;
                sum2_price_ += next_event_.data_.cme_trds_.trd_px_ * next_event_.data_.cme_trds_.trd_px_;
                periodic_traded_volume_ += next_event_.data_.cme_trds_.trd_qty_;
                periodic_num_trades_++;
              }
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }
    } break;

    case HFSAT::kExchSourceLIFFE: {
      std::string t_liffe_filename_ =
          HFSAT::LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      LIFFE_MDS::LIFFECommonStruct next_event_;
      bulk_file_reader_.open(t_liffe_filename_);
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
        if (available_len_ >= sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file

          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == LIFFE_MDS::LIFFE_DELTA) {
            if (next_event_.data_.liffe_dels_.level_ == 1 &&
                next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.liffe_dels_.type_ == '1') {
                bid_sz_ = next_event_.data_.liffe_dels_.size_;
              } else if (next_event_.data_.liffe_dels_.type_ == '2') {
                ask_sz_ = next_event_.data_.liffe_dels_.size_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          } else if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.liffe_trds_.trd_qty_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.liffe_trds_.trd_qty_ * next_event_.data_.liffe_trds_.trd_px_;
              trade_info_->high_trade_price_ = ((next_event_.data_.liffe_trds_.trd_px_ > trade_info_->high_trade_price_)
                                                    ? next_event_.data_.liffe_trds_.trd_px_
                                                    : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ = ((next_event_.data_.liffe_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                   ? next_event_.data_.liffe_trds_.trd_px_
                                                   : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.liffe_trds_.trd_px_
                                                                                : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.liffe_trds_.trd_px_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.liffe_trds_.trd_px_;
                sum2_price_ += next_event_.data_.liffe_trds_.trd_px_ * next_event_.data_.liffe_trds_.trd_px_;
                periodic_traded_volume_ += next_event_.data_.liffe_trds_.trd_qty_;
                periodic_num_trades_++;
              }
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }

    } break;

    case HFSAT::kExchSourceICE: {
      std::string t_ice_filename_ =
          HFSAT::ICELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      ICE_MDS::ICECommonStruct next_event_;
      bulk_file_reader_.open(t_ice_filename_);
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ICE_MDS::ICECommonStruct));
        if (available_len_ >= sizeof(ICE_MDS::ICECommonStruct)) {  // data not found in file

          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == ICE_MDS::ICE_PL) {
            if (next_event_.data_.ice_pls_.level_ == 1 &&
                next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.ice_pls_.side_ == '1') {
                bid_sz_ = next_event_.data_.ice_pls_.size_;
              } else if (next_event_.data_.ice_pls_.side_ == '2') {
                ask_sz_ = next_event_.data_.ice_pls_.size_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          } else if (next_event_.msg_ == ICE_MDS::ICE_TRADE) {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.ice_trds_.size_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.ice_trds_.size_ * next_event_.data_.ice_trds_.price_;
              trade_info_->high_trade_price_ = ((next_event_.data_.ice_trds_.price_ > trade_info_->high_trade_price_)
                                                    ? next_event_.data_.ice_trds_.price_
                                                    : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ = ((next_event_.data_.ice_trds_.price_ < trade_info_->low_trade_price_)
                                                   ? next_event_.data_.ice_trds_.price_
                                                   : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.ice_trds_.price_
                                                                                : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.ice_trds_.price_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.ice_trds_.price_;
                sum2_price_ += next_event_.data_.ice_trds_.price_ * next_event_.data_.ice_trds_.price_;
                periodic_traded_volume_ += next_event_.data_.ice_trds_.size_;
                periodic_num_trades_++;
              }
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }

    } break;

    case HFSAT::kExchSourceEUREX: {
      std::string t_eurex_filename_;

      if (HFSAT::UseEOBIData(trading_location_file_read_, input_date_, shortcode_)) {
        t_eurex_filename_ = HFSAT::EOBIPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                                trading_location_file_read_);
      } else {
        t_eurex_filename_ =
            HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      }

      EUREX_MDS::EUREXCommonStruct next_event_;
      bulk_file_reader_.open(t_eurex_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
        if (available_len_ >= sizeof(EUREX_MDS::EUREXCommonStruct)) {  // data not found in file

          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == EUREX_MDS::EUREX_DELTA) {
            if (next_event_.data_.eurex_dels_.level_ == 1 &&
                next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.eurex_dels_.type_ == 2) {
                bid_sz_ = next_event_.data_.eurex_dels_.size_;
              } else {
                ask_sz_ = next_event_.data_.eurex_dels_.size_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          } else if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.eurex_trds_.trd_qty_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.eurex_trds_.trd_qty_ * next_event_.data_.eurex_trds_.trd_px_;
              trade_info_->high_trade_price_ = ((next_event_.data_.eurex_trds_.trd_px_ > trade_info_->high_trade_price_)
                                                    ? next_event_.data_.eurex_trds_.trd_px_
                                                    : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ = ((next_event_.data_.eurex_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                   ? next_event_.data_.eurex_trds_.trd_px_
                                                   : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.eurex_trds_.trd_px_
                                                                                : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.eurex_trds_.trd_px_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.eurex_trds_.trd_px_;
                sum2_price_ += next_event_.data_.eurex_trds_.trd_px_ * next_event_.data_.eurex_trds_.trd_px_;
                periodic_traded_volume_ += next_event_.data_.eurex_trds_.trd_qty_;
                periodic_num_trades_++;
              }
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }
    } break;
    case HFSAT::kExchSourceTMX: {
      if (input_date_ >= USING_TMX_OBF_FROM) {
        std::string tmx_pf_filename = HFSAT::CommonLoggedMessageFileNamer::GetName(
            HFSAT::kExchSourceTMX, t_exchange_symbol_, input_date_, trading_location_file_read_);

        TMX_OBF_MDS::TMXPFCommonStruct next_event_;
        bulk_file_reader_.open(tmx_pf_filename);
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_OBF_MDS::TMXPFCommonStruct));
          if (available_len_ >= sizeof(TMX_OBF_MDS::TMXPFCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
            UpdateDatainFrame(next_event_.time_.tv_sec);

            if (next_event_.msg_ == TMX_OBF_MDS::TMXPriceFeedmsgType::TMX_PF_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += next_event_.data_.trade_.quantity_;
                trade_info_->num_trades_++;
                trade_info_->sum_sz_wt_trade_price_ +=
                    next_event_.data_.trade_.quantity_ * next_event_.data_.trade_.price_;
                trade_info_->high_trade_price_ = ((next_event_.data_.trade_.price_ > trade_info_->high_trade_price_)
                                                      ? next_event_.data_.trade_.price_
                                                      : trade_info_->high_trade_price_);
                trade_info_->low_trade_price_ =
                    ((next_event_.data_.trade_.price_ < trade_info_->low_trade_price_) ? next_event_.data_.trade_.price_
                                                                                       : trade_info_->low_trade_price_);
                trade_info_->open_trade_price_ =
                    ((trade_info_->num_trades_ == 1) ? next_event_.data_.trade_.price_ : trade_info_->low_trade_price_);
                trade_info_->close_trade_price_ = next_event_.data_.trade_.price_;

                if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data_.trade_.price_;
                  sum2_price_ += next_event_.data_.trade_.price_ * next_event_.data_.trade_.price_;
                  periodic_traded_volume_ += next_event_.data_.trade_.quantity_;
                  periodic_num_trades_++;
                }
              }
            } else if (next_event_.msg_ == TMX_OBF_MDS::TMXPriceFeedmsgType::TMX_PF_DELTA) {
              if (next_event_.data_.delta_.level_ == 1 &&
                  next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (next_event_.data_.delta_.side_ == 'B') {
                  bid_sz_ = next_event_.data_.delta_.quantity_;
                } else {
                  ask_sz_ = next_event_.data_.delta_.quantity_;
                }
                trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                trade_info_->l1_event_count_++;
              }
            }
          } else {
            FlushLeftOverData(next_event_.time_.tv_sec);
            break;
          }
        }
      } else {
        std::string t_tmx_filename_ =
            HFSAT::TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

        TMX_MDS::TMXCommonStruct next_event_;
        bulk_file_reader_.open(t_tmx_filename_);

        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
          if (available_len_ >= sizeof(TMX_MDS::TMXCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
            UpdateDatainFrame(next_event_.time_.tv_sec);

            if (next_event_.msg_ == TMX_MDS::TMX_BOOK) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (next_event_.data_.tmx_books_.bid_szs_[0] > 0 && next_event_.data_.tmx_books_.num_bid_ords_[0]) {
                  bid_sz_ = next_event_.data_.tmx_books_.bid_szs_[0];
                } else if (next_event_.data_.tmx_books_.ask_szs_[0] > 0 &&
                           next_event_.data_.tmx_books_.num_ask_ords_[0]) {
                  ask_sz_ = next_event_.data_.tmx_books_.ask_szs_[0];
                }
                trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                trade_info_->l1_event_count_++;
              }
            } else if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += next_event_.data_.tmx_trds_.trd_qty_;
                trade_info_->num_trades_++;
                trade_info_->sum_sz_wt_trade_price_ +=
                    next_event_.data_.tmx_trds_.trd_qty_ * next_event_.data_.tmx_trds_.trd_px_;
                trade_info_->high_trade_price_ = ((next_event_.data_.tmx_trds_.trd_px_ > trade_info_->high_trade_price_)
                                                      ? next_event_.data_.tmx_trds_.trd_px_
                                                      : trade_info_->high_trade_price_);
                trade_info_->low_trade_price_ = ((next_event_.data_.tmx_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                     ? next_event_.data_.tmx_trds_.trd_px_
                                                     : trade_info_->low_trade_price_);
                trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.tmx_trds_.trd_px_
                                                                                  : trade_info_->low_trade_price_);
                trade_info_->close_trade_price_ = next_event_.data_.tmx_trds_.trd_px_;

                if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data_.tmx_trds_.trd_px_;
                  sum2_price_ += next_event_.data_.tmx_trds_.trd_px_ * next_event_.data_.tmx_trds_.trd_px_;
                  periodic_traded_volume_ += next_event_.data_.tmx_trds_.trd_qty_;
                  periodic_num_trades_++;
                }
              }
            }
          } else {
            FlushLeftOverData(next_event_.time_.tv_sec);
            break;
          }
        }
      }
    } break;
    case HFSAT::kExchSourceBMFEQ:
    case HFSAT::kExchSourceBMF: {
      std::string t_ntp_filename_ = HFSAT::NTPLoggedMessageFileNamer::GetName(
          t_exchange_symbol_, input_date_, trading_location_file_read_, false, is_bmf_equity_);

      NTP_MDS::NTPCommonStruct next_event_;
      bulk_file_reader_.open(t_ntp_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
        if (available_len_ >= sizeof(NTP_MDS::NTPCommonStruct)) {
          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == NTP_MDS::NTP_DELTA) {
            // There is no NTP_DELTA messages from my experience
            // just record level= 1 message as l1_event
            if (next_event_.data_.ntp_dels_.level_ == 1 &&
                next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.ntp_dels_.type_ == 0) {
                bid_sz_ = next_event_.data_.ntp_dels_.size_;
              } else {
                ask_sz_ = next_event_.data_.ntp_dels_.size_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          } else if (next_event_.msg_ == NTP_MDS::NTP_TRADE && next_event_.data_.ntp_trds_.flags_[0] != 'X') {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.ntp_trds_.trd_qty_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.ntp_trds_.trd_qty_ * next_event_.data_.ntp_trds_.trd_px_;
              trade_info_->high_trade_price_ = ((next_event_.data_.ntp_trds_.trd_px_ > trade_info_->high_trade_price_)
                                                    ? next_event_.data_.ntp_trds_.trd_px_
                                                    : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ = ((next_event_.data_.ntp_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                   ? next_event_.data_.ntp_trds_.trd_px_
                                                   : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.ntp_trds_.trd_px_
                                                                                : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.ntp_trds_.trd_px_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.ntp_trds_.trd_px_;
                sum2_price_ += next_event_.data_.ntp_trds_.trd_px_ * next_event_.data_.ntp_trds_.trd_px_;
                periodic_traded_volume_ += next_event_.data_.ntp_trds_.trd_qty_;
                periodic_num_trades_++;
              }
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }
    } break;
    case HFSAT::kExchSourceHONGKONG: {
      std::string t_hk_filename_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      HKEX_MDS::HKEXCommonStruct next_event_;
      bulk_file_reader_.open(t_hk_filename_);
      if (!bulk_file_reader_.is_open()) {
        trade_info_->sum_l1_szs_ = -1;
        trade_info_->l1_event_count_ = -1;
        trade_info_->traded_volume_ = -1;
        trade_info_->num_trades_ = -1;
      } else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
          if (available_len_ >= sizeof(HKEX_MDS::HKEXCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
            UpdateDatainFrame(next_event_.time_.tv_sec);

            if (next_event_.msg_ == HKEX_MDS::HKEX_BOOK) {
              trade_info_->sum_l1_szs_ += next_event_.data_.hkex_books_.demand_[0];
              trade_info_->l1_event_count_++;
            } else if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += next_event_.data_.hkex_trds_.trd_qty_;
                trade_info_->num_trades_++;
                trade_info_->sum_sz_wt_trade_price_ +=
                    next_event_.data_.hkex_trds_.trd_qty_ * next_event_.data_.hkex_trds_.trd_px_;
                trade_info_->high_trade_price_ =
                    ((next_event_.data_.hkex_trds_.trd_px_ > trade_info_->high_trade_price_)
                         ? next_event_.data_.hkex_trds_.trd_px_
                         : trade_info_->high_trade_price_);
                trade_info_->low_trade_price_ = ((next_event_.data_.hkex_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                     ? next_event_.data_.hkex_trds_.trd_px_
                                                     : trade_info_->low_trade_price_);
                trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.hkex_trds_.trd_px_
                                                                                  : trade_info_->low_trade_price_);
                trade_info_->close_trade_price_ = next_event_.data_.hkex_trds_.trd_px_;

                if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data_.hkex_trds_.trd_px_;
                  sum2_price_ += next_event_.data_.hkex_trds_.trd_px_ * next_event_.data_.hkex_trds_.trd_px_;
                  periodic_traded_volume_ += next_event_.data_.hkex_trds_.trd_qty_;
                  periodic_num_trades_++;
                }
              }
            }
          } else {
            FlushLeftOverData(next_event_.time_.tv_sec);
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceHKOMDCPF: {
      std::string t_hk_filename_ = "";
      if (is_hk_equity_) {
        t_hk_filename_ = HFSAT::HKStocksPFLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                          trading_location_file_read_);
      } else {
        t_hk_filename_ = HFSAT::HKOMDCPFLoggedMessagefileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                        trading_location_file_read_);
      }
      HKOMD_MDS::HKOMDPFCommonStruct next_event_;
      bulk_file_reader_.open(t_hk_filename_);
      if (!bulk_file_reader_.is_open()) {
        trade_info_->sum_l1_szs_ = -1;
        trade_info_->l1_event_count_ = -1;
        trade_info_->traded_volume_ = -1;
        trade_info_->num_trades_ = -1;
      } else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
          if (available_len_ >= sizeof(HKOMD_MDS::HKOMDPFCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
            UpdateDatainFrame(next_event_.time_.tv_sec);

            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_DELTA) {
                // TODO : the below computation is wrong as no level_ info is there in file to conclude an l1 event
                //                            trade_info_->sum_l1_szs_ += next_event_.data_.delta_.num_orders_;
                //                            trade_info_->l1_event_count_++;
                trade_info_->sum_l1_szs_ = -1;
                trade_info_->l1_event_count_ = -1;
              } else if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_TRADE) {
                trade_info_->traded_volume_ += next_event_.data_.trade_.quantity_;
                trade_info_->num_trades_++;
                trade_info_->sum_sz_wt_trade_price_ +=
                    next_event_.data_.trade_.quantity_ * next_event_.data_.trade_.price_;
                trade_info_->high_trade_price_ = ((next_event_.data_.trade_.price_ > trade_info_->high_trade_price_)
                                                      ? next_event_.data_.trade_.price_
                                                      : trade_info_->high_trade_price_);
                trade_info_->low_trade_price_ =
                    ((next_event_.data_.trade_.price_ < trade_info_->low_trade_price_) ? next_event_.data_.trade_.price_
                                                                                       : trade_info_->low_trade_price_);
                trade_info_->open_trade_price_ =
                    ((trade_info_->num_trades_ == 1) ? next_event_.data_.trade_.price_ : trade_info_->low_trade_price_);
                trade_info_->close_trade_price_ = next_event_.data_.trade_.price_;

                if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data_.trade_.price_;
                  sum2_price_ += next_event_.data_.trade_.price_ * next_event_.data_.trade_.price_;
                  periodic_traded_volume_ += next_event_.data_.trade_.quantity_;
                  periodic_num_trades_++;
                }
              }
            }
          } else {
            FlushLeftOverData(next_event_.time_.tv_sec);
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceJPY: {
      if (input_date_ < USING_OSE_ITCH_FROM) {
        // Using OSEPriceFeed for volume and trades info and OSEL1 for l1events info.
        std::string t_ose_full_filename_ = HFSAT::OSEPriceFeedLoggedMessageFileNamer::GetName(
            t_exchange_symbol_, input_date_, trading_location_file_read_);

        OSE_MDS::OSEPriceFeedCommonStruct next_event_;
        bulk_file_reader_.open(t_ose_full_filename_);

        if (!bulk_file_reader_.is_open()) {
          trade_info_->traded_volume_ = -1;
          trade_info_->num_trades_ = -1;
        } else
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
            if (available_len_ >= sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) {  // data not found in file

              HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
              UpdateDatainFrame(next_event_.time_.tv_sec);

              if (next_event_.type_ == 2) {
                if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                    next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                  trade_info_->traded_volume_ += next_event_.size;
                  trade_info_->num_trades_++;
                  trade_info_->sum_sz_wt_trade_price_ += next_event_.size * next_event_.price;
                  trade_info_->high_trade_price_ =
                      ((next_event_.price > trade_info_->high_trade_price_) ? next_event_.price
                                                                            : trade_info_->high_trade_price_);
                  trade_info_->low_trade_price_ =
                      ((next_event_.price < trade_info_->low_trade_price_) ? next_event_.price
                                                                           : trade_info_->low_trade_price_);
                  trade_info_->open_trade_price_ =
                      ((trade_info_->num_trades_ == 1) ? next_event_.price : trade_info_->low_trade_price_);
                  trade_info_->close_trade_price_ = next_event_.price;

                  if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                    sum_price_ += next_event_.price;
                    sum2_price_ += next_event_.price * next_event_.price;
                    periodic_traded_volume_ += next_event_.size;
                    periodic_num_trades_++;
                  }
                }
              }
            } else {
              FlushLeftOverData(next_event_.time_.tv_sec);
              break;
            }
          }

        HFSAT::BulkFileReader l1_feed_reader;
        std::string t_ose_l1_filename_ =
            HFSAT::OSEL1LoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
        OSE_MDS::OSEPLCommonStruct next_event_l1_;
        l1_feed_reader.open(t_ose_l1_filename_);
        if (!l1_feed_reader.is_open()) {
          trade_info_->sum_l1_szs_ = -1;
          trade_info_->l1_event_count_ = -1;

        } else
          while (true) {
            size_t available_len_ = l1_feed_reader.read(&next_event_l1_, sizeof(OSE_MDS::OSEPLCommonStruct));
            if (available_len_ >= sizeof(OSE_MDS::OSEPLCommonStruct)) {
              if (next_event_l1_.get_buy_sell_trade() != OSE_MDS::kL1TRADE) {
                trade_info_->sum_l1_szs_ += next_event_l1_.size;
                trade_info_->l1_event_count_++;
              }

            } else
              break;
          }
      } else {
        std::string ose_pf_filename = HFSAT::CommonLoggedMessageFileNamer::GetName(
            HFSAT::kExchSourceJPY, t_exchange_symbol_, input_date_, trading_location_file_read_);

        OSE_ITCH_MDS::OSEPFCommonStruct next_event_;
        bulk_file_reader_.open(ose_pf_filename);
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_ITCH_MDS::OSEPFCommonStruct));
          if (available_len_ >= sizeof(OSE_ITCH_MDS::OSEPFCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
            UpdateDatainFrame(next_event_.time_.tv_sec);

            if (next_event_.msg_ == OSE_ITCH_MDS::OSE_PF_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += next_event_.data_.trade_.quantity_;
                trade_info_->num_trades_++;
                trade_info_->sum_sz_wt_trade_price_ +=
                    next_event_.data_.trade_.quantity_ * next_event_.data_.trade_.price_;
                trade_info_->high_trade_price_ = ((next_event_.data_.trade_.price_ > trade_info_->high_trade_price_)
                                                      ? next_event_.data_.trade_.price_
                                                      : trade_info_->high_trade_price_);
                trade_info_->low_trade_price_ =
                    ((next_event_.data_.trade_.price_ < trade_info_->low_trade_price_) ? next_event_.data_.trade_.price_
                                                                                       : trade_info_->low_trade_price_);
                trade_info_->open_trade_price_ =
                    ((trade_info_->num_trades_ == 1) ? next_event_.data_.trade_.price_ : trade_info_->low_trade_price_);
                trade_info_->close_trade_price_ = next_event_.data_.trade_.price_;

                if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data_.trade_.price_;
                  sum2_price_ += next_event_.data_.trade_.price_ * next_event_.data_.trade_.price_;
                  periodic_traded_volume_ += next_event_.data_.trade_.quantity_;
                  periodic_num_trades_++;
                }
              }
            } else if (next_event_.msg_ == OSE_ITCH_MDS::OSE_PF_DELTA) {
              if (next_event_.data_.delta_.level_ == 1 &&
                  next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (next_event_.data_.delta_.side_ == 'B') {
                  bid_sz_ = next_event_.data_.delta_.quantity_;
                } else {
                  ask_sz_ = next_event_.data_.delta_.quantity_;
                }
                trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                trade_info_->l1_event_count_++;
              }
            }
          } else {
            FlushLeftOverData(next_event_.time_.tv_sec);
            break;
          }
        }
      }
    } break;
    case HFSAT::kExchSourceRTS: {
      std::string t_rts_filename_ =
          HFSAT::RTSLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      RTS_MDS::RTSCommonStruct next_event_;
      bulk_file_reader_.open(t_rts_filename_);

      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(RTS_MDS::RTSCommonStruct));
        if (available_len_ >= sizeof(RTS_MDS::RTSCommonStruct)) {  // data not found in file

          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == RTS_MDS::RTS_DELTA) {
            if (next_event_.data_.rts_dels_.level_ == 1 &&
                next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.rts_dels_.type_ == '0') {
                bid_sz_ = next_event_.data_.rts_dels_.size_;
              } else {
                ask_sz_ = next_event_.data_.rts_dels_.size_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          } else if (next_event_.msg_ == RTS_MDS::RTS_TRADE) {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.rts_trds_.trd_qty_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.rts_trds_.trd_qty_ * next_event_.data_.rts_trds_.trd_px_;
              trade_info_->high_trade_price_ = ((next_event_.data_.rts_trds_.trd_px_ > trade_info_->high_trade_price_)
                                                    ? next_event_.data_.rts_trds_.trd_px_
                                                    : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ = ((next_event_.data_.rts_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                   ? next_event_.data_.rts_trds_.trd_px_
                                                   : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.rts_trds_.trd_px_
                                                                                : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.rts_trds_.trd_px_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.rts_trds_.trd_px_;
                sum2_price_ += next_event_.data_.rts_trds_.trd_px_ * next_event_.data_.rts_trds_.trd_px_;
                periodic_traded_volume_ += next_event_.data_.rts_trds_.trd_qty_;
                periodic_num_trades_++;
              }
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }
    } break;
    case HFSAT::kExchSourceASX: {
      std::string t_asx_filename_ = HFSAT::CommonLoggedMessageFileNamer::GetName(
          HFSAT::kExchSourceASX, t_exchange_symbol_, input_date_, trading_location_file_read_);
      ASX_MDS::ASXPFCommonStruct next_event_;
      bulk_file_reader_.open(t_asx_filename_);
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(ASX_MDS::ASXPFCommonStruct));
        if (available_len_ >= sizeof(ASX_MDS::ASXPFCommonStruct)) {  // data not found in file

          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == ASX_MDS::ASX_PF_TRADE) {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.trade_.quantity_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.trade_.quantity_ * next_event_.data_.trade_.price_;
              trade_info_->high_trade_price_ =
                  ((next_event_.data_.trade_.price_ > trade_info_->high_trade_price_) ? next_event_.data_.trade_.price_
                                                                                      : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ =
                  ((next_event_.data_.trade_.price_ < trade_info_->low_trade_price_) ? next_event_.data_.trade_.price_
                                                                                     : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ =
                  ((trade_info_->num_trades_ == 1) ? next_event_.data_.trade_.price_ : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.trade_.price_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.trade_.price_;
                sum2_price_ += next_event_.data_.trade_.price_ * next_event_.data_.trade_.price_;
                periodic_traded_volume_ += next_event_.data_.trade_.quantity_;
                periodic_num_trades_++;
              }
            }
          } else if (next_event_.msg_ == ASX_MDS::ASX_PF_DELTA) {
            if (next_event_.data_.delta_.level_ == 1 && next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.delta_.side_ == 'B') {
                bid_sz_ = next_event_.data_.delta_.quantity_;
              } else {
                ask_sz_ = next_event_.data_.delta_.quantity_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }

    } break;
    case HFSAT::kExchSourceSGX: {
      std::string t_sgx_filename_ = HFSAT::CommonLoggedMessageFileNamer::GetName(
          HFSAT::kExchSourceSGX, t_exchange_symbol_, input_date_, trading_location_file_read_);
      SGX_MDS::SGXPFCommonStruct next_event_;
      bulk_file_reader_.open(t_sgx_filename_);
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(SGX_MDS::SGXPFCommonStruct));
        if (available_len_ >= sizeof(SGX_MDS::SGXPFCommonStruct)) {  // data not found in file

          HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
          UpdateDatainFrame(next_event_.time_.tv_sec);

          if (next_event_.msg_ == SGX_MDS::SGX_PF_TRADE) {
            if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              trade_info_->traded_volume_ += next_event_.data_.trade_.quantity_;
              trade_info_->num_trades_++;
              trade_info_->sum_sz_wt_trade_price_ +=
                  next_event_.data_.trade_.quantity_ * next_event_.data_.trade_.price_;
              trade_info_->high_trade_price_ =
                  ((next_event_.data_.trade_.price_ > trade_info_->high_trade_price_) ? next_event_.data_.trade_.price_
                                                                                      : trade_info_->high_trade_price_);
              trade_info_->low_trade_price_ =
                  ((next_event_.data_.trade_.price_ < trade_info_->low_trade_price_) ? next_event_.data_.trade_.price_
                                                                                     : trade_info_->low_trade_price_);
              trade_info_->open_trade_price_ =
                  ((trade_info_->num_trades_ == 1) ? next_event_.data_.trade_.price_ : trade_info_->low_trade_price_);
              trade_info_->close_trade_price_ = next_event_.data_.trade_.price_;

              if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                sum_price_ += next_event_.data_.trade_.price_;
                sum2_price_ += next_event_.data_.trade_.price_ * next_event_.data_.trade_.price_;
                periodic_traded_volume_ += next_event_.data_.trade_.quantity_;
                periodic_num_trades_++;
              }
            }
          } else if (next_event_.msg_ == SGX_MDS::SGX_PF_DELTA) {
            if (next_event_.data_.delta_.level_ == 1 && next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
              if (next_event_.data_.delta_.side_ == 'B') {
                bid_sz_ = next_event_.data_.delta_.quantity_;
              } else {
                ask_sz_ = next_event_.data_.delta_.quantity_;
              }
              trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
              trade_info_->l1_event_count_++;
            }
          }
        } else {
          FlushLeftOverData(next_event_.time_.tv_sec);
          break;
        }
      }
    } break;
    case HFSAT::kExchSourceMICEX:
    case HFSAT::kExchSourceMICEX_EQ:
    case HFSAT::kExchSourceMICEX_CR: {
      std::string t_micex_filename_ =
          HFSAT::MICEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      MICEX_MDS::MICEXCommonStruct next_event_;
      bulk_file_reader_.open(t_micex_filename_);

      int lot_size_ = HFSAT::SecurityDefinitions::GetContractMinOrderSize(shortcode_, input_date_);

      if (!bulk_file_reader_.is_open()) {
        trade_info_->sum_l1_szs_ = -1;
        trade_info_->l1_event_count_ = -1;
        trade_info_->traded_volume_ = -1;
        trade_info_->num_trades_ = -1;
      } else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(MICEX_MDS::MICEXCommonStruct));
          if (available_len_ >= sizeof(MICEX_MDS::MICEXCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
            UpdateDatainFrame(next_event_.time_.tv_sec);

            if (next_event_.msg_ == MICEX_MDS::MICEX_DELTA) {
              if (next_event_.data_.micex_dels_.level_ == 1 &&
                  next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (next_event_.data_.micex_dels_.type_ == '0') {
                  bid_sz_ = next_event_.data_.micex_dels_.size_;
                } else if (next_event_.data_.micex_dels_.type_ == '1') {
                  ask_sz_ = next_event_.data_.micex_dels_.size_;
                }
                trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                trade_info_->l1_event_count_++;
              }
            }

            if (next_event_.msg_ == MICEX_MDS::MICEX_TRADE) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += (next_event_.data_.micex_trds_.trd_qty_ * lot_size_);
                trade_info_->num_trades_++;
                trade_info_->sum_sz_wt_trade_price_ +=
                    next_event_.data_.micex_trds_.trd_qty_ * next_event_.data_.micex_trds_.trd_px_;
                trade_info_->high_trade_price_ =
                    ((next_event_.data_.micex_trds_.trd_px_ > trade_info_->high_trade_price_)
                         ? next_event_.data_.micex_trds_.trd_px_
                         : trade_info_->high_trade_price_);
                trade_info_->low_trade_price_ = ((next_event_.data_.micex_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                     ? next_event_.data_.micex_trds_.trd_px_
                                                     : trade_info_->low_trade_price_);
                trade_info_->open_trade_price_ =
                    ((trade_info_->num_trades_ == 1) ? next_event_.data_.micex_trds_.trd_px_
                                                     : trade_info_->low_trade_price_);
                trade_info_->close_trade_price_ = next_event_.data_.micex_trds_.trd_px_;

                if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data_.micex_trds_.trd_px_;
                  sum2_price_ += next_event_.data_.micex_trds_.trd_px_ * next_event_.data_.micex_trds_.trd_px_;
                  periodic_traded_volume_ += next_event_.data_.micex_trds_.trd_qty_;
                  periodic_num_trades_++;
                }
              }
            }
          } else {
            FlushLeftOverData(next_event_.time_.tv_sec);
            break;
          }
        }
    } break;
    case HFSAT::kExchSourceCFE: {
      std::string t_cfe_filename_ =
          HFSAT::CFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

      CSM_MDS::CSMCommonStruct next_event_;
      bulk_file_reader_.open(t_cfe_filename_);

      if (!bulk_file_reader_.is_open()) {
        trade_info_->sum_l1_szs_ = -1;
        trade_info_->l1_event_count_ = -1;
        trade_info_->traded_volume_ = -1;
        trade_info_->num_trades_ = -1;
      } else
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
          if (available_len_ >= sizeof(CSM_MDS::CSMCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time_.tv_sec, next_event_.time_.tv_usec);
            UpdateDatainFrame(next_event_.time_.tv_sec);

            if (next_event_.msg_ == CSM_MDS::CSM_TRADE &&
                (shortcode_.substr(0, 3) == "SP_" || next_event_.data_.csm_trds_.trade_condition[0] != 'S')) {
              if (next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += (next_event_.data_.csm_trds_.trd_qty_);
                trade_info_->num_trades_++;
                trade_info_->sum_sz_wt_trade_price_ +=
                    next_event_.data_.csm_trds_.trd_qty_ * next_event_.data_.csm_trds_.trd_px_;
                trade_info_->high_trade_price_ = ((next_event_.data_.csm_trds_.trd_px_ > trade_info_->high_trade_price_)
                                                      ? next_event_.data_.csm_trds_.trd_px_
                                                      : trade_info_->high_trade_price_);
                trade_info_->low_trade_price_ = ((next_event_.data_.csm_trds_.trd_px_ < trade_info_->low_trade_price_)
                                                     ? next_event_.data_.csm_trds_.trd_px_
                                                     : trade_info_->low_trade_price_);
                trade_info_->open_trade_price_ = ((trade_info_->num_trades_ == 1) ? next_event_.data_.csm_trds_.trd_px_
                                                                                  : trade_info_->low_trade_price_);
                trade_info_->close_trade_price_ = next_event_.data_.csm_trds_.trd_px_;

                if (next_event_.time_.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data_.csm_trds_.trd_px_;
                  sum2_price_ += next_event_.data_.csm_trds_.trd_px_ * next_event_.data_.csm_trds_.trd_px_;
                  periodic_traded_volume_ += next_event_.data_.csm_trds_.trd_qty_;
                  periodic_num_trades_++;
                }
              }
            } else if (next_event_.msg_ == CSM_MDS::CSM_DELTA || next_event_.msg_ == CSM_MDS::CSM_TOB) {
              if (next_event_.data_.csm_dels_.level_ == 1 &&
                  next_event_.time_.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time_.tv_sec % 86400 < end_secs_from_midnight_) {
                if (next_event_.data_.csm_dels_.type_ == '0') {
                  bid_sz_ = next_event_.data_.csm_dels_.size_[0];
                } else if (next_event_.data_.csm_dels_.type_ == '1') {
                  ask_sz_ = next_event_.data_.csm_dels_.size_[0];
                }
                trade_info_->sum_l1_szs_ += (bid_sz_ + ask_sz_) / 2;
                trade_info_->l1_event_count_++;
              }
            }
          } else {
            FlushLeftOverData(next_event_.time_.tv_sec);
            break;
          }
        }
    } break;

    case HFSAT::kExchSourceNSE: {
      if (is_l1_mode_) {
        std::string filename_ =
            HFSAT::NSEL1LoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

        ::HFSAT::GenericL1DataStruct next_event_;
        bulk_file_reader_.open(filename_);

        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(::HFSAT::GenericL1DataStruct));
          if (available_len_ >= sizeof(::HFSAT::GenericL1DataStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.time.tv_sec, next_event_.time.tv_usec);
            UpdateDatainFrame(next_event_.time.tv_sec);

            if (next_event_.type == ::HFSAT::GenericL1DataType::L1_TRADE) {
              if (next_event_.time.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.time.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += next_event_.trade.size;
                trade_info_->num_trades_++;

                if (next_event_.time.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.trade.price;
                  sum2_price_ += next_event_.trade.price * next_event_.trade.price;
                  periodic_traded_volume_ += next_event_.trade.size;
                  periodic_num_trades_++;
                }
              }
            } else if (next_event_.type == ::HFSAT::GenericL1DataType::L1_DELTA) {
              // Probably no level info
            }
          } else {
            FlushLeftOverData(next_event_.time.tv_sec);
            break;
          }
        }
      } else {
        std::string t_nse_filename_ =
            HFSAT::NSELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);

        NSE_MDS::NSEDotexOfflineCommonStruct next_event_;
        bulk_file_reader_.open(t_nse_filename_);

        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
          if (available_len_ >= sizeof(NSE_MDS::NSEDotexOfflineCommonStruct)) {  // data not found in file

            HandleFirstEvent(next_event_.source_time.tv_sec, next_event_.source_time.tv_usec);
            UpdateDatainFrame(next_event_.source_time.tv_sec);

            if (next_event_.msg_type == NSE_MDS::MsgType::kNSETrade) {
              if (next_event_.source_time.tv_sec % 86400 > begin_secs_from_midnight_ &&
                  next_event_.source_time.tv_sec % 86400 < end_secs_from_midnight_) {
                trade_info_->traded_volume_ += next_event_.data.nse_dotex_trade.trade_quantity;
                trade_info_->num_trades_++;

                if (next_event_.source_time.tv_sec < next_delta_time_event.tv_sec) {
                  sum_price_ += next_event_.data.nse_dotex_trade.trade_price;
                  sum2_price_ +=
                      next_event_.data.nse_dotex_trade.trade_price * next_event_.data.nse_dotex_trade.trade_price;
                  periodic_traded_volume_ += next_event_.data.nse_dotex_trade.trade_quantity;
                  periodic_num_trades_++;
                }
              }
            } else if (next_event_.msg_type == NSE_MDS::MsgType::kNSEOrderDelta) {
              // Probably no level info
            }
          } else {
            FlushLeftOverData(next_event_.source_time.tv_sec);
            break;
          }
        }
      }

    } break;

    default: { } break; }
}
