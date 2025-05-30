#ifndef BASE_INDICATORS_SIMPLE_BOOK_H
#define BASE_INDICATORS_SIMPLE_BOOK_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvctrade/Indicators/book_info_manager.hpp"

namespace HFSAT {

class SimpleBook : public CommonIndicator {
 protected:
  SecurityMarketView& indep_market_view_;
  unsigned int num_levels_;
  double decay_factor_;
  double stdev_duration_;

  std::vector<double> decay_vector_;

  BookInfoManager& book_info_manager_;
  BookInfoManager::BookInfoStruct* book_info_struct_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SimpleBook* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SimpleBook* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                       double _decay_factor_, PriceType_t _basepx_pxtype_, double _stdev_duration_);

 protected:
  SimpleBook(DebugLogger& _debuglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
             SecurityMarketView& _indep_market_view_, unsigned int _num_levels_, double _decay_factor_,
             double _stdev_duration_);

 public:
  ~SimpleBook() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  // functions
  static std::string VarName() { return "SimpleBook"; }
};
}
#endif
