/**
   \file Tools/get_all_mds_stats_for_day.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717

 */

#include <iostream>
#include <stdlib.h>
#include "baseinfra/Tools/common_smv_source.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvctrade/Indicators/moving_avg_price_implied_vol.hpp"
#include "dvctrade/Indicators/options_greek.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/OptionsHelper/option.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

class NSEOptionsMktInfo : public HFSAT::SecurityMarketViewChangeListener, public HFSAT::IndicatorListener {
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  int date_;
  HFSAT::SecurityMarketView* opt_smv_;
  HFSAT::SecurityMarketView* underlying_smv_;

public:
  // common underlying & option
  std::vector<double> tick_size_;
  std::vector<int> lot_size_;
  double strike_price_;

  std::vector <double> midprice_avg_;
  std::vector <double> midprice_square_;

  std::vector<double> midprice_stdev_;
  std::vector<double> avg_baspread_;
  std::vector<int> l1_events_;
  std::vector<double> avg_tradesize_;
  std::vector<int> noof_trades_;
  std::vector<int> total_volume_;
  std::vector<double> avg_l1size_;
  
  std::vector<double> implied_vol_ts_;
  std::vector<double> delta_ts_;
  std::vector<double> gamma_ts_;
  std::vector<double> vega_ts_;
  std::vector<double> theta_ts_;

  HFSAT::OptionType_t option_type_;
  double interest_rate_;
  double days_to_expire_;

  HFSAT::Option* opt_obj_;
  HFSAT::MovingAvgPriceImpliedVol* impld_vol_;
  HFSAT::OptionsGreek* delta_;
  HFSAT::OptionsGreek* gamma_;
  HFSAT::OptionsGreek* vega_;
  HFSAT::OptionsGreek* theta_;


public:
  NSEOptionsMktInfo(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, int t_date_, HFSAT::SecurityMarketView* p_opt_smv_, HFSAT::SecurityMarketView* p_underlying_smv_)
    : dbglogger_(_dbglogger_), watch_(_watch_), date_(t_date_),opt_smv_(p_opt_smv_),underlying_smv_(p_underlying_smv_){
      p_underlying_smv_->subscribe_tradeprints(this);
      p_opt_smv_->subscribe_tradeprints(this);
      p_underlying_smv_->subscribe_price_type(this, HFSAT::kPriceTypeMktSizeWPrice);
      p_opt_smv_->subscribe_price_type(this, HFSAT::kPriceTypeMktSizeWPrice);
   
      tick_size_.resize(2,0.0);
      lot_size_.resize(2,0.0);
      midprice_avg_.resize(2,0.0);
      midprice_square_.resize(2,0.0);
      midprice_stdev_.resize(2,0.0);
      avg_baspread_.resize(2,0.0);
      l1_events_.resize(2,0);
      avg_tradesize_.resize(2,0.0);
      noof_trades_.resize(2,0);
      total_volume_.resize(2,0);
      avg_l1size_.resize(2,0.0);

      option_type_  = (HFSAT::OptionType_t)HFSAT::NSESecurityDefinitions::GetOptionType(opt_smv_->shortcode());
      days_to_expire_ = difftime(HFSAT::DateTime::GetTimeUTC(HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(opt_smv_->shortcode()),1000),HFSAT::DateTime::GetTimeMidnightUTC(date_))/(3600*24);
      interest_rate_ = HFSAT::NSESecurityDefinitions::GetInterestRate(date_);

      opt_obj_ = new HFSAT::Option( option_type_, strike_price_, 0.0, days_to_expire_, 0.0, interest_rate_);

      tick_size_[0] = HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(opt_smv_->shortcode(),date_);
      lot_size_[0] = HFSAT::SecurityDefinitions::GetContractMinOrderSize(opt_smv_->shortcode(),date_);
      tick_size_[1] = HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(underlying_smv_->shortcode(),date_);
      lot_size_[1] = HFSAT::SecurityDefinitions::GetContractMinOrderSize(underlying_smv_->shortcode(),date_);
      strike_price_ = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(opt_smv_->shortcode());

      impld_vol_ = HFSAT::MovingAvgPriceImpliedVol::GetUniqueInstance(dbglogger_, watch_, *opt_smv_, 100.0, HFSAT::kPriceTypeMidprice);
      impld_vol_->add_unweighted_indicator_listener(1u, this);
      delta_ = HFSAT::OptionsGreek::GetUniqueInstance(dbglogger_, watch_, *opt_smv_, 100.0, 1, HFSAT::kPriceTypeMidprice);
      delta_-> add_unweighted_indicator_listener(2u, this);

      gamma_ = HFSAT::OptionsGreek::GetUniqueInstance(dbglogger_, watch_, *opt_smv_, 100.0, 2, HFSAT::kPriceTypeMidprice);
      gamma_-> add_unweighted_indicator_listener(3u, this);

      vega_ = HFSAT::OptionsGreek::GetUniqueInstance(dbglogger_, watch_, *opt_smv_, 100.0, 3, HFSAT::kPriceTypeMidprice);
      vega_-> add_unweighted_indicator_listener(4u, this);

      theta_ = HFSAT::OptionsGreek::GetUniqueInstance(dbglogger_, watch_, *opt_smv_, 100.0, 4, HFSAT::kPriceTypeMidprice);
      theta_-> add_unweighted_indicator_listener(5u, this);
}

  
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if (_security_id_==opt_smv_->security_id()){
      avg_baspread_[0] += (_market_update_info_.bestask_price_ - _market_update_info_.bestbid_price_);
      l1_events_[0] += 1;
      avg_l1size_[0] += (_market_update_info_.bestbid_size_ + _market_update_info_.bestask_size_)/2;

      double mid_price_ = (_market_update_info_.bestask_price_ + _market_update_info_.bestbid_price_)/2;
      midprice_avg_[0] += mid_price_;
      midprice_square_[0] += mid_price_*mid_price_;

    } else {
      avg_baspread_[1] += (_market_update_info_.bestask_price_ - _market_update_info_.bestbid_price_);
      l1_events_[1] += 1;
      avg_l1size_[1] += (_market_update_info_.bestbid_size_ + _market_update_info_.bestask_size_)/2;

      double mid_price_ = (_market_update_info_.bestask_price_ + _market_update_info_.bestbid_price_)/2;
      midprice_avg_[1] += mid_price_;
      midprice_square_[1] += mid_price_*mid_price_;
    }
  }
  
  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
			   const HFSAT::MarketUpdateInfo& _market_update_info_) {
    if (_security_id_==opt_smv_->security_id()){
      total_volume_[0] += _trade_print_info_.size_traded_;
      noof_trades_[0] += 1;
      //_trade_print_info_.trade_price_;     
    } else {
      total_volume_[1] += _trade_print_info_.size_traded_;
      noof_trades_[1] += 1;
    }
  }

  void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_) {
    if ( indicator_index_ == 1u ){
      implied_vol_ts_.push_back(new_value_);
    } else if ( indicator_index_ == 2u ) {
      delta_ts_.push_back(new_value_);
    } else if ( indicator_index_ == 3u ) {
      gamma_ts_.push_back(new_value_);
    } else if ( indicator_index_ == 4u ) {
      vega_ts_.push_back(new_value_);
    } else if ( indicator_index_ == 5u ) {
      theta_ts_.push_back(new_value_);
    }
  }
  inline void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
  
};

void ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ]"
              << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);

    if (argc > 3) {
      begin_secs_from_midnight_ = (atoi(argv[3]) / 100) * 60 * 60 + (atoi(argv[3]) % 100) * 60;
    }
    if (argc > 4) {
      end_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
  }
}

int main(int argc, char** argv) {
  std::string opt_code_ = "";
  int input_date_ = 20150101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  ParseCommandLineParams(argc, (const char**)argv, opt_code_, input_date_, begin_secs_from_midnight_,
                         end_secs_from_midnight_);

std::vector<std::string> shc_list;
HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
shc_list.push_back(opt_code_);
 shc_list.push_back(HFSAT::NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(opt_code_));
CommonSMVSource* common_smv_source = new CommonSMVSource(shc_list, input_date_);
 HFSAT::Watch& watch_ = common_smv_source->getWatch();
 HFSAT::DebugLogger& dbglogger_ = common_smv_source->getLogger();
common_smv_source->Initialize();


 NSEOptionsMktInfo main_run ( dbglogger_, watch_, input_date_, HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shc_list[0]),
			      HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shc_list[1]));
  common_smv_source->Run();

  main_run.midprice_stdev_[0] = std::sqrt ( main_run.midprice_square_[0]/main_run.l1_events_[0] - main_run.midprice_avg_[0]/main_run.l1_events_[0]* main_run.midprice_avg_[0]/main_run.l1_events_[0]);
  main_run.midprice_stdev_[1] = std::sqrt ( main_run.midprice_square_[1]/main_run.l1_events_[1] - main_run.midprice_avg_[1]/main_run.l1_events_[1]* main_run.midprice_avg_[1]/main_run.l1_events_[1]);

  main_run.avg_baspread_[0] = main_run.avg_baspread_[0]/main_run.l1_events_[0];
  main_run.avg_baspread_[1] = main_run.avg_baspread_[1]/main_run.l1_events_[1];

  main_run.avg_tradesize_[0] = main_run.total_volume_[0]/main_run.noof_trades_[0];
  main_run.avg_tradesize_[1] = main_run.total_volume_[1]/main_run.noof_trades_[1];

  main_run.avg_l1size_[0] = main_run.avg_l1size_[0]/main_run.l1_events_[0];
  main_run.avg_l1size_[1] = main_run.avg_l1size_[1]/main_run.l1_events_[1];

  std::cout << "Security TickSize LotSize StrikePrice MidpriceStdev AvgBASpread L1Events AvgTradeSize NoOfTrades TotalVolume AvgL1Size \n";
  std::cout << shc_list[0] << " "
	    << main_run.tick_size_[0] << " "
	    << main_run.lot_size_[0] << " "
	    << main_run.strike_price_ << " "
	    << main_run.midprice_stdev_[0] << " "
	    << main_run.avg_baspread_[0] << " "
	    << main_run.l1_events_[0] << " "
	    << main_run.avg_tradesize_[0] << " "
	    << main_run.noof_trades_[0] << " "
	    << main_run.total_volume_[0] << " "
	    << main_run.avg_l1size_[0] << "\n";

  std::cout << shc_list[1] << " "
	    << main_run.tick_size_[1] << " "
	    << main_run.lot_size_[1] << " "
	    << main_run.strike_price_ << " "
	    << main_run.midprice_stdev_[1] << " "
	    << main_run.avg_baspread_[1] << " "
	    << main_run.l1_events_[1] << " "
	    << main_run.avg_tradesize_[1] << " "
	    << main_run.noof_trades_[1] << " "
	    << main_run.total_volume_[1] << " "
	    << main_run.avg_l1size_[1] << "\n";

  std::cout << "Greek/Param "
	    << "Mean Median Stdev Min Max\n";

  std::cout << "ImpliedVol "
    << HFSAT::VectorUtils::GetMean(main_run.implied_vol_ts_) << " "
    << HFSAT::VectorUtils::GetMedian(main_run.implied_vol_ts_) << " "
    << HFSAT::VectorUtils::GetStdev(main_run.implied_vol_ts_) << " "
    << HFSAT::VectorUtils::GetMin(main_run.implied_vol_ts_) << " "
    << HFSAT::VectorUtils::GetMax(main_run.implied_vol_ts_) << "\n";

  std::cout << "Delta "
    << HFSAT::VectorUtils::GetMean(main_run.delta_ts_) << " "
    << HFSAT::VectorUtils::GetMedian(main_run.delta_ts_) << " "
    << HFSAT::VectorUtils::GetStdev(main_run.delta_ts_) << " "
    << HFSAT::VectorUtils::GetMin(main_run.delta_ts_) << " "
    << HFSAT::VectorUtils::GetMax(main_run.delta_ts_) << "\n";

  std::cout << "Gamma "
    << HFSAT::VectorUtils::GetMean(main_run.gamma_ts_) << " "
    << HFSAT::VectorUtils::GetMedian(main_run.gamma_ts_) << " "
    << HFSAT::VectorUtils::GetStdev(main_run.gamma_ts_) << " "
    << HFSAT::VectorUtils::GetMin(main_run.gamma_ts_) << " "
    << HFSAT::VectorUtils::GetMax(main_run.gamma_ts_) << "\n";

  std::cout << "Vega "
    << HFSAT::VectorUtils::GetMean(main_run.vega_ts_) << " "
    << HFSAT::VectorUtils::GetMedian(main_run.vega_ts_) << " "
    << HFSAT::VectorUtils::GetStdev(main_run.vega_ts_) << " "
    << HFSAT::VectorUtils::GetMin(main_run.vega_ts_) << " "
    << HFSAT::VectorUtils::GetMax(main_run.vega_ts_) << "\n";

  std::cout << "Theta "
    << HFSAT::VectorUtils::GetMean(main_run.theta_ts_) << " "
    << HFSAT::VectorUtils::GetMedian(main_run.theta_ts_) << " "
    << HFSAT::VectorUtils::GetStdev(main_run.theta_ts_) << " "
    << HFSAT::VectorUtils::GetMin(main_run.theta_ts_) << " "
    << HFSAT::VectorUtils::GetMax(main_run.theta_ts_) << "\n";

}

