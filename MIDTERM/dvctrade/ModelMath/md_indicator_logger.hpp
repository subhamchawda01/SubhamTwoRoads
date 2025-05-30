/**
   \file ModelMath/md_indicator_logger.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
//#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/TradeUtils/market_update_manager.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvccode/Utils/in_memory_data.hpp"

namespace HFSAT {

class MDIndicatorLogger : public IndicatorListener, public SecurityMarketViewOnReadyListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  //  std::string instruction_filename_;
  //  const std::string& final_output_file_;

  /// internal variable for computation of status of model
  std::vector<bool> is_ready_vec_;
  std::vector<bool> readiness_required_vec_;
  bool is_ready_;

  // individual indicators followed
  std::vector<CommonIndicator*> indicator_vec_;
  unsigned int number_of_iindicators_;
  unsigned int number_of_gindicators_;

  unsigned int number_of_dependants_;
  std::vector<SecurityMarketView*> p_dep_market_view_vec_;
  std::vector<PriceType_t> dep_baseprice_type_vec_;

  std::vector<double> prev_value_vec_;
  // BulkFileWriter& bulk_file_writer_;

  // these vars control when new samples are printed
  const unsigned int msecs_to_wait_to_print_again_;
  unsigned int last_print_msecs_from_midnight_;
  const unsigned int num_trades_to_wait_print_again_;
  unsigned int last_print_num_trades_;

  int min_msec_toprint_;
  int max_msec_toprint_;

  int last_indicators_debug_print_;

  InMemData* in_memory_tsdata_;
  InMemData* in_memory_regdata_;

 public:
  MDIndicatorLogger(DebugLogger& _dbglogger_, const Watch& _watch_,
                    // BulkFileWriter& _bulk_file_writer_,
                    // const std::string& _output_filename_,
                    const unsigned int t_msecs_to_wait_to_print_again_,
                    const unsigned int t_num_trades_to_wait_print_again_);

  ~MDIndicatorLogger() {
    // clean up memory
    // std::cerr << in_memory_tsdata_->NumLines() << "\n";
    // std::cerr << in_memory_regdata_->NumLines() << "\n";
    delete in_memory_tsdata_;
    delete in_memory_regdata_;

    // all the indicators created are not destructed anywhere ( as far as I know )
    unsigned int deleted_instances_ = CommonIndicator::ClearIndicatorMap();

    if (deleted_instances_ < indicator_vec_.size()) {
      std::cerr << "Not all created indicator instances are deleted "
                << "Created: " << indicator_vec_.size() << " Deleted: " << deleted_instances_ << "\n";
      exit(-1);
    }
  }

  void AddDependant(SecurityMarketView* dependant_smv_, const PriceType_t baseprice_type_);
  void AddUnweightedIndicator(CommonIndicator* _this_indicator_, bool is_global_ = false);

  // define interrupts
  // set_start and end_times for sample data
  // add indicator listener
  // add onready listener

  // protected_function_to_trigger_logging

  // trading :
  /*static BaseModelMath* CreateMDModelMath(DebugLogger& _dbglogger_, const Watch& _watch_,
                                          const std::string& _model_filename_);


  static void LinkupModelMathToOnReadySources(BaseModelMath* p_this_model_math_,
                                              std::vector<std::string>& shortcodes_affecting_this_model_,
                                              std::vector<std::string>& ors_source_needed_vec_);*/

  void SubscribeMarketInterrupts(MarketUpdateManager& market_update_manager_) {
    for (auto i = 0u; i < indicator_vec_.size(); i++) {
      if (indicator_vec_[i] != NULL) {
        market_update_manager_.AddMarketDataInterruptedListener(indicator_vec_[i]);
        indicator_vec_[i]->SubscribeDataInterrupts(market_update_manager_);
      }
    }
  }

  void set_start_end_mfm() {
    if (p_dep_market_view_vec_.size() != 0) {
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        indicator_vec_[i]->set_start_mfm(min_msec_toprint_);
        indicator_vec_[i]->set_end_mfm(max_msec_toprint_);
      }
    }
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
    if (!is_ready_) {
      is_ready_vec_[_indicator_index_] = true;
      is_ready_ = AreAllReady();

      if ((!is_ready_) && (VectorUtils::CheckAllForValue(is_ready_vec_, false) == false) &&
          ((last_indicators_debug_print_ == 0) ||
           (watch_.msecs_from_midnight() >
            last_indicators_debug_print_ + 2000))) {  // some indicator is not ready but at least one other is ready
        last_indicators_debug_print_ = watch_.msecs_from_midnight();

        for (auto i = 0u; i < indicator_vec_.size(); i++) {
          if (is_ready_vec_[i]) {
          } else {
          }
          if (!is_ready_vec_[i]) {  // if this indicator isn't ready
            // print this and
            // ask if it was ready, but just could not notify us ... basically did not get any update yet.
            if (indicator_vec_[i]->IsIndicatorReady()) {  // if it was secretly ready
              is_ready_vec_[i] = indicator_vec_[i]->IsIndicatorReady();
              is_ready_ = AreAllReady();
            } else {
              if (!dbglogger_.IsNoLogs()) {
                DBGLOG_TIME_CLASS_FUNC << "Indicator Not Ready " << indicator_vec_[i]->concise_indicator_description()
                                       << DBGLOG_ENDL_FLUSH;
                // DBGLOG_DUMP ; // no dump since this is always in historical
                indicator_vec_[i]->WhyNotReady();
              }
            }
          }
        }
        if (is_ready_) {
          if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
            DBGLOG_TIME_CLASS_FUNC << "All Indicators Ready " << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
    // if(!is_ready_) { std::cerr << "Isready is not fine\n";}
    prev_value_vec_[_indicator_index_] = _new_value_;  // doing this irrespective of readiness of all indicators, so
    // that we always have correct indicator_vals in prev_value_vec_
  }

  // MDIndicatorLogger is notified after all the indicators are updated.
  // Indicators get updates from
  // (i) Portfolio which listens to SMV updates
  // (ii) SMV updates
  // (iii) GlobalPositionChange from PromOrderManager
  inline void SMVOnReady() { ShouldPrintNow(); }

  /// time support
  inline void setStartTM(int hhmm) { min_msec_toprint_ = GetMsecsFromMidnightFromHHMM(hhmm); }
  inline void setEndTM(int start_date_, int end_date_, int hhmm) {
    max_msec_toprint_ = GetMsecsFromMidnightFromHHMM(hhmm) + (end_date_ - start_date_) * 86400000;
  }

  // print to file from memory
  // 1 = timedata. 2 = regdata.
  inline void DumpData(int type_, const std::string& file_name_, std::ios_base::openmode flags_,
                       bool t_print_ref_ = true) {
    std::ofstream local_ofstream_;
    local_ofstream_.open(file_name_.c_str(), flags_);
    if (type_ == 1 && local_ofstream_.is_open()) {
      int num_rows_ = in_memory_tsdata_->NumLines();
      int num_cols_ = in_memory_tsdata_->NumWords();
      for (int i = 0; i < num_rows_; i++) {
        int j = 0;
        if (!t_print_ref_) {
          j = 2;
        }
        local_ofstream_ << in_memory_tsdata_->Item(i, j);
        j++;
        for (; j < num_cols_; j++) {
          local_ofstream_ << ' ' << in_memory_tsdata_->Item(i, j);
        }
        local_ofstream_ << '\n';
      }
    } else if (type_ == 2 && local_ofstream_.is_open()) {
      int num_rows_ = in_memory_regdata_->NumLines();
      int num_cols_ = in_memory_regdata_->NumWords();
      for (int i = 0; i < num_rows_; i++) {
        local_ofstream_ << in_memory_regdata_->Item(i, 0);
        for (int j = 1; j < num_cols_; j++) {
          local_ofstream_ << ' ' << in_memory_regdata_->Item(i, j);
        }
        local_ofstream_ << '\n';
      }
    }
    // bulk_file_writer_.Close();
    local_ofstream_.close();
  }

  // TimeSeries Stats

  // ReturnsSpace TimeSeries
  // ReturnSpace Stats

  // ChangeSeries
  // na_t1, na_t3 ALGO
  // msecs_duration
  void MakeRegData(const char* process_algo_, const unsigned int duration_in_msecs_);

  // ChangeSeries Stats

 protected:
  inline unsigned int num_trades() const {
    unsigned int ntrades_ = 0;
    for (auto i = 0u; i < p_dep_market_view_vec_.size(); i++) {
      ntrades_ += p_dep_market_view_vec_[i]->num_trades();
    }
    return ntrades_;
  }

  inline const bool ReachedTimePeriodToPrint() const {
    return ((last_print_msecs_from_midnight_ == 0u) ||
            (watch_.msecs_from_midnight() > (int)(last_print_msecs_from_midnight_ + msecs_to_wait_to_print_again_)));
  }

  inline const unsigned int ReachedNumTradesToPrint() const {
    return ((num_trades_to_wait_print_again_ > 0u) &&
            (num_trades() > last_print_num_trades_ + num_trades_to_wait_print_again_));
  }

 protected:
  inline bool AreAllReady() {
    for (auto i = 0u; i < p_dep_market_view_vec_.size(); i++) {
      if (!p_dep_market_view_vec_[i]->is_ready_complex(2)) {
        return false;
      }
    }

    for (auto i = 0u; i < is_ready_vec_.size(); ++i) {
      if (is_ready_vec_[i] == false && readiness_required_vec_[i] == true) {
        return false;
      }
    }
    return true;
  }

  inline virtual void ShouldPrintNow() {
    if ((is_ready_) && (ReachedTimePeriodToPrint() || ReachedNumTradesToPrint()) &&
        (watch_.msecs_from_midnight() > min_msec_toprint_) && (watch_.msecs_from_midnight() < max_msec_toprint_)) {
      last_print_msecs_from_midnight_ = watch_.msecs_from_midnight();
      last_print_num_trades_ = num_trades();

      in_memory_tsdata_->AddWord(last_print_msecs_from_midnight_);
      in_memory_tsdata_->AddWord(last_print_num_trades_);

      for (auto i = 0u; i < p_dep_market_view_vec_.size(); i++) {
        in_memory_tsdata_->AddWord(p_dep_market_view_vec_[i]->price_from_type(dep_baseprice_type_vec_[i]));
      }
      for (auto i = 0u; i < indicator_vec_.size(); i++) {
        in_memory_tsdata_->AddWord(prev_value_vec_[i]);
      }
      in_memory_tsdata_->FinishLine();
    } else {
      if ((!is_ready_) && AreAllReady()) {
        is_ready_ = true;
      }
    }
  }
};
}
