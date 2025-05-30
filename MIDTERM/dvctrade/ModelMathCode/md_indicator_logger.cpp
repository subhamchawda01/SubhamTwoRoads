/**
   \file ModelMath/md_indicator_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "dvctrade/ModelMath/md_indicator_logger.hpp"
#include "dvctrade/ModelMath/nse_simulator_processing.hpp"

namespace HFSAT {
MDIndicatorLogger::MDIndicatorLogger(DebugLogger& _dbglogger_, const Watch& _watch_,
                                     // BulkFileWriter& _bulk_file_writer_,
                                     // const std::string& _output_filename_,
                                     const unsigned int t_msecs_to_wait_to_print_again_,
                                     const unsigned int t_num_trades_to_wait_print_again_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      //  final_output_file_(_output_filename_),
      is_ready_vec_(),
      readiness_required_vec_(),
      is_ready_(false),
      indicator_vec_(),
      number_of_iindicators_(0u),
      number_of_gindicators_(0u),
      number_of_dependants_(0u),
      p_dep_market_view_vec_(),
      dep_baseprice_type_vec_(),
      prev_value_vec_(),
      // bulk_file_writer_(_bulk_file_writer_),
      msecs_to_wait_to_print_again_(t_msecs_to_wait_to_print_again_),
      last_print_msecs_from_midnight_(0u),
      num_trades_to_wait_print_again_(t_num_trades_to_wait_print_again_),
      last_print_num_trades_(0u),
      min_msec_toprint_(0),
      max_msec_toprint_(0),
      last_indicators_debug_print_(0) {
  in_memory_tsdata_ = new InMemData();
  in_memory_regdata_ = new InMemData();
}

void MDIndicatorLogger::AddDependant(SecurityMarketView* dependant_smv_, const PriceType_t baseprice_type_) {
  // VectorUtils::UniqueVectorAdd(p_dep_market_view_vec_, dependant_smv_);
  p_dep_market_view_vec_.push_back(dependant_smv_);

  // NULL subscriptions ensures computation
  if (!dependant_smv_->subscribe_price_type(NULL, baseprice_type_)) {
    PriceType_t t_error_price_type_ = baseprice_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  dep_baseprice_type_vec_.push_back(baseprice_type_);
  number_of_dependants_++;
  // std::cerr << "Adding dependant " << p_dep_market_view_vec_.size() << "\n";
}

void MDIndicatorLogger::AddUnweightedIndicator(CommonIndicator* _this_indicator_, bool is_global_) {
  if (_this_indicator_ != NULL) {
    indicator_vec_.push_back(_this_indicator_);
    _this_indicator_->add_unweighted_indicator_listener(indicator_vec_.size() - 1, this);

    if (is_global_) {
      number_of_gindicators_++;
    } else {
      number_of_iindicators_++;
    }
    bool is_this_ready_ = _this_indicator_->IsIndicatorReady();
    if (!is_this_ready_) {
      is_ready_vec_.push_back(false);
      readiness_required_vec_.push_back(true);
    } else {
      is_ready_vec_.push_back(true);
      readiness_required_vec_.push_back(false);
    }
    prev_value_vec_.push_back(0);
  } else {
    if (dbglogger_.CheckLoggingLevel(DBG_MODEL_ERROR)) {
      DBGLOG_TIME_CLASS_FUNC << "Empty Indicator Added " << DBGLOG_ENDL_FLUSH;
    }
    ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
  }
  // std::cerr << "Adding Indicator " << indicator_vec_.size() << "\n";
}

void MDIndicatorLogger::MakeRegData(const char* process_algo_, const unsigned int duration_in_msecs_) {
  if (strcmp(process_algo_, "na_t1") == 0) {
    InMemT1Processing(in_memory_tsdata_, in_memory_regdata_, duration_in_msecs_, number_of_dependants_);
  } else if (strcmp(process_algo_, "na_t3") == 0) {
    InMemT3Processing(in_memory_tsdata_, in_memory_regdata_, duration_in_msecs_, number_of_dependants_);
  } else {
    std::cerr << "Unrecognised processing algo \n";
    ExitVerbose(kExitErrorCodeGeneral, dbglogger_);
  }
}
}
