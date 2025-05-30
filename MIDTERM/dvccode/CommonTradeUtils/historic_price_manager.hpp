/**
   \file dvccode/CommonTradeUtils/historic_price_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_COMMONTRADEUTILS_HISTORIC_PRICE_MANAGER_H
#define BASE_COMMONTRADEUTILS_HISTORIC_PRICE_MANAGER_H

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#define HIST_PRICE_DIR "/spare/local/HistoricPrices/"
#define SHORTCODE_LIST_FILENAME "shortcode_list.txt"
#define LAST_PRICE_BASENAME "last_prices."

namespace HFSAT {

class HistoricPriceManager {
 public:
  static void SetUniqueInstance(DebugLogger *_p_dbglogger_, SecurityNameIndexer *_p_sec_name_indexer_,
                                const int _trading_date_) {
    HistoricPriceManager *p_historic_price_manager_ = GetUniqueInstance();

    p_historic_price_manager_->loadHistoricalPrices(_p_dbglogger_, _p_sec_name_indexer_, _trading_date_);
  }

  static HistoricPriceManager *GetUniqueInstance() {
    static HistoricPriceManager *p_unique_instance_ = NULL;

    if (p_unique_instance_ == NULL) {
      p_unique_instance_ = new HistoricPriceManager();
    }

    return p_unique_instance_;
  }

  ~HistoricPriceManager() {
    shortcode_to_last_price_info_.clear();  // Unnecessary
  }

  inline double GetPriceFromType(const PriceType_t t_price_type_, const unsigned int t_security_id_) const {
    if (!p_sec_name_indexer_ || t_security_id_ >= p_sec_name_indexer_->NumSecurityId()) {
      return 0.0;
    }

    DebugLogger &dbglogger_ = *p_dbglogger_;
    std::map<std::string, PriceInfo>::const_iterator _itr_ =
        shortcode_to_last_price_info_.find(p_sec_name_indexer_->GetShortcodeFromId(t_security_id_));

    if (_itr_ == shortcode_to_last_price_info_.end()) {
      if (dbglogger_.CheckLoggingLevel(HPM_INFO)) {
        DBGLOG_CLASS_FUNC_LINE << "No price info. for " << p_sec_name_indexer_->GetShortcodeFromId(t_security_id_)
                               << DBGLOG_ENDL_FLUSH;
      }

      return 0.0;
    }

    double t_price_ = 0.0;

    switch (t_price_type_) {
      case kPriceTypeMidprice:
        t_price_ = _itr_->second.mid_price_;
        break;
      case kPriceTypeMktSizeWPrice:
        t_price_ = _itr_->second.mkt_size_w_price_;
        break;
      case kPriceTypeMktSinusoidal:
        t_price_ = _itr_->second.mkt_sinusoidal_;
        break;
      case kPriceTypeOrderWPrice:
        t_price_ = _itr_->second.order_w_price_;
        break;
      case kPriceTypeOfflineMixMMS:
        t_price_ = _itr_->second.offline_mix_mms_;
        break;
      default:
        t_price_ = _itr_->second.mid_price_;
        break;
    }

    if (dbglogger_.CheckLoggingLevel(HPM_INFO)) {
      DBGLOG_CLASS_FUNC_LINE << p_sec_name_indexer_->GetShortcodeFromId(t_security_id_) << " "
                             << PriceType_t_To_String(t_price_type_) << " : " << t_price_ << DBGLOG_ENDL_FLUSH;
    }

    return t_price_;
  }

 private:
  HistoricPriceManager()
      : p_dbglogger_(NULL), p_sec_name_indexer_(NULL), trading_date_(0), shortcode_to_last_price_info_() {}

  void loadHistoricalPrices(DebugLogger *_p_dbglogger_, SecurityNameIndexer *_p_sec_name_indexer_,
                            const int _trading_date_) {
    p_dbglogger_ = _p_dbglogger_;
    p_sec_name_indexer_ = _p_sec_name_indexer_;
    trading_date_ = _trading_date_;

    DebugLogger &dbglogger_ = *p_dbglogger_;

    std::ifstream shortcode_list_ifs_;
    {  // Get the list of shortcodes
      std::ostringstream t_oss_;
      t_oss_ << HIST_PRICE_DIR << SHORTCODE_LIST_FILENAME;
      shortcode_list_ifs_.open(t_oss_.str().c_str(), std::ifstream::in);

      if (!shortcode_list_ifs_.is_open()) {
        if (dbglogger_.CheckLoggingLevel(HPM_ERROR)) {
          DBGLOG_CLASS_FUNC_LINE << "Failed to open " << t_oss_.str() << DBGLOG_ENDL_FLUSH;
        }

        return;
      }
    }

    char shortcode_line_[64];
    while (shortcode_list_ifs_.getline(shortcode_line_, sizeof(shortcode_line_))) {
      const char *p_shortcode_ = strtok(shortcode_line_, " \t");

      {  // Open the last price file corresponding to this shortcode and try to load PriceInfo.
        std::ifstream last_price_ifs_;
        {
          std::ostringstream t_oss_;
          t_oss_ << HIST_PRICE_DIR << LAST_PRICE_BASENAME << p_shortcode_;

          last_price_ifs_.open(t_oss_.str().c_str(), std::ifstream::in);

          if (!last_price_ifs_.is_open()) {
            if (dbglogger_.CheckLoggingLevel(HPM_ERROR)) {
              DBGLOG_CLASS_FUNC_LINE << "PriceInfo unavailable for " << p_shortcode_ << DBGLOG_ENDL_FLUSH;
            }

            continue;  // Process next shortcode in list.
          }
        }

        std::map<int, PriceInfo> date_to_price_info_;
        {  // Read in [DATE -> PriceInfo]
          char price_info_line_[1024];

          while (last_price_ifs_.getline(price_info_line_, sizeof(price_info_line_))) {
            int _date_ = 0;
            double _bid_px_ = 0.0, _ask_px_ = 0.0;
            int _bid_sz_ = 0, _ask_sz_ = 0, _bid_ords_ = 0, _ask_ords_ = 0;

            // FORMAT:
            // Date BS BO BP AP AO AS
            sscanf(price_info_line_, "%d %d %d %lf %lf %d %d", &_date_, &_bid_sz_, &_bid_ords_, &_bid_px_, &_ask_px_,
                   &_ask_ords_, &_ask_sz_);

            const double mid_price_ = computePrice(kPriceTypeMidprice, _bid_px_, _ask_sz_, _bid_sz_, _ask_sz_,
                                                   _bid_ords_, _ask_ords_, p_shortcode_);
            const double mkt_size_w_price_ = computePrice(kPriceTypeMktSizeWPrice, _bid_px_, _ask_sz_, _bid_sz_,
                                                          _ask_sz_, _bid_ords_, _ask_ords_, p_shortcode_);
            const double mkt_sinusoidal_ = computePrice(kPriceTypeMktSinusoidal, _bid_px_, _ask_sz_, _bid_sz_, _ask_sz_,
                                                        _bid_ords_, _ask_ords_, p_shortcode_);
            const double order_w_price_ = computePrice(kPriceTypeOrderWPrice, _bid_px_, _ask_sz_, _bid_sz_, _ask_sz_,
                                                       _bid_ords_, _ask_ords_, p_shortcode_);
            const double offline_mix_mms_ = computePrice(kPriceTypeOfflineMixMMS, _bid_px_, _ask_sz_, _bid_sz_,
                                                         _ask_sz_, _bid_ords_, _ask_ords_, p_shortcode_);

            date_to_price_info_[_date_] =
                PriceInfo(mid_price_, mkt_size_w_price_, mkt_sinusoidal_, order_w_price_, offline_mix_mms_);
          }

          last_price_ifs_.close();
        }

        // Theoretically any information older than 1 day should not be used.
        // Limit of 5 is to account for long weekends, etc.
        int current_trading_date_ = DateTime::CalcPrevDay(_trading_date_);  // Start with yesterday's prices.
        int no_days_ = 0;
        for (no_days_ = 0; no_days_ < 5; ++no_days_) {
          if (date_to_price_info_.find(current_trading_date_) != date_to_price_info_.end()) {
            if (dbglogger_.CheckLoggingLevel(HPM_INFO)) {
              DBGLOG_CLASS_FUNC_LINE << "Using " << current_trading_date_ << " closing prices for " << p_shortcode_
                                     << DBGLOG_ENDL_FLUSH;
            }

            shortcode_to_last_price_info_[p_shortcode_] =
                date_to_price_info_[current_trading_date_];  // Found a usable entry.
            break;
          }
        }

        if (no_days_ == 5) {  // Could not find a recent price entry.
          if (dbglogger_.CheckLoggingLevel(HPM_ERROR)) {
            DBGLOG_CLASS_FUNC_LINE << "Recent PriceInfo unavailable for " << p_shortcode_ << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }

    shortcode_list_ifs_.close();
  }

  double computePrice(const PriceType_t t_price_type_, const double _bid_px_, const double _ask_px_, const int _bid_sz_,
                      const int _ask_sz_, const int _bid_ords_, const int _ask_ords_, const char *t_shortcode_) {
    const FastPriceConvertor fast_price_convertor_(
        SecurityDefinitions::GetContractMinPriceIncrement(t_shortcode_, trading_date_));
    const int spread_increments_ = fast_price_convertor_.GetFastIntPx(_ask_px_ - _bid_px_);

    double mid_price_ = (_bid_px_ + _ask_px_) / 2.0;
    double mkt_size_w_price_ = 0.0;
    double mkt_sinusoidal_ = 0.0;
    double mkt_order_w_price_ = 0.0;
    double offline_mix_mms_ = 0.0;

    if (spread_increments_ <= 1) {  // if spread is 1 tick then this
      {                             // MktSizeWPrice
        mkt_size_w_price_ = (_bid_px_ * _ask_sz_ + _ask_px_ * _bid_sz_) / (_bid_sz_ + _ask_sz_);
      }

      {  // MktSinusoidal
        /// tilt ... top level book pressure ... if > 0 then bid is heavier and hence price should be towards offer
        const double size_tilt_ = (double)(_bid_sz_ - _ask_sz_) / (double)(_bid_sz_ + _ask_sz_);

        mkt_sinusoidal_ = mid_price_ + (((_ask_px_ - _bid_px_) / 2.0) * size_tilt_ * size_tilt_ * size_tilt_);
      }

      {  // MktOrderWPrice
        /// tilt ... top level book pressure ... if > 0 then bid is heavier and hence price shoudl be towards offer
        double size_tilt_ = (double)(_bid_ords_ - _ask_ords_) / (double)(_bid_ords_ + _ask_ords_);

        mkt_order_w_price_ = mid_price_ + (((_ask_px_ - _bid_px_) / 2.0) * size_tilt_);
      }

      {  // OfflineMixMMS
        offline_mix_mms_ = mid_price_;
      }
    } else {
      {  // MktSizeWPrice
        // else 50% midprice_ 50% mkt_size_weighted_price_
        mkt_size_w_price_ = ((_bid_px_ * _ask_sz_ + _ask_px_ * _bid_sz_) / (_bid_sz_ + _ask_sz_) + (mid_price_)) / 2.0;
      }

      {  // MktSinusoidal
        // else just mid_price_
        mkt_sinusoidal_ = (_bid_px_ + _ask_px_) / 2.0;
      }

      {  // MktOrderWPrice
        // else just mid_price_
        mkt_order_w_price_ = (_bid_px_ + _ask_px_) / 2.0;
      }

      {  // OfflineMixMMS
        offline_mix_mms_ = mid_price_;
      }
    }

    double t_price_ = 0.0;

    switch (t_price_type_) {
      case kPriceTypeMidprice:
        t_price_ = mid_price_;
        break;
      case kPriceTypeMktSizeWPrice:
        t_price_ = mkt_size_w_price_;
        break;
      case kPriceTypeMktSinusoidal:
        t_price_ = mkt_sinusoidal_;
        break;
      case kPriceTypeOrderWPrice:
        t_price_ = mkt_order_w_price_;
        break;
      case kPriceTypeOfflineMixMMS:
        t_price_ = offline_mix_mms_;
        break;
      default:
        t_price_ = mid_price_;
        break;
    }

    return t_price_;
  }

 private:
  DebugLogger *p_dbglogger_;
  SecurityNameIndexer *p_sec_name_indexer_;

  int trading_date_;

  struct PriceInfo {
    double mid_price_;
    double mkt_size_w_price_;
    double mkt_sinusoidal_;
    double order_w_price_;
    double offline_mix_mms_;

   public:
    PriceInfo() : mid_price_(0), mkt_size_w_price_(0), mkt_sinusoidal_(0), order_w_price_(0), offline_mix_mms_(0) {}

    PriceInfo(const double _mid_price_, const double _mkt_size_w_price_, const double _mkt_sinusoidal_,
              const double _order_w_price_, const double _offline_mix_mms_)
        : mid_price_(_mid_price_),
          mkt_size_w_price_(_mkt_size_w_price_),
          mkt_sinusoidal_(_mkt_sinusoidal_),
          order_w_price_(_order_w_price_),
          offline_mix_mms_(_offline_mix_mms_) {}
  };

  std::map<std::string, PriceInfo> shortcode_to_last_price_info_;
};
}

#endif  // BASE_COMMONTRADEUTILS_HISTORIC_PRICE_MANAGER_H
