/**
   \file ModelMathCode/model_creator.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <string.h>

#include <boost/tokenizer.hpp>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/ModelMath/linear_model_aggregator.hpp"
#include "dvctrade/ModelMath/logistic_model_aggregator.hpp"
#include "dvctrade/ModelMath/siglr_model_aggregator.hpp"
#include "dvctrade/ModelMath/random_forest_model_aggregator.hpp"
#include "dvctrade/ModelMath/nodependant_linear_model_aggregator.hpp"
#include "dvctrade/ModelMath/model_creator.hpp"
#include "dvctrade/ModelMath/non_linear_model_aggregator.hpp"
#include "dvctrade/ModelMath/selective_model_aggregator.hpp"
#include "dvctrade/ModelMath/selective_model_aggregator_new.hpp"
#include "dvctrade/ModelMath/selective_model_aggregator_siglr.hpp"
#include "dvctrade/ModelMath/boosting_model_aggregator.hpp"
#include "dvctrade/ModelMath/tree_boosting_model_aggregator.hpp"
#include "dvctrade/ModelMath/regime_model_aggregator.hpp"
#include "dvctrade/ModelMath/online_linear_model_aggregator.hpp"
#include "dvctrade/ModelMath/online_selective_model_aggregator.hpp"
#include "dvctrade/ModelMath/selective_continuous_model_aggregator.hpp"
#include "dvctrade/ModelMath/md_model_aggregator.hpp"
#include "dvctrade/ModelMath/gradient_boosting_classifier_model_aggregator.hpp"
#include "dvctrade/ModelMath/mult_model_creator.hpp"

#include "dvctrade/InitCommon/strategy_desc.hpp"

#include "dvctrade/Indicators/indicator_list.hpp"

namespace HFSAT {
std::map<std::string, BaseModelMath*> ModelCreator::modelfilename_basemodelmath_map_;
std::vector<BaseModelMath*> ModelCreator::modelfilename_basemodelmath_map_exclude_;
std::vector<std::vector<std::string> > ModelCreator::indicator_token_strings_vec_;
std::vector<SignalAlgo_t> ModelCreator::signal_algo_vec_;
std::vector<BaseModelMath*> ModelCreator::model_math_vec_;
std::vector<std::string> ModelCreator::indicator_order_vec_;
std::vector<SecurityMarketView*> ModelCreator::dep_market_view_vec_;
std::vector<PriceType_t> ModelCreator::baseprice_vec_;
std::vector<int> ModelCreator::model_entry_type_vec_;  // 1 for INDICATOR 2 for NONLINEARCOMPONENT
std::vector<double> ModelCreator::model_scaling_factor_vec_;
std::vector<double> regime_cutoffs_;
unsigned int num_regimes_ = 0;
std::vector<double> regime_index_prob_;

bool ModelCreator::CollectRegimeModelShortcodes(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                const std::string _model_filename_,
                                                std::vector<std::string>& rw_shortcodes_affecting_this_model_,
                                                std::vector<std::string>& ors_source_needed_vec_,
                                                bool allow_non_dependant_, std::string& _dep_shortcode_) {
  std::ifstream model_file_;
  model_file_.open(_model_filename_.c_str(), std::ifstream::in);
  bool is_this_regime_model_ = false;
  if (model_file_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    while (model_file_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_file_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_file_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        } else if (std::string(tokens_[0]).compare("REGIMEMODEL") == 0) {
          _dep_shortcode_ = CollectShortCodes(t_dbglogger_, cr_watch_, tokens_[1], rw_shortcodes_affecting_this_model_,
                                              ors_source_needed_vec_, allow_non_dependant_, false);
          is_this_regime_model_ = true;
        } else if (std::string(tokens_[0]).compare("REGIMEINDICATOR") == 0) {
          (CollectShortCodeFunc(tokens_[2]))(rw_shortcodes_affecting_this_model_, ors_source_needed_vec_, tokens_);
          is_this_regime_model_ = true;
        } else {
          break;
        }
      }
    }
  }
  return is_this_regime_model_;
}

std::string ModelCreator::CollectShortCodes(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                            const std::string _model_filename_,
                                            std::vector<std::string>& rw_shortcodes_affecting_this_model_,
                                            std::vector<std::string>& ors_source_needed_vec_, bool allow_non_dependant_,
                                            bool _check_regime_model_) {
  std::string dep_shortcode_ = "NONAME";
  if (_check_regime_model_) {
    bool this_is_regime_model_ =
        CollectRegimeModelShortcodes(t_dbglogger_, cr_watch_, _model_filename_, rw_shortcodes_affecting_this_model_,
                                     ors_source_needed_vec_, allow_non_dependant_, dep_shortcode_);
    if (this_is_regime_model_) {
      return dep_shortcode_;
    }
  }

  SetIndicatorListMap();

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (!model_infile_.is_open()) {
    ExitVerbose(kModelCreationCouldNotOpenModelFile);
  } else {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    ModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }
        switch (current_model_creation_phase_) {
          case kModelCreationPhaseInit:
            if (strcmp(tokens_[0], "MODELINIT") == 0) {  // MODELINIT
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelInitLineLessArgs);
              }
              if (strcmp(tokens_[1], "DEPBASE") == 0) {  // MODELINIT DEPBASE ZN0
                if (tokens_.size() < 3) {
                  ExitVerbose(kModelCreationDepBaseLineLessArgs);
                }
                // Writing "MODELINIT DEPBASE NONAME" allows us to test if we can generate data of only the indicators
                // without any dependant line.
                if (strcmp(tokens_[2], "NONAME") != 0) {
                  VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, (std::string)tokens_[2]);
                  dep_shortcode_ = tokens_[2];
                }
              }
              current_model_creation_phase_ = kModelCreationPhasePreModel;
            }
            break;
          case kModelCreationPhasePreModel:
            if (strcmp(tokens_[0], "MODELMATH") == 0) {  // MODELMATH [LINEAR RETURNS]
              current_model_creation_phase_ = kModelCreationPhasePostModel;
            }
            break;

          case kModelCreationPhasePostModel:
            if (strcmp(tokens_[0], "INDICATORSTART") == 0) {  // INDICATORSTART
              current_model_creation_phase_ = kModelCreationPhaseIndicatorStarted;
            }
            if (strcmp(tokens_[0], "REGIMEINDICATOR") == 0) {
              if (tokens_.size() < 3) {
                ExitVerbose(kModelCreationIndicatorLineLessArgs);
              }
              (CollectShortCodeFunc(tokens_[2]))(rw_shortcodes_affecting_this_model_, ors_source_needed_vec_, tokens_);
            }
            if (strcmp(tokens_[0], "USE_IMPLIED_PRICE_FOR_NIK") == 0 && dep_shortcode_ == "NK_0") {
              std::string nkm_shortcode_ = "NKM_0";
              VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, nkm_shortcode_);
            }

            break;
          case kModelCreationPhaseIndicatorStarted:
            if (strcmp(tokens_[0], "INDICATOREND") == 0) {  // INDICATOREND
              current_model_creation_phase_ = kModelCreationPhaseIndicatorEnded;
            } else {
              if (strcmp(tokens_[0], "INDICATOR") == 0 || strcmp(tokens_[0], "REGIMEINDICATOR") == 0) {
                if (tokens_.size() < 3) {
                  ExitVerbose(kModelCreationIndicatorLineLessArgs);
                }
                (CollectShortCodeFunc(tokens_[2]))(rw_shortcodes_affecting_this_model_, ors_source_needed_vec_,
                                                   tokens_);
              }
            }
            break;
          case kModelCreationPhaseIndicatorEnded:
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }

  if (HFSAT::VectorUtils::LinearSearchValue(rw_shortcodes_affecting_this_model_, std::string("HYB_ESPY"))) {
    HFSAT::VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, std::string("ES_0"));
    HFSAT::VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, std::string("SPY"));
  }
  if (HFSAT::VectorUtils::LinearSearchValue(rw_shortcodes_affecting_this_model_, std::string("HYB_NQQQ"))) {
    HFSAT::VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, std::string("NQ_0"));
    HFSAT::VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, std::string("QQQ"));
  }
  if (HFSAT::VectorUtils::LinearSearchValue(rw_shortcodes_affecting_this_model_, std::string("HYB_YDIA"))) {
    HFSAT::VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, std::string("YM_0"));
    HFSAT::VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, std::string("DIA"));
  }

  HFSAT::SyntheticSecurityManager& synthetic_security_manager_ = HFSAT::SyntheticSecurityManager::GetUniqueInstance();
  for (auto i = 0u; i < rw_shortcodes_affecting_this_model_.size(); i++) {
    if (synthetic_security_manager_.IsSyntheticSecurity(rw_shortcodes_affecting_this_model_[i])) {
      std::vector<std::string> t_constituent_vec_ =
          synthetic_security_manager_.GetConstituentSHC(rw_shortcodes_affecting_this_model_[i]);
      for (unsigned int const_shc_idx_ = 0; const_shc_idx_ < t_constituent_vec_.size(); const_shc_idx_++) {
        HFSAT::VectorUtils::UniqueVectorAdd(rw_shortcodes_affecting_this_model_, t_constituent_vec_[const_shc_idx_]);
      }
    }
  }

  return dep_shortcode_;
}

void ModelCreator::CollectMDModelShortCodes(DebugLogger& _dbglogger_, const Watch& _watch_,
                                            std::string _instruction_filename_,
                                            std::vector<std::string>& dependant_shortcodes_,
                                            std::vector<std::string>& source_shortcodes_) {
  std::vector<std::string> futures_;
  std::map<std::string, std::vector<std::string> > futures_2_options_maps_;

  //  order of intialization for indicators
  SetIndicatorListMap();
  std::ifstream model_infile_;
  // model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  // simply navigate through files and collect shortcodes
  if (!model_infile_.is_open()) {
    ExitVerbose(kModelCreationCouldNotOpenModelFile);
  } else {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    MModelCreationPhases_t current_model_creation_phase_ = kMModelCreationPhaseModelInit;
    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) {
          continue;
        }
        switch (current_model_creation_phase_) {
          case kMModelCreationPhaseModelInit:
            if (strcmp(tokens_[0], "UNDERLYING") == 0) {
              current_model_creation_phase_ = kMModelCreationPhaseShortCodes;
            } else {
              ExitVerbose(kModelCreationModelInitLineLessArgs, "UNDERLYING");
            }
            break;
          case kMModelCreationPhaseShortCodes:
            if (tokens_.size() >= 3) {
              futures_.push_back(tokens_[0]);
              futures_2_options_maps_[tokens_[0]] = std::vector<std::string>();
              VectorUtils::UniqueVectorAdd(dependant_shortcodes_, (std::string)tokens_[0]);
              VectorUtils::UniqueVectorAdd(source_shortcodes_, (std::string)tokens_[0]);
              {
                std::vector<std::string> options_ = HFSAT::MultModelCreator::GetOptionsToTrade(tokens_);
                for (auto i = 0u; i < options_.size(); i++) {
                  VectorUtils::UniqueVectorAdd(source_shortcodes_, (std::string)options_[i]);
                  VectorUtils::UniqueVectorAdd(dependant_shortcodes_, (std::string)options_[i]);
                  futures_2_options_maps_[tokens_[0]].push_back(options_[i]);
                }
              }
            } else if (strcmp(tokens_[0], "IINDICATORS") == 0) {
              current_model_creation_phase_ = kMModelCreationPhaseIIndicators;
            } else {
              ExitVerbose(kModelCreationNoModelWeightIndicator, "IINDICATORS Missing");
            }
            break;
          case kMModelCreationPhaseIIndicators:
            if (strcmp(tokens_[0], "INDICATOR") == 0 && tokens_.size() >= 3) {
              std::vector<std::string> tokens1_;
              for (auto i = 0u; i < tokens_.size(); i++) {
                tokens1_.push_back(tokens_[i]);
              }
              std::string dummy_underlying_shc_ = "FUT";
              for (auto i = 0u; i < futures_.size(); i++) {
                std::string dummy_options_shc_ = "OPT";
                std::vector<std::string> const_vec_ = futures_2_options_maps_[futures_[i]];
                for (unsigned int j = 0; j < const_vec_.size(); j++) {
                  std::vector<const char*> vc;
                  for (auto _token_ : tokens1_) {
                    if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0) {
                      vc.push_back(futures_[i].c_str());
                    } else if (strcmp(_token_.c_str(), dummy_options_shc_.c_str()) == 0) {
                      vc.push_back(const_vec_[j].c_str());
                    } else {
                      vc.push_back(_token_.c_str());
                    }
                  }
                  // no of indicators per line == no_of_futures * no_of_contracts
                  (CollectShortCodeFunc(tokens1_[2]))(source_shortcodes_, source_shortcodes_, vc);
                }
              }
            } else if (strcmp(tokens_[0], "GINDICATORS") == 0) {
              current_model_creation_phase_ = kMModelCreationPhaseGIndicators;
            } else {
              ExitVerbose(kModelCreationModelMathLineLessArgs, "GINDICATORS Missing");
            }
            break;
          case kMModelCreationPhaseGIndicators:
            if (strcmp(tokens_[0], "INDICATOR") == 0 && tokens_.size() >= 3) {
              std::vector<std::string> tokens1_;
              for (auto i = 0u; i < tokens_.size(); i++) {
                tokens1_.push_back(tokens_[i]);
              }
              std::string dummy_underlying_shc_ = "FUT";
              for (auto i = 0u; i < futures_.size(); i++) {
                std::vector<const char*> vc;
                for (auto _token_ : tokens1_) {
                  if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0)
                    vc.push_back(futures_[i].c_str());
                  else
                    vc.push_back(_token_.c_str());
                }
                // no of indicators per line == no_of_futures
                (CollectShortCodeFunc(tokens_[2]))(source_shortcodes_, source_shortcodes_, vc);
              }
            } else if (strcmp(tokens_[0], "MODELEND") == 0) {
              current_model_creation_phase_ = kMModelCreationPhaseEnd;
            } else {
              ExitVerbose(kModelCreationModelMathLineLessArgs, "MODELEND Missing");
            }
            break;
          case kMModelCreationPhaseModelEnd:
            break;
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }
}

bool ModelCreator::ShcPresentInFile(std::string filename, std::string shortcode) {
  std::ifstream t_offline_mix_mms_infile_;
  t_offline_mix_mms_infile_.open(filename.c_str(), std::ifstream::in);
  bool wts_found_ = false;
  if (t_offline_mix_mms_infile_.is_open()) {
    const int kOFFLINEMIXLineBufferLen = 1024;
    char readline_buffer_[kOFFLINEMIXLineBufferLen];
    bzero(readline_buffer_, kOFFLINEMIXLineBufferLen);

    while (t_offline_mix_mms_infile_.good()) {
      bzero(readline_buffer_, kOFFLINEMIXLineBufferLen);
      t_offline_mix_mms_infile_.getline(readline_buffer_, kOFFLINEMIXLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kOFFLINEMIXLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= NUM_PRICETYPES_IN_OFFLINEMIXMMS + 1) {
        std::string dep_ = tokens_[0];
        if (shortcode.compare(dep_) == 0) {
          wts_found_ = true;
          break;
        }
      }
    }
    t_offline_mix_mms_infile_.close();
  }
  return wts_found_;
}

std::string ModelCreator::GetOfflineMixMMSWtsFileName(std::string _model_filename_, int _trading_start_utc_mfm_,
                                                      std::string _dep_shc_) {
  std::string retval_ = std::string(DEFAULT_OFFLINEMIXMMS_FILE);

  int pre_as_start_time_mfm_ = 43 * 1800 * 1000;  // UTC_2130, AST_1630
  int eu_start_time_mfm_ = 11 * 1800 * 1000;      // UTC_530
  int us_start_time_mfm_ = 21 * 1800 * 1000;      // UTC_1030

  if (_dep_shc_.compare("") != 0 && _dep_shc_.compare("NONAME") != 0) {
    if (_trading_start_utc_mfm_ <= eu_start_time_mfm_ || _trading_start_utc_mfm_ >= pre_as_start_time_mfm_) {
      if (FileUtils::exists(std::string(DEFAULT_OFFLINEMIXMMS_FILE_AS)) &&
          ShcPresentInFile(std::string(DEFAULT_OFFLINEMIXMMS_FILE_AS), _dep_shc_)) {
        retval_ = std::string(DEFAULT_OFFLINEMIXMMS_FILE_AS);
      }
    } else if (_trading_start_utc_mfm_ <= us_start_time_mfm_) {
      if (FileUtils::exists(std::string(DEFAULT_OFFLINEMIXMMS_FILE_EU)) &&
          ShcPresentInFile(std::string(DEFAULT_OFFLINEMIXMMS_FILE_EU), _dep_shc_)) {
        retval_ = std::string(DEFAULT_OFFLINEMIXMMS_FILE_EU);
      }
    } else {
      if (FileUtils::exists(std::string(DEFAULT_OFFLINEMIXMMS_FILE_US)) &&
          ShcPresentInFile(std::string(DEFAULT_OFFLINEMIXMMS_FILE_US), _dep_shc_)) {
        retval_ = std::string(DEFAULT_OFFLINEMIXMMS_FILE_US);
      }
    }
  }

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (!model_infile_.is_open()) {
    ExitVerbose(kModelCreationCouldNotOpenModelFile);
  } else {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "OFFLINEMIXMMS_FILE") == 0)) {
          retval_ = tokens_[1];
        }
      }
    }
    model_infile_.close();
  }

  return retval_;
}

bool ModelCreator::IsShcPresentInOnlineFilename(std::string filename, std::string shortcode) {
  std::ifstream t_online_price_infile_;
  t_online_price_infile_.open(filename.c_str(), std::ifstream::in);
  bool const_found_ = false;
  if (t_online_price_infile_.is_open()) {
    const int kONLINELineBufferLen = 1024;
    char readline_buffer_[kONLINELineBufferLen];
    bzero(readline_buffer_, kONLINELineBufferLen);

    while (t_online_price_infile_.good()) {
      bzero(readline_buffer_, kONLINELineBufferLen);
      t_online_price_infile_.getline(readline_buffer_, kONLINELineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kONLINELineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= NUM_CONST_IN_ONLINEMIXPRICE + 1) {
        std::string dep_ = tokens_[0];
        if (shortcode.compare(dep_) == 0) {
          const_found_ = true;
          break;
        }
      }
    }
    t_online_price_infile_.close();
  }
  return const_found_;
}

std::string ModelCreator::GetOnlinePriceConstFilename(std::string _model_filename_, int _trading_start_utc_mfm_,
                                                      std::string _dep_shc_) {
  std::string retval_ = std::string(DEFAULT_ONLINE_MIX_PRICE_FILE);

  int pre_as_start_time_mfm_ = 43 * 1800 * 1000;
  int eu_start_time_mfm_ = 11 * 1800 * 1000;
  int us_start_time_mfm_ = 21 * 1800 * 1000;

  if (_dep_shc_.compare("") != 0 && _dep_shc_.compare("NONAME") != 0) {
    if (_trading_start_utc_mfm_ <= eu_start_time_mfm_ || _trading_start_utc_mfm_ >= pre_as_start_time_mfm_) {
      if (FileUtils::exists(std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_AS)) &&
          IsShcPresentInOnlineFilename(std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_AS), _dep_shc_)) {
        retval_ = std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_AS);
      }
    } else if (_trading_start_utc_mfm_ <= us_start_time_mfm_) {
      if (FileUtils::exists(std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_EU)) &&
          IsShcPresentInOnlineFilename(std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_EU), _dep_shc_)) {
        retval_ = std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_EU);
      }
    } else {
      if (FileUtils::exists(std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_US)) &&
          IsShcPresentInOnlineFilename(std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_US), _dep_shc_)) {
        retval_ = std::string(DEFAULT_ONLINE_MIX_PRICE_FILE_US);
      }
    }
  }

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (!model_infile_.is_open()) {
    ExitVerbose(kModelCreationCouldNotOpenModelFile);
  } else {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "ONLINEMIXPRICE_FILE") == 0)) {
          retval_ = tokens_[1];
        }
      }
    }
    model_infile_.close();
  }

  return retval_;
}

std::string ModelCreator::GetOnlineBetaKalmanConstFilename(std::string _model_filename_, int _trading_start_utc_mfm_,
                                                           std::string _dep_shc_) {
  std::string retval_ = std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE);

  int eu_start_time_mfm_ = 11 * 1800 * 1000;
  int us_start_time_mfm_ = 23 * 1800 * 1000;

  if (_dep_shc_.compare("") != 0 && _dep_shc_.compare("NONAME") != 0) {
    if (_trading_start_utc_mfm_ <= eu_start_time_mfm_) {
      if (FileUtils::exists(std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_AS)) &&
          IsShcPresentInOnlineFilename(std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_AS), _dep_shc_)) {
        retval_ = std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_AS);
      }
    } else if (_trading_start_utc_mfm_ <= us_start_time_mfm_) {
      if (FileUtils::exists(std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_EU)) &&
          IsShcPresentInOnlineFilename(std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_EU), _dep_shc_)) {
        retval_ = std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_EU);
      }
    } else {
      if (FileUtils::exists(std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_US)) &&
          IsShcPresentInOnlineFilename(std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_US), _dep_shc_)) {
        retval_ = std::string(DEFAULT_ONLINE_BETA_KALMAN_FILE_US);
      }
    }
  }

  return retval_;
}

void ModelCreator::GetModelMathVec(std::vector<BaseModelMath*>& base_model_math_vec_) {
  for (std::map<std::string, BaseModelMath*>::iterator _iter_ = modelfilename_basemodelmath_map_.begin();
       _iter_ != modelfilename_basemodelmath_map_.end(); _iter_++) {
    if (!HFSAT::VectorUtils::LinearSearchValue(modelfilename_basemodelmath_map_exclude_, _iter_->second)) {
      if (_iter_->second->CheckCancellation())
        base_model_math_vec_.insert(base_model_math_vec_.begin(), _iter_->second);
      else
        base_model_math_vec_.push_back(_iter_->second);
    }
  }
}

void ModelCreator::LinkupModelMathToOnReadySources(BaseModelMath* p_this_model_math_,
                                                   std::vector<std::string>& shortcodes_affecting_this_model_,
                                                   std::vector<std::string>& ors_source_needed_vec_) {
  if (p_this_model_math_ != nullptr) {
    // subscribe_OnReady to all the shortcodes affecting this model
    for (auto i = 0u; i < shortcodes_affecting_this_model_.size(); i++) {
      (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcodes_affecting_this_model_[i]))
          ->subscribe_OnReady(p_this_model_math_);
    }
    // subscribe to PromOM events for sources in ors_source_needed_vec_
    // most indicators are global_position based and hence they would have been added as listeners already
    // now if we add model_math we could be giving control model math object to use the updated values of
    // the indicators to compute new target price and alert basetrading
    for (auto i = 0u; i < ors_source_needed_vec_.size(); i++) {
      PromOrderManager* p_this_prom_order_manager_ = PromOrderManager::GetCreatedInstance(ors_source_needed_vec_[i]);
      if (p_this_prom_order_manager_ != nullptr) {  // as should be
        p_this_prom_order_manager_->AddGlobalPositionChangeListener(p_this_model_math_);
      }
    }
  }
}

BaseModelMath* ModelCreator::CreateMDModelMath(DebugLogger& t_dbg_logger_, const Watch& cr_watch_,
                                               const std::string& _model_filename_) {
  std::string map_key_ = _model_filename_;
  if (modelfilename_basemodelmath_map_.find(map_key_) == modelfilename_basemodelmath_map_.end()) {
    std::ifstream model_infile_;
    model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
    if (model_infile_.is_open()) {
      const unsigned int kModelLineBufferLen = 1024;
      char readline_buffer_[kModelLineBufferLen];
      bzero(readline_buffer_, kModelLineBufferLen);

      SecurityMarketView* dep_market_view_ = nullptr;
      PriceType_t dep_base_pricetype_ = kPriceTypeMidprice;
      SignalAlgo_t signal_algo_ = kSignalAlgoLinear;
      BaseModelMath* p_this_model_math_ = nullptr;

      SignalAlgo_t submodel_signal_algo_ = kSignalAlgoLinear;
      BaseModelMath* p_this_submodel_math_ = nullptr;

      MModelCreationPhases_t current_model_creation_phase_ = kMModelCreationPhaseModelInit;

      while (model_infile_.good()) {
        // MODELINIT DEPBASE // init
        // MODELMATH MDLINEAR CHANGE // modelstart
        // WEIGHTINDICATOR // submodel weight

        // SUBMODELMATH LINEAR CHANGE // submodel modelmath
        // INDICATORSTART // submodel indicators
        // INDICATORSTART // submodel indicators
        // INDICATOREND // submodelend

        // WEIGHTINDICATOR // submodel weight

        // SUBMODELMATH SIGLR CHANGE // submodel modelmath
        // INDICATORSTART // submodel indicators
        // INDICATORSTART // submodel indicators
        // INDICATOREND // submodel end

        // MODELEND // modelend

        bzero(readline_buffer_, kModelLineBufferLen);
        model_infile_.getline(readline_buffer_, kModelLineBufferLen);

        if (model_infile_.gcount() > 0) {
          t_dbg_logger_ << readline_buffer_ << '\n';
          t_dbg_logger_.CheckToFlushBuffer();

          PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);
          const std::vector<const char*>& tokens_ = st_.GetTokens();
          if (tokens_.size() < 1) {
            continue;
          }
          switch (current_model_creation_phase_) {
            case kMModelCreationPhaseModelInit:
              if (tokens_.size() >= 4 && strcmp(tokens_[0], "MODELINIT") == 0 && strcmp(tokens_[1], "DEPBASE") == 0) {
                dep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens_[2]);
                dep_base_pricetype_ = StringToPriceType_t((std::string)tokens_[3]);
                dep_market_view_->SetBasePriceType(dep_base_pricetype_);
                current_model_creation_phase_ = kMModelCreationPhaseModelMath;
              } else {
                ExitVerbose(kModelCreationModelInitLineLessArgs, "MODELINIT");
              }
              break;
            case kMModelCreationPhaseModelMath:
              if (tokens_.size() >= 3 && strcmp(tokens_[0], "MODELMATH") == 0) {
                signal_algo_ = GetSignalAlgo(tokens_[1]);
                switch (signal_algo_) {
                  case kSignalAlgoMDLinear: {
                    p_this_model_math_ = new MDModelAggregator(t_dbg_logger_, cr_watch_, _model_filename_,
                                                               *dep_market_view_, dep_base_pricetype_);
                  } break;
                  default:
                    break;
                }
                current_model_creation_phase_ = kMModelCreationPhaseSubModelInit;
              } else {
                ExitVerbose(kModelCreationModelMathLineLessArgs, "MODELMATH");
              }
              if (p_this_model_math_ == nullptr) {
                ExitVerbose(kModelCreationModelMathLineLessArgs, "EMPTY MODELMATH");
              }
              break;
            case kMModelCreationPhaseSubModelInit:
              if (strcmp(tokens_[0], "WEIGHTINDICATOR") == 0 && tokens_.size() > 3) {
                // should we instantiate this along with other indicators
                CommonIndicator* _this_indicator_ =
                    GetIndicatorFromTokens(t_dbg_logger_, cr_watch_, tokens_, dep_base_pricetype_);
                if (p_this_model_math_ != nullptr && _this_indicator_ != nullptr) {
                  p_this_model_math_->SetModelWeightIndicator(_this_indicator_, false);
                  current_model_creation_phase_ = kMModelCreationPhaseSubModelMath;
                }
              } else if (strcmp(tokens_[0], "MODELEND") == 0) {
                current_model_creation_phase_ = kMModelCreationPhaseModelEnd;
              } else {
                ExitVerbose(kModelCreationNoModelWeightIndicator, "WEIGHT INDICATOR");
              }
              break;
            case kMModelCreationPhaseSubModelMath:
              if (tokens_.size() >= 3 && strcmp(tokens_[0], "SUBMODELMATH") == 0) {
                p_this_submodel_math_ = nullptr;
                submodel_signal_algo_ = GetSignalAlgo(tokens_[1]);
                bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                switch (submodel_signal_algo_) {
                  case kSignalAlgoLinear:
                    p_this_submodel_math_ =
                        new LinearModelAggregator(t_dbg_logger_, cr_watch_, _model_filename_, *dep_market_view_,
                                                  dep_base_pricetype_, is_returns_based_);
                    break;
                  case kSignalAlgoSIGLR:
                    p_this_submodel_math_ =
                        new SIGLRModelAggregator(t_dbg_logger_, cr_watch_, _model_filename_, *dep_market_view_,
                                                 dep_base_pricetype_, is_returns_based_);
                    break;
                  default:
                    break;
                }
              } else {
                ExitVerbose(kModelCreationModelMathLineLessArgs, "SUBMODELMATH");
              }
              if (p_this_submodel_math_ != nullptr) {
                p_this_model_math_->SetSubModelMath(p_this_submodel_math_);
                current_model_creation_phase_ = kMModelCreationPhasePostSubModelMath;
              }
              break;
            case kMModelCreationPhasePostSubModelMath:
              if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
                current_model_creation_phase_ = kMModelCreationPhaseSubModelIndicatorStarted;
              }
              break;
            case kMModelCreationPhaseSubModelIndicatorStarted:
              if (strcmp(tokens_[0], "INDICATOREND") == 0) {
                current_model_creation_phase_ = kMModelCreationPhaseSubModelInit;
              } else if (strcmp(tokens_[0], "INDICATOR") == 0 && tokens_.size() > 3) {
                // we collect all the indicators for this program, that means for multiple modelfiles and
                // for multiple modelmaths. we assign for every indicator the modelmath pointer
                // we added respective submodelmath to modelmath and modelmath to modelfile
                std::vector<std::string> this_indicator_tokens_vec_;
                for (unsigned index_ = 0; index_ < tokens_.size(); index_++) {
                  this_indicator_tokens_vec_.push_back(tokens_[index_]);
                }
                indicator_token_strings_vec_.push_back(this_indicator_tokens_vec_);
                signal_algo_vec_.push_back(submodel_signal_algo_);
                model_math_vec_.push_back(p_this_submodel_math_);
                dep_market_view_vec_.push_back(dep_market_view_);
                baseprice_vec_.push_back(dep_base_pricetype_);
              }
              break;
            case kMModelCreationPhaseSubModelEnd:
              break;
            default:
              break;
          }
        }
      }
      model_infile_.close();
      modelfilename_basemodelmath_map_[map_key_] = p_this_model_math_;
    }
  }
  return modelfilename_basemodelmath_map_[map_key_];
}

BaseModelMath* ModelCreator::CreateRegimeModelMath(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                   const std::string& _model_filename_,
                                                   std::vector<double>* _p_scaling_model_vec_, int trading_start_mfm,
                                                   int trading_end_mfm, int runtime_id) {
  std::ifstream model_file_;
  model_file_.open(_model_filename_.c_str(), std::ifstream::in);
  BaseModelMath* p_regime_model_math_ = nullptr;
  unsigned int index_ = 0;
  if (model_file_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    while (model_file_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_file_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_file_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() < 1 || tokens_[0][0] == '#' || strcmp(tokens_[0], "MODELINFO") == 0) {  // skip empty lines
          continue;
        } else if (std::string(tokens_[0]).compare("REGIMEMODEL") == 0) {
          std::vector<double> this_model_scaling_factors_vec_;
          if (_p_scaling_model_vec_ != nullptr && _p_scaling_model_vec_->size() > index_) {
            this_model_scaling_factors_vec_.push_back((*_p_scaling_model_vec_)[index_]);
          }
          BaseModelMath* this_reg_model_math_ =
              CreateModelMathComponent(t_dbglogger_, cr_watch_, tokens_[1], &this_model_scaling_factors_vec_,
                                       trading_start_mfm, trading_end_mfm, runtime_id);
          HFSAT::VectorUtils::UniqueVectorAdd(modelfilename_basemodelmath_map_exclude_, this_reg_model_math_);

          if (p_regime_model_math_ == nullptr) {
            p_regime_model_math_ = new RegimeModelAggregator(t_dbglogger_, cr_watch_, _model_filename_);
          }

          if (p_regime_model_math_ != nullptr) {
            p_regime_model_math_->SetRegimeModelMath(this_reg_model_math_, index_);
            index_++;
          }
        } else if (std::string(tokens_[0]).compare("REGIMEINDICATOR") == 0) {
          CommonIndicator* _this_indicator_ =
              GetIndicatorFromTokens(t_dbglogger_, cr_watch_, tokens_, kPriceTypeMktSizeWPrice);
          if (p_regime_model_math_ == nullptr) {
            p_regime_model_math_ = new RegimeModelAggregator(t_dbglogger_, cr_watch_, _model_filename_);
          }

          if (p_regime_model_math_ != nullptr && _this_indicator_ != nullptr) {
            p_regime_model_math_->SetRegimeIndicator(_this_indicator_);
          }
        } else {
          if (p_regime_model_math_ == nullptr) {
            return nullptr;
          }
        }
      }
    }
  }
  return p_regime_model_math_;
}

BaseModelMath* ModelCreator::CreateModelMathComponent(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                      const std::string& _model_filename_,
                                                      std::vector<double>* _p_scaling_model_vec_, int trading_start_mfm,
                                                      int trading_end_mfm, int runtime_id) {
  std::string map_key_ = _model_filename_;
  std::ostringstream temp_oss_;
  temp_oss_ << _model_filename_ << " ";
  bool to_scale_ = false;
  if (_p_scaling_model_vec_ != nullptr && _p_scaling_model_vec_->size() > 0) {
    for (auto i = 0u; i < _p_scaling_model_vec_->size(); i++) {
      double t_factor_ = (*_p_scaling_model_vec_)[i];
      temp_oss_ << t_factor_ << " ";
      if (t_factor_ > 0.0 && !HFSAT::MathUtils::DblPxCompare(t_factor_, 1.0, 0.0005)) {
        to_scale_ = true;
      }
    }
    if (to_scale_) {
      map_key_ = temp_oss_.str();
    }
  }

  if (modelfilename_basemodelmath_map_.find(map_key_) == modelfilename_basemodelmath_map_.end()) {
    // for models like /home/dvctrader/modelling/models/HHI_0/HKT_1300-HKT_1615/model_12_3
    BaseModelMath* p_this_model_math_ =
        CreateRegimeModelMath(t_dbglogger_, cr_watch_, _model_filename_, _p_scaling_model_vec_, trading_start_mfm,
                              trading_end_mfm, runtime_id);

    if (p_this_model_math_ == nullptr) {
      // normal models
      double current_model_scaling_factor_ = 1.0;
      unsigned int current_model_num_ = 0;
      std::ifstream model_infile_;
      model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
      if (model_infile_.is_open()) {
        const unsigned int kModelLineBufferLen = 1024;
        char readline_buffer_[kModelLineBufferLen];
        bzero(readline_buffer_, kModelLineBufferLen);
        ModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;
        SecurityMarketView* dep_market_view__ = nullptr;
        PriceType_t dep_base_pricetype_ = kPriceTypeMidprice;
        std::vector<std::string> core_shortcodes_;
        SignalAlgo_t signal_algo_ = kSignalAlgoLinear;
        double model_stdev_ = 0.0;
        core_shortcodes_.clear();
        while (model_infile_.good()) {
          bzero(readline_buffer_, kModelLineBufferLen);
          model_infile_.getline(readline_buffer_, kModelLineBufferLen);
          if (model_infile_.gcount() > 0) {
            t_dbglogger_ << readline_buffer_ << '\n';
            t_dbglogger_.CheckToFlushBuffer();  // added logging to see later what was running
            PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
            const std::vector<const char*>& tokens_ = st_.GetTokens();

            if (tokens_.size() < 1) {  // skip empty lines
              continue;
            }

            switch (current_model_creation_phase_) {
              case kModelCreationPhaseInit:
                if (strcmp(tokens_[0], "MODELINIT") == 0) {  // MODELINIT
                  if (tokens_.size() < 2) {
                    ExitVerbose(kModelCreationModelInitLineLessArgs);
                  }
                  if (strcmp(tokens_[1], "DEPBASE") == 0) {  // MODELINIT DEPBASE ZN0 OfflineMixMMS
                    if (tokens_.size() < 4) {
                      ExitVerbose(kModelCreationDepBaseLineLessArgs);
                    }
                    dep_market_view__ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens_[2]);
                    dep_market_view__->SetBasePriceType(dep_base_pricetype_);
                    std::string this_shortcode_ = std::string(tokens_[2]);
                    GetCoreShortcodes(this_shortcode_, core_shortcodes_);
                    dep_base_pricetype_ = StringToPriceType_t((std::string)tokens_[3]);
                    dep_market_view__->SetBasePriceType(dep_base_pricetype_);
                  }
                  current_model_creation_phase_ = kModelCreationPhasePreModel;
                }

                break;
              case kModelCreationPhasePreModel:
                // MODELMATH LINEAR CHANGE
                if (strcmp(tokens_[0], "MODELMATH") == 0) {  // MODELMATH LINEAR RETURNS
                  if (tokens_.size() < 2) {
                    ExitVerbose(kModelCreationModelMathLineLessArgs);
                  }
                  signal_algo_ = GetSignalAlgo(tokens_[1]);
                  switch (signal_algo_) {
                    case kSignalAlgoLinear:  // linearmodel with tgt a derivative of dependant price
                    {
                      // MODELMATH LINEAR CHANGE
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationLinearModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new LinearModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                    dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoOnlineLinear: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationLinearModelMathLineLessArgs);
                      }

                      p_this_model_math_ =
                          new OnlineLinearModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                          dep_base_pricetype_, false, "LINEAR");
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);

                    } break;

                    case kSignalAlgoNoDepLinear:  // linearmodel without a dependant
                    {
                      // MODELMATH NODEPLINEAR
                      p_this_model_math_ =
                          new NoDependantLinearModelAggregator(t_dbglogger_, cr_watch_, _model_filename_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;
                    case kSignalAlgoClassifier: {  // classification based models
                    } break;
                    case kSignalAlgoCART: {  // regression trees
                    } break;
                    case kSignalAlgoNeuralNetwork: {  // NeuralNetwork
                    } break;
                    case kSignalAlgoLogistic: {  // Logistic Regression
                      // MODELMATH LOGISTIC CHANGE
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationLogisticModelMathLineLessArgs);
                      }
                      // probably not needed since here we aren't predicting any quantity.
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new LogisticModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                      dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;
                    case kSignalAlgoNonLinear: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationNonLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationNonLinearComponentLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new NonLinearModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                       dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);

                    } break;
                    case kSignalAlgoSIGLR: {  // SIGLR
                      // MODELMATH SIGLR CHANGE
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationLogisticModelMathLineLessArgs);
                      }
                      // probably not needed since here we aren't predicting any quantity.
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new SIGLRModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                   dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoOnlineSIGLR: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationLogisticModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new OnlineLinearModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                          dep_base_pricetype_, is_returns_based_, "SIGLR");
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoRandomForest: {  // MODELMATH RANDOMFOREST CHANGE
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationLogisticModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new RandomForestModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                          dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;
                    case kSignalAlgoTreeBoosting: {  // MODELMATH TREEBOOSTING CHANGE
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationLogisticModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new TreeBoostingModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                          dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;
                    case kSignalAlgoSelective: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationSelectiveModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new SelectiveModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                       dep_base_pricetype_, is_returns_based_, core_shortcodes_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;
                    case kSignalAlgoBoosting: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new BoostingModelAggregator(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                      dep_base_pricetype_, is_returns_based_, core_shortcodes_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;
                    case kSignalAlgoSelectiveNew: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationSelectiveModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new SelectiveModelAggregatorNew(t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__,
                                                          dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoSelectiveContinuous: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationSelectiveModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ = new SelectiveContinuousModelAggregator(
                          t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__, dep_base_pricetype_,
                          is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoOnlineSelectiveNew: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationSelectiveModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ = new OnlineSelectiveModelAggregatorNew(
                          t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__, dep_base_pricetype_,
                          is_returns_based_, "LINEAR");
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoOnlineSelectiveSiglr: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationSelectiveModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ = new OnlineSelectiveModelAggregatorNew(
                          t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__, dep_base_pricetype_,
                          is_returns_based_, "SIGLR");
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoSelectiveSiglr: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 3) {
                        ExitVerbose(kModelCreationSelectiveModelMathLineLessArgs);
                      }
                      bool is_returns_based_ = (strcmp(tokens_[2], "RETURNS") == 0);
                      p_this_model_math_ =
                          new SelectiveModelAggregatorSiglr(t_dbglogger_, cr_watch_, _model_filename_,
                                                            *dep_market_view__, dep_base_pricetype_, is_returns_based_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);
                    } break;

                    case kSignalAlgoGradBoostingClassifier: {
                      if (dep_market_view__ == nullptr) {
                        ExitVerbose(kModelCreationNonLinearModelMathNeedsNonNullDependant);
                      }
                      if (tokens_.size() < 2) {
                        ExitVerbose(kModelCreationSelectiveModelMathLineLessArgs);
                      }
                      p_this_model_math_ = new GradientBoostingClassifierModelAggregator(
                          t_dbglogger_, cr_watch_, _model_filename_, *dep_market_view__, dep_base_pricetype_);
                      p_this_model_math_->SetTradingTimeAndQueryId(trading_start_mfm, trading_end_mfm, runtime_id);

                    } break;

                    default: {
                      if (p_this_model_math_ == nullptr) {
                        ExitVerbose(kModelCreationNeedNonNullBaseModelMath);
                      }
                    }
                  }
                  if (p_this_model_math_ == nullptr) {
                    ExitVerbose(kModelCreationNeedNonNullBaseModelMath);
                  }
                  current_model_creation_phase_ = kModelCreationPhasePostModel;
                }
                break;

              case kModelCreationPhasePostModel: {
                if (strcmp(tokens_[0], "MODELINFO") == 0) {
                  if (tokens_.size() < 2) {
                    ExitVerbose(kModelCreationModelInfoLessArgs);
                  }
                  model_stdev_ = atof(tokens_[1]);
                  p_this_model_math_->set_model_stdev(model_stdev_);
                  current_model_creation_phase_ = kModelCreationPhasePostModelInfo;
                  break;
                }
                // break only if we get MODELINFO part, otherwise continue
              }

              case kModelCreationPhasePostModelInfo:

                if (strcmp(tokens_[0], "REGIMEINDICATOR") == 0 &&
                    ((signal_algo_ == kSignalAlgoSelectiveNew) || (signal_algo_ == kSignalAlgoSelectiveSiglr) ||
                     (signal_algo_ == kSignalAlgoOnlineSelectiveNew) ||
                     (signal_algo_ == kSignalAlgoSelectiveContinuous) ||
                     (signal_algo_ == kSignalAlgoOnlineSelectiveSiglr))) {
                  CommonIndicator* _this_indicator_ =
                      GetIndicatorFromTokens(t_dbglogger_, cr_watch_, tokens_, dep_base_pricetype_);
                  if ((signal_algo_ == kSignalAlgoSelectiveNew) || (signal_algo_ == kSignalAlgoOnlineSelectiveNew) ||
                      (signal_algo_ == kSignalAlgoOnlineSelectiveSiglr) ||
                      (signal_algo_ == kSignalAlgoSelectiveContinuous))
                    p_this_model_math_->SetRegimeIndicator(_this_indicator_);
                  else
                    p_this_model_math_->SetRegimeIndicator(_this_indicator_, true);
                }
                if (strcmp(tokens_[0], "USE_MID_AS_BASE") == 0) {
                  DiffPriceType* _this_indicator_ = DiffPriceType::GetUniqueInstance(
                      t_dbglogger_, cr_watch_,
                      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(dep_market_view__->shortcode())),
                      kPriceTypeMidprice, dep_base_pricetype_);
                  p_this_model_math_->SetDiffPriceIndicator(_this_indicator_);
                  if (tokens_.size() >= 3) {
                    RegimeSlowStdev* _this_indicator_2_ = RegimeSlowStdev::GetUniqueInstance(
                        t_dbglogger_, cr_watch_,
                        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(dep_market_view__->shortcode())),
                        atof(tokens_[1]), atof(tokens_[2]));
                    p_this_model_math_->SetRegimeIndicatorForDat(_this_indicator_2_);
                  }
                }
                if (strcmp(tokens_[0], "CREGIMECUT") == 0 && signal_algo_ == kSignalAlgoSelectiveContinuous) {
                  ((SelectiveContinuousModelAggregator*)p_this_model_math_)->SetNumRegimes(stoul(tokens_[1]));
                  std::vector<double> regimes_cutoff_vec_;
                  for (auto i = 0u; i < stoul(tokens_[1]); i++) {
                    regimes_cutoff_vec_.push_back(stod(tokens_[i + 2]));
                  }
                  ((SelectiveContinuousModelAggregator*)p_this_model_math_)->SetRegimesCutoff(regimes_cutoff_vec_);
                }
                if (strcmp(tokens_[0], "USE_IMPLIED_PRICE_FOR_NIK") == 0 && dep_market_view__->shortcode() == "NK_0") {
                  BaseImpliedPrice* _this_indicator_ = BaseImpliedPrice::GetUniqueInstance(
                      t_dbglogger_, cr_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NK_0")),
                      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NKM_0")), dep_base_pricetype_);

                  p_this_model_math_->SetImpliedPriceIndicator(_this_indicator_);
                }
                if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
                  current_model_scaling_factor_ = (to_scale_ && _p_scaling_model_vec_->size() > current_model_num_)
                                                      ? (*_p_scaling_model_vec_)[current_model_num_]
                                                      : 1.0;
                  current_model_scaling_factor_ =
                      current_model_scaling_factor_ > 0.0 ? current_model_scaling_factor_ : 1.0;
                  current_model_scaling_factor_ = std::min(
                      4.0,
                      std::max(current_model_scaling_factor_, 0.25));  // for sanity not scaling more than a factor of 4
                  current_model_num_++;
                  t_dbglogger_ << "Scaling model_num: " << current_model_num_ << " in modelfile: " << _model_filename_
                               << " with factor: " << current_model_scaling_factor_ << '\n';
                  t_dbglogger_.CheckToFlushBuffer();

                  current_model_creation_phase_ = kModelCreationPhaseIndicatorStarted;
                  if (signal_algo_ == kSignalAlgoSelective || signal_algo_ == kSignalAlgoSelectiveNew ||
                      signal_algo_ == kSignalAlgoSelectiveSiglr || signal_algo_ == kSignalAlgoOnlineSelectiveNew ||
                      signal_algo_ == kSignalAlgoOnlineSelectiveSiglr ||
                      signal_algo_ == kSignalAlgoSelectiveContinuous) {
                    p_this_model_math_->StartCreation();
                  }
                  if (signal_algo_ == kSignalAlgoBoosting) {
                    if (tokens_.size() <= 1) {
                      std::cerr << "Not enough arguments to initialise the ensemble\n";
                      exit(1);
                    }
                    p_this_model_math_->StartCreation(atof(tokens_[1]));
                  }
                  if (signal_algo_ == kSignalAlgoGradBoostingClassifier) {
                    if (tokens_.size() <= 1) {
                      std::cerr << "Not enough arguments to initialise the tree ensemble\n";
                      exit(1);
                    }
                    p_this_model_math_->StartCreation(atof(tokens_[1]));
                  }
                } else {
                  if (strcmp(tokens_[0], "MODELARGS") == 0) {
                    // common modelmath arguments like :
                    // MODELARGS number_of_layers_in_neural_network
                    // MODELARGS num_trees max_nodes_per_tree   // random forest
                    p_this_model_math_->InterpretModelParameters(tokens_);
                  }
                }
                break;
              case kModelCreationPhaseIndicatorStarted: {
                std::vector<std::string> this_indicator_tokens_vec_;
                for (unsigned index_ = 0; index_ < tokens_.size(); index_++) {
                  this_indicator_tokens_vec_.push_back(tokens_[index_]);
                }
                indicator_token_strings_vec_.push_back(this_indicator_tokens_vec_);
                signal_algo_vec_.push_back(signal_algo_);
                model_math_vec_.push_back(p_this_model_math_);
                dep_market_view_vec_.push_back(dep_market_view__);
                baseprice_vec_.push_back(dep_base_pricetype_);
                model_entry_type_vec_.push_back(-1);
                model_scaling_factor_vec_.push_back(current_model_scaling_factor_);

                if (strcmp(tokens_[0], "INDICATOREND") == 0) {
                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 4;
                  if (signal_algo_ == kSignalAlgoRandomForest || signal_algo_ == kSignalAlgoTreeBoosting ||
                      signal_algo_ == kSignalAlgoGradBoostingClassifier) {
                    model_entry_type_vec_[model_entry_type_vec_.size() - 1] = -1;
                    current_model_creation_phase_ = kModelTreeCreationPhaseStarted;
                  } else {
                    current_model_creation_phase_ = kModelCreationPhaseIndicatorEnded;
                    // called for modelmath objects that might need a signal that all inputs have been registered and
                    // linkages can be finalized now.
                  }
                  model_scaling_factor_vec_[model_entry_type_vec_.size() - 1] = current_model_scaling_factor_;
                }

                if (strcmp(tokens_[0], "INDICATORINTERMEDIATE") == 0) {
                  current_model_scaling_factor_ = (to_scale_ && _p_scaling_model_vec_->size() > current_model_num_)
                                                      ? (*_p_scaling_model_vec_)[current_model_num_]
                                                      : 1.0;
                  current_model_scaling_factor_ =
                      current_model_scaling_factor_ > 0.0 ? current_model_scaling_factor_ : 1.0;
                  current_model_scaling_factor_ = std::min(
                      4.0,
                      std::max(current_model_scaling_factor_, 0.25));  // for sanity not scaling more than a factor of 4
                  current_model_num_++;
                  t_dbglogger_ << "Scaling model_num: " << current_model_num_ << " in modelfile: " << _model_filename_
                               << " with factor: " << current_model_scaling_factor_ << '\n';
                  t_dbglogger_.CheckToFlushBuffer();

                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 3;
                  model_scaling_factor_vec_[model_entry_type_vec_.size() - 1] = current_model_scaling_factor_;
                } else {
                  if (strcmp(tokens_[0], "INDICATOR") == 0) {
                    model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 1;
                    model_scaling_factor_vec_[model_entry_type_vec_.size() - 1] = current_model_scaling_factor_;
                  } else if (strcmp(tokens_[0], "NONLINEARCOMPONENT") == 0) {
                    model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 2;
                    model_scaling_factor_vec_[model_entry_type_vec_.size() - 1] = current_model_scaling_factor_;
                    // this one is only meant for the non-linear components
                  } else if (strcmp(tokens_[0], "INTERCEPT") == 0) {
                    model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 5;
                    model_scaling_factor_vec_[model_entry_type_vec_.size() - 1] = current_model_scaling_factor_;
                  }
                }
              } break;
              case kModelTreeCreationPhaseStarted: {
                std::vector<std::string> this_indicator_tokens_vec_;
                for (unsigned index_ = 0; index_ < tokens_.size(); index_++) {
                  this_indicator_tokens_vec_.push_back(tokens_[index_]);
                }
                indicator_token_strings_vec_.push_back(this_indicator_tokens_vec_);
                signal_algo_vec_.push_back(signal_algo_);
                model_math_vec_.push_back(p_this_model_math_);
                dep_market_view_vec_.push_back(dep_market_view__);
                baseprice_vec_.push_back(dep_base_pricetype_);
                model_entry_type_vec_.push_back(-1);
                model_scaling_factor_vec_.push_back(1);

                if (strcmp(tokens_[0], "RANDOMFORESTEND") == 0 || strcmp(tokens_[0], "ENSEMBLEEND") == 0) {
                  current_model_creation_phase_ = kModelCreationPhaseIndicatorEnded;
                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 4;
                } else if (strcmp(tokens_[0], "TREESTART") == 0) {
                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 6;
                } else if (strcmp(tokens_[0], "TREELINE") == 0) {
                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 7;
                } else if (strcmp(tokens_[0], "SIDE") == 0) {
                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 8;
                } else if (strcmp(tokens_[0], "TREE") == 0) {
                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 9;
                } else if (strcmp(tokens_[0], "LEFTCHILD") == 0 || strcmp(tokens_[0], "RIGHTCHILD") == 0 ||
                           strcmp(tokens_[0], "FEATURE") == 0 || strcmp(tokens_[0], "THRESHOLD") == 0 ||
                           strcmp(tokens_[0], "VALUE") == 0) {
                  model_entry_type_vec_[model_entry_type_vec_.size() - 1] = 10;
                }

              } break;
              case kModelCreationPhaseIndicatorEnded: {
              }
              default:
                break;
            }
          }
        }
        model_infile_.close();
      }
    }

    // store in map so that for each model file only one modelmath object is created.
    modelfilename_basemodelmath_map_[map_key_] = p_this_model_math_;
  }
  return modelfilename_basemodelmath_map_[map_key_];
}

void ModelCreator::CreateIndicatorInstances(DebugLogger& t_dbglogger_, const Watch& cr_watch_) {
  //    static int count_call_ = 0;
  //    count_call_++;
  indicator_order_vec_ = GetIndicatorOrder();

  std::vector<bool> instance_created_;
  instance_created_.resize(indicator_token_strings_vec_.size(), false);
  std::vector<CommonIndicator*> this_indicator_vec_;
  this_indicator_vec_.resize(indicator_token_strings_vec_.size(), nullptr);
  for (unsigned index_ = 0; index_ < indicator_order_vec_.size() + 1; index_++) {
    // assumption, the tokens would be of the form INDICATOR _value_ Indicator
    std::vector<unsigned> indicator_index_vec_;
    if (index_ == indicator_order_vec_.size()) {
      // Having extra case if there is no entry for indicator in IndicatorOrder file
      for (unsigned i = 0; i < instance_created_.size(); i++) {
        if (!instance_created_[i] && (model_entry_type_vec_[i] == 1)) {
          //		    std::cerr << "KP:Missing in order vector " << i << " " << indicator_token_strings_vec_[i][2]
          //<<
          //"\n";
          indicator_index_vec_.push_back(i);
        }
      }
    } else {
      indicator_index_vec_ =
          VectorUtils::LinearSearchValueIdxVec(indicator_token_strings_vec_, indicator_order_vec_[index_]);
    }

    for (unsigned idx_ = 0; idx_ < indicator_index_vec_.size(); idx_++) {
      unsigned indicator_index_ = indicator_index_vec_[idx_];
      //	    std::cerr << "KP " << count_call_ << " " << index_ << " " << idx_ << " " << indicator_index_ << " "
      //<< model_entry_type_vec_[indicator_index_] << "\n";
      if (indicator_index_ == indicator_token_strings_vec_.size()) {
        // No indicator of this name
      } else {
        std::vector<const char*> tokens_;
        for (unsigned i = 0; i < indicator_token_strings_vec_[indicator_index_].size(); i++) {
          tokens_.push_back(indicator_token_strings_vec_[indicator_index_][i].c_str());
        }
        if (tokens_.size() < 3 && (model_entry_type_vec_[indicator_index_] == 1)) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        if (model_entry_type_vec_[indicator_index_] == 2 || model_entry_type_vec_[indicator_index_] == -1) {
          continue;
        }

        PriceType_t dep_base_pricetype_ = baseprice_vec_[indicator_index_];
        CommonIndicator* _this_indicator_ =
            GetIndicatorFromTokens(t_dbglogger_, cr_watch_, tokens_, dep_base_pricetype_);
        this_indicator_vec_[indicator_index_] = _this_indicator_;

        std::vector<std::string> this_shortcodes_affecting_;
        std::vector<std::string> this_ors_source_needed_vec_;
        (CollectShortCodeFunc(tokens_[2]))(this_shortcodes_affecting_, this_ors_source_needed_vec_, tokens_);

        for (unsigned i = 0; i < this_shortcodes_affecting_.size(); i++) {
          SecurityMarketView* this_smv_ =
              ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(this_shortcodes_affecting_[i]);
          this_smv_->subscribe_MktStatus(_this_indicator_);
        }
      }
      instance_created_[indicator_index_] = true;
    }
  }

  unsigned int tree_count_ = 0;
  unsigned int tree_line_count_ = 0;
  unsigned int boosting_level_ = 0;
  unsigned int tree_class_ = 0;
  unsigned int side_ = 0;

  for (unsigned indicator_index_ = 0; indicator_index_ < indicator_token_strings_vec_.size(); indicator_index_++) {
    // std::cout << " total tokens : " << indicator_token_strings_vec_[indicator_index_].size() << " total indicators: "
    // << indicator_token_strings_vec_.size() << std::endl;
    // std::cout << std::endl ;
    // for ( unsigned i =0 ; i < indicator_token_strings_vec_[indicator_index_].size(); i++ ) { std::cout <<
    // indicator_token_strings_vec_[indicator_index_][i] << " " ; }
    // std::cout << std::endl ;
    BaseModelMath* p_this_model_math_ = model_math_vec_[indicator_index_];
    SignalAlgo_t signal_algo_ = signal_algo_vec_[indicator_index_];
    std::vector<const char*> tokens_;
    for (unsigned i = 0; i < indicator_token_strings_vec_[indicator_index_].size(); i++) {
      tokens_.push_back(indicator_token_strings_vec_[indicator_index_][i].c_str());
    }
    SecurityMarketView* dep_market_view__ = dep_market_view_vec_[indicator_index_];
    double current_model_scaling_factor_ = model_scaling_factor_vec_[indicator_index_];

    if (model_entry_type_vec_[indicator_index_] == 1) {
      if (signal_algo_ == kSignalAlgoRandomForest || signal_algo_ == kSignalAlgoTreeBoosting) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];

        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_readiness_required_);
        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoLinear || signal_algo_ == kSignalAlgoGradBoostingClassifier) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        // INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
        double _this_weight_ = atof(tokens_[1]);
        _this_weight_ *= current_model_scaling_factor_;  // scaling model to suits params

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];

        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (std::isnan(_this_weight_)) {
            ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_weight_, _this_readiness_required_);

        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoNonLinear) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationNonLinearComponentLineLessArgs);
        }
        //  INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
        double _this_weight_ = atof(tokens_[1]);

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];
        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }

          //  expecting weigth to be 1.0
          if (std::isnan(_this_weight_)) {
            ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight in NAN\n");
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_weight_, _this_readiness_required_);

        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoOnlineLinear) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationNonLinearComponentLineLessArgs);
        }

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];
        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }

          p_this_model_math_->AddIndicator(_this_indicator_, 1, _this_readiness_required_);

        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      }

      else if (signal_algo_ == kSignalAlgoLogistic) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        // INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
        double _this_weight_decrease_ = 0.0;
        double _this_weight_nochange_ = 0.0;
        double _this_weight_increase_ = 0.0;
        std::istringstream iss(tokens_[1]);
        std::string token;
        getline(iss, token, ':');
        _this_weight_decrease_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_weight_nochange_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_weight_increase_ = atof(token.c_str());

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];

        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (std::isnan(_this_weight_decrease_) || std::isnan(_this_weight_nochange_) ||
              std::isnan(_this_weight_increase_)) {
            ExitVerbose(kModelCreationIndicatorWeightNAN,
                        "One of decrease, nochange or increase is NAN in logistic model\n");
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_weight_decrease_, _this_weight_nochange_,
                                           _this_weight_increase_, _this_readiness_required_);

        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoSIGLR) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        // INDICATOR _this_alpha_:_this_beta_  _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
        double _this_alpha_ = 0.0;
        double _this_beta_ = 0.0;
        std::istringstream iss(tokens_[1]);
        std::string token;
        getline(iss, token, ':');
        _this_alpha_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_beta_ = atof(token.c_str());

        _this_beta_ *= current_model_scaling_factor_;  // scaling model to suits params
        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];

        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (std::isnan(_this_alpha_) || std::isnan(_this_beta_)) {
            ExitVerbose(kModelCreationIndicatorWeightNAN, "alpha or beta is NAN in SIGLR\n");
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_alpha_, _this_beta_, _this_readiness_required_);

        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoOnlineSIGLR) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        double _this_alpha_ = 0.0;
        double _this_beta_ = 0.0;
        std::istringstream iss(tokens_[1]);
        std::string token;
        getline(iss, token, ':');
        _this_alpha_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_beta_ = atof(token.c_str());

        _this_beta_ *= current_model_scaling_factor_;
        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];

        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (indicator_index_ == 0) {
            p_this_model_math_->AddIndicator(_this_indicator_, 1.00, 1.00, _this_readiness_required_);
          } else {
            if (std::isnan(_this_alpha_)) {
              ExitVerbose(kModelCreationIndicatorWeightNAN, "OnlineSIGLR alpha is NAN\n");
            }
            p_this_model_math_->AddIndicator(_this_indicator_, _this_alpha_, 1.00, _this_readiness_required_);
          }

        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoSelective || signal_algo_ == kSignalAlgoSelectiveNew ||
                 signal_algo_ == kSignalAlgoBoosting || signal_algo_ == kSignalAlgoSelectiveContinuous) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        double _this_weight_ = atof(tokens_[1]);

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];
        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (signal_algo_ == kSignalAlgoSelectiveNew || signal_algo_ == kSignalAlgoSelectiveContinuous) {
            if (p_this_model_math_->IsNULLRegime()) {
              ExitVerbose(kModelCreationSelectiveModelMathNoIndicatorForRegime);
            }
            _this_weight_ *= current_model_scaling_factor_;  // scaling model to suits params
          }
          if (std::isnan(_this_weight_)) {
            ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_weight_, _this_readiness_required_);
        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoOnlineSelectiveNew) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }

        double _this_weight_ = 0;
        unsigned int _this_ind_ = 0;
        std::istringstream iss(tokens_[1]);
        std::string token;

        std::string token_1;

        getline(iss, token, ':');
        _this_weight_ = atof(token.c_str());

        token_1 = token;
        while (getline(iss, token, ':')) {
          token_1 = token;
        }
        _this_ind_ = atoi(token_1.c_str());

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];
        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (signal_algo_ == kSignalAlgoOnlineSelectiveNew) {
            if (p_this_model_math_->IsNULLRegime()) {
              ExitVerbose(kModelCreationSelectiveModelMathNoIndicatorForRegime);
            }
            _this_weight_ *= current_model_scaling_factor_;  // scaling model to suits params
          }

          p_this_model_math_->AddIndicator(_this_indicator_, 1.00, _this_ind_, _this_readiness_required_);
        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoOnlineSelectiveSiglr) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }

        double _this_alpha_ = 0.0;
        double _this_beta_ = 0.0;
        std::istringstream iss(tokens_[1]);
        std::string token;
        getline(iss, token, ':');
        _this_alpha_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_beta_ = atof(token.c_str());

        _this_beta_ *= current_model_scaling_factor_;
        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];

        std::string token_1 = token;
        while (getline(iss, token, ':')) {
          token_1 = token;
        }

        unsigned int _this_ind_ = 0;
        _this_ind_ = atoi(token_1.c_str());

        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (signal_algo_ == kSignalAlgoOnlineSelectiveSiglr) {
            if (p_this_model_math_->IsNULLRegime()) {
              ExitVerbose(kModelCreationSelectiveModelMathNoIndicatorForRegime);
            }
            _this_beta_ *= current_model_scaling_factor_;  // scaling model to suits params
          }
          if (std::isnan(_this_alpha_)) {
            ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_alpha_, 1.00, _this_ind_, _this_readiness_required_);
        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      } else if (signal_algo_ == kSignalAlgoSelectiveSiglr) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        double _this_alpha_ = 0.0;
        double _this_beta_ = 0.0;
        std::istringstream iss(tokens_[1]);
        std::string token;
        getline(iss, token, ':');
        _this_alpha_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_beta_ = atof(token.c_str());
        _this_beta_ *= current_model_scaling_factor_;  // scaling model to suits params

        CommonIndicator* _this_indicator_ = this_indicator_vec_[indicator_index_];

        if (_this_indicator_ != nullptr) {
          bool _this_readiness_required_ = false;
          if (dep_market_view__ != nullptr) {
            _this_readiness_required_ = GetReadinessRequired(dep_market_view__->shortcode(), tokens_);
          }
          if (p_this_model_math_->IsNULLRegime()) {
            ExitVerbose(kModelCreationSelectiveModelMathNoIndicatorForRegime);
          }
          if (std::isnan(_this_alpha_) || std::isnan(_this_beta_)) {
            ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
          }
          p_this_model_math_->AddIndicator(_this_indicator_, _this_alpha_, _this_beta_, _this_readiness_required_);
        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
        }
      }
    } else if (model_entry_type_vec_[indicator_index_] == 2) {
      if (signal_algo_ == kSignalAlgoNonLinear) {
        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationNonLinearComponentLineLessArgs);
        }
        // INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
        double _this_weight_ = atof(tokens_[1]);

        NonLinearWrapper* _this_non_linear_wrapper_ =
            GetNonLinearComponentFromTokens(p_this_model_math_, _this_weight_, t_dbglogger_, cr_watch_, tokens_);

        if (_this_non_linear_wrapper_ != nullptr) {
          p_this_model_math_->AddNonLinearComponent(_this_non_linear_wrapper_);

        } else {
          ExitVerbose(kModelCreationNonLinearComponentLineNull, tokens_[2]);
        }
      }
    } else if (model_entry_type_vec_[indicator_index_] == 3) {
      // IndicatorIntermediate

      if (signal_algo_ == kSignalAlgoSelective || signal_algo_ == kSignalAlgoSelectiveNew ||
          signal_algo_ == kSignalAlgoSelectiveSiglr || signal_algo_ == kSignalAlgoOnlineSelectiveNew ||
          signal_algo_ == kSignalAlgoOnlineSelectiveSiglr || signal_algo_ == kSignalAlgoSelectiveContinuous) {
        p_this_model_math_->FinishCreation(false);
        p_this_model_math_->StartCreation();
      }
      if (signal_algo_ == kSignalAlgoBoosting) {
        if (tokens_.size() <= 1) {
          std::cerr << "Not enough arguments to initialise the ensemble\n";
          exit(1);
        }
        p_this_model_math_->FinishCreation(false);
        p_this_model_math_->StartCreation(atof(tokens_[1]));
      }
    } else if (model_entry_type_vec_[indicator_index_] == 4) {
      p_this_model_math_->set_basepx_pxtype();
      if (signal_algo_ == kSignalAlgoSelective || signal_algo_ == kSignalAlgoSelectiveNew ||
          signal_algo_ == kSignalAlgoSelectiveSiglr || signal_algo_ == kSignalAlgoBoosting ||
          signal_algo_ == kSignalAlgoOnlineSelectiveNew || signal_algo_ == kSignalAlgoOnlineSelectiveSiglr ||
          signal_algo_ == kSignalAlgoSelectiveContinuous) {
        p_this_model_math_->FinishCreation(true);
      } else {
        p_this_model_math_->FinishCreation();
      }
      tree_count_ = 0;
      tree_line_count_ = 0;
    } else if (model_entry_type_vec_[indicator_index_] == 5) {
      if (signal_algo_ == kSignalAlgoLogistic) {
        if (tokens_.size() < 2) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        // INTERCEPT Coeff1:Coeff2:Coeff3
        double _this_weight_decrease_ = 0.0;
        double _this_weight_nochange_ = 0.0;
        double _this_weight_increase_ = 0.0;

        std::istringstream iss(tokens_[1]);
        std::string token;
        getline(iss, token, ':');
        _this_weight_decrease_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_weight_nochange_ = atof(token.c_str());
        getline(iss, token, ':');
        _this_weight_increase_ = atof(token.c_str());

        p_this_model_math_->AddIntercept(_this_weight_decrease_, _this_weight_nochange_, _this_weight_increase_);
      }

      else if (signal_algo_ == kSignalAlgoNonLinear) {
        if (tokens_.size() < 2) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        // INTERCEPT Coeff1:Coeff2:Coeff3
        double _this_weight_constant_ = atof(tokens_[1]);
        p_this_model_math_->AddIntercept(_this_weight_constant_);

      } else if (signal_algo_ == kSignalAlgoLinear) {
        if (tokens_.size() < 2) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }
        // INTERCEPT Coeff1
        double _this_weight_constant_ = atof(tokens_[1]);
        p_this_model_math_->AddIntercept(_this_weight_constant_);
      }

    } else if (model_entry_type_vec_[indicator_index_] == 6) {
      p_this_model_math_->AddTree(atof(tokens_[1]));  // change it in the random forest as well
      tree_count_++;
      tree_line_count_ = 0;
    } else if (model_entry_type_vec_[indicator_index_] == 7) {
      unsigned int left_daughter_ = atoi(tokens_[1]);
      unsigned int right_daughter_ = atoi(tokens_[2]);
      unsigned int split_indicator_index_ = atoi(tokens_[3]);
      double split_indicator_value_ = atof(tokens_[4]);
      bool is_leaf_ = true;
      if (strcmp(tokens_[5], "N") == 0) {
        is_leaf_ = false;
      }
      double prediction_ = atof(tokens_[6]);
      p_this_model_math_->AddTreeLine(tree_count_ - 1, tree_line_count_, split_indicator_index_, left_daughter_,
                                      right_daughter_, split_indicator_value_, prediction_, is_leaf_);
      tree_line_count_++;
    } else if (model_entry_type_vec_[indicator_index_] == 8) {
      if (tokens_.size() <= 1) {
        std::cerr << "Cancellation side not specified\n";
        exit(1);
      }
      if (strcmp(tokens_[1], "BID") == 0)
        side_ = 1;
      else if (strcmp(tokens_[1], "ASK") == 0)
        side_ = 2;
    } else if (model_entry_type_vec_[indicator_index_] == 9) {
      if (tokens_.size() <= 2) {
        std::cerr << "Tree boosting level and class not specified\n";
        exit(1);
      }
      boosting_level_ = atoi(tokens_[1]);
      tree_class_ = atoi(tokens_[2]);
    } else if (model_entry_type_vec_[indicator_index_] == 10) {
      if (strcmp(tokens_[0], "LEFTCHILD") == 0) {
        std::vector<int> left_child_;
        for (unsigned indx = 1; indx < tokens_.size(); indx++) left_child_.push_back(atoi(tokens_[indx]));
        p_this_model_math_->AddLeftChild(boosting_level_, tree_class_, left_child_, side_);
      } else if (strcmp(tokens_[0], "RIGHTCHILD") == 0) {
        std::vector<int> right_child_;
        for (unsigned indx = 1; indx < tokens_.size(); indx++) right_child_.push_back(atoi(tokens_[indx]));
        p_this_model_math_->AddRightChild(boosting_level_, tree_class_, right_child_, side_);
      } else if (strcmp(tokens_[0], "THRESHOLD") == 0) {
        std::vector<double> threshold;
        for (unsigned indx = 1; indx < tokens_.size(); indx++) threshold.push_back(atof(tokens_[indx]));
        p_this_model_math_->AddThreshold(boosting_level_, tree_class_, threshold, side_);
      } else if (strcmp(tokens_[0], "VALUE") == 0) {
        std::vector<double> value;
        for (unsigned indx = 1; indx < tokens_.size(); indx++) value.push_back(atof(tokens_[indx]));
        p_this_model_math_->AddValue(boosting_level_, tree_class_, value, side_);
      } else if (strcmp(tokens_[0], "FEATURE") == 0) {
        std::vector<int> feature;
        for (unsigned indx = 1; indx < tokens_.size(); indx++) feature.push_back(atoi(tokens_[indx]));
        p_this_model_math_->AddFeature(boosting_level_, tree_class_, feature, side_);
      }
    }
  }
}

bool ModelCreator::NeedsAflashFeed() {
  const std::string af_indicators_[] = {"EventBiasOfflineIndicator"};
  const int af_indc_size_ = 1;
  bool af_found_ = false;
  for (unsigned index_ = 0; index_ < indicator_token_strings_vec_.size(); index_++) {
    for (unsigned j = 0; j < af_indc_size_; j++) {
      if (indicator_token_strings_vec_[index_].size() > 2 &&
          indicator_token_strings_vec_[index_][2] == af_indicators_[j]) {
        af_found_ = true;
        break;
      }
    }
    if (af_found_) {
      break;
    }
  }
  return af_found_;
}

CommonIndicator* ModelCreator::GetIndicatorFromTokens(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                      const std::vector<const char*>& tokens_,
                                                      PriceType_t _dep_base_pricetype_) {
  return (GetUniqueInstanceFunc(tokens_[2]))(t_dbglogger_, cr_watch_, tokens_, _dep_base_pricetype_);
}

NonLinearWrapper* ModelCreator::GetNonLinearComponentFromTokens(BaseModelMath* p_this_model_math_,
                                                                const double this_component_weight_,
                                                                DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                                const std::vector<const char*>& tokens_) {
  int total_wrapper_ = (tokens_.size() - 2);

  if (total_wrapper_ % NONLINEAR_COMPONENT_ARGS != 0) {
    //	if ( ! (total_wrapper_ % (NONLINEAR_COMPONENT_ARGS + 1 ) == 0 && StringToPriceType_t ( tokens_[ tokens_.size ()
    //- 1 ] ) != kPriceTypeMax) )
    //	{
    ExitVerbose(kModelCreationNonLinearComponentLineLessArgs);
    //	}
  }

  NonLinearWrapper* new_non_linear_component_ = new NonLinearWrapper(this_component_weight_);

  for (unsigned int nonlinear_model_arg_counter_ = 2; nonlinear_model_arg_counter_ < tokens_.size();
       nonlinear_model_arg_counter_ += NONLINEAR_COMPONENT_ARGS) {
    // this is to be used if we don't want the direct indexing in model file and can pass args with indicators to
    // retrieve the index
    //      int this_indicator_index_ = p_this_model_math_ -> GetIndicatorIndexFromConciseDescription ( tokens_ [
    //      nonlinear_model_arg_counter_ ] ) ;

    std::string this_indicator_desc_ = tokens_[nonlinear_model_arg_counter_];
    int this_indicator_index_ = atoi(tokens_[nonlinear_model_arg_counter_ + 1]);
    double this_indicator_constant_value_ = atof(tokens_[nonlinear_model_arg_counter_ + 2]);
    bool this_indicator_sign_ = (atoi(tokens_[nonlinear_model_arg_counter_ + 3])) == -1 ? false : true;

    BasicWrapper* this_basic_non_linear_wrapper_ = new BasicWrapper(
        this_indicator_desc_, this_indicator_index_, this_indicator_constant_value_, this_indicator_sign_);

    new_non_linear_component_->AddBasicWrapper(this_basic_non_linear_wrapper_);
  }

  return new_non_linear_component_;
}
IndicatorLogger* ModelCreator::CreateIndicatorLogger(
    DebugLogger& t_dbglogger_, const Watch& cr_watch_, BulkFileWriter& _bulk_file_writer_,
    EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
    const std::string& _output_filename_, const unsigned int t_msecs_to_wait_to_print_again_,
    const unsigned long long t_l1events_timeout_, const unsigned int t_num_trades_to_wait_print_again_,
    const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
    const std::vector<SecurityMarketView*>& t_core_indep_smv_vec_, const std::vector<double>& t_c3_required_cutoff_,
    unsigned int regime_mode_to_print_, bool weighted_, bool pbat_bias_) {
  IndicatorLogger* p_indicator_logger_ = nullptr;

  SecurityMarketView* p_dep_market_view_ = nullptr;
  PriceType_t dep_base_pricetype_ = kPriceTypeMidprice;
  PriceType_t dep_pred_pricetype_ = kPriceTypeMidprice;
  SignalAlgo_t signal_algo_ = kSignalAlgoLinear;
  SetIndicatorListMap();

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (model_infile_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);

    ModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;
    // SignalAlgo_t signal_algo_ = kSignalAlgoLinear ; // unused in this mode

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_ ..
                                                                               // in tokenizing the string contents are
                                                                               // changed
        const std::vector<const char*>& tokens_ = st_.GetTokens();


        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }

        switch (current_model_creation_phase_) {
          case kModelCreationPhaseInit: {
            if (strcmp(tokens_[0], "MODELINIT") == 0) {  // MODELINIT
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelInitLineLessArgs);
              }
              if (strcmp(tokens_[1], "DEPBASE") == 0) {
                if ((tokens_.size() >= 3) && (strcmp(tokens_[2], "NONAME") == 0)) {  // MODELINIT DEPBASE NONAME

                  p_indicator_logger_ = new IndicatorLogger(
                      t_dbglogger_, cr_watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                      p_dep_market_view_, dep_base_pricetype_, dep_pred_pricetype_, _output_filename_,
                      t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                      t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_core_indep_smv_vec_,
                      t_c3_required_cutoff_, regime_mode_to_print_);
                } else {  // MODELINIT DEPBASE ZN0 Midprice MktSizeWPrice
                  if (tokens_.size() < 5) {
                    ExitVerbose(kModelCreationDepBaseLineLessArgs);
                  }
                  p_dep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens_[2]);
                  dep_base_pricetype_ = StringToPriceType_t((std::string)tokens_[3]);
                  dep_pred_pricetype_ = StringToPriceType_t((std::string)tokens_[4]);

                  p_indicator_logger_ = new IndicatorLogger(
                      t_dbglogger_, cr_watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                      p_dep_market_view_, dep_base_pricetype_, dep_pred_pricetype_, _output_filename_,
                      t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                      t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_core_indep_smv_vec_,
                      t_c3_required_cutoff_, regime_mode_to_print_, pbat_bias_);
                }
              }
              current_model_creation_phase_ = kModelCreationPhasePreModel;
            }
          } break;
          case kModelCreationPhasePreModel: {
            // MODELMATH LINEAR CHANGE
            // MODELMATH LINEAR RETURNS
            // MODELMATH NODEPLINEAR CHANGE
            // MODELMATH CLASSIFIER CHANGE
            // MODELMATH CART CHANGE
            // MODELMATH NEURALNETWORK CHANGE

            if (strcmp(tokens_[0], "MODELMATH") == 0) {
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelMathLineLessArgs);
              }
              signal_algo_ = GetSignalAlgo(tokens_[1]);  // unused in this mode
              current_model_creation_phase_ = kModelCreationPhasePostModel;
            }
          } break;
          case kModelCreationPhasePostModel: {
            if (strcmp(tokens_[0], "USE_IMPLIED_PRICE_FOR_NIK") == 0 && p_dep_market_view_->shortcode() == "NK_0") {
              BaseImpliedPrice* _this_indicator_ = BaseImpliedPrice::GetUniqueInstance(
                  t_dbglogger_, cr_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NK_0")),
                  *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NKM_0")), dep_base_pricetype_);

              _this_indicator_->SubscribePrice(dep_pred_pricetype_);
              p_indicator_logger_->SetImpliedPriceIndicator(_this_indicator_);
            }
            if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorStarted;
            } else {
              if (strcmp(tokens_[0], "MODELARGS") == 0) {
                // common modelmath arguments like :
                // MODELARGS number_of_layers_in_neural_network
                // MODELARGS num_trees max_nodes
              }
            }
          } break;
          case kModelCreationPhaseIndicatorStarted: {
            if (strcmp(tokens_[0], "INDICATOREND") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorEnded;
              p_indicator_logger_->set_basepx_pxtype();
              p_indicator_logger_->FinishCreation();
            } else {
              if (strcmp(tokens_[0], "INDICATOR") == 0) {
                if (tokens_.size() < 3) {
                  ExitVerbose(kModelCreationIndicatorLineLessArgs);
                }
                // INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
                CommonIndicator* _this_indicator_ =
                    GetIndicatorFromTokens(t_dbglogger_, cr_watch_, tokens_, dep_base_pricetype_);

                if (_this_indicator_ != nullptr) {
                  bool _this_readiness_required_ = false;
                  if (p_dep_market_view_ != nullptr) {
                    _this_readiness_required_ = GetReadinessRequired(p_dep_market_view_->shortcode(), tokens_);
                  }
                  if (weighted_) {
                    if (signal_algo_ == kSignalAlgoSIGLR) {
                      double _this_alpha_ = 0.0;
                      double _this_beta_ = 0.0;
                      std::istringstream iss(tokens_[1]);
                      std::string token;
                      getline(iss, token, ':');
                      _this_alpha_ = atof(token.c_str());
                      getline(iss, token, ':');
                      _this_beta_ = atof(token.c_str());
                      if (std::isnan(_this_alpha_) || std::isnan(_this_beta_)) {
                        ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
                      }
                      p_indicator_logger_->AddIndicator(_this_indicator_, _this_alpha_, _this_beta_,
                                                        _this_readiness_required_);
                    } else {
                      if (std::isnan(atof(tokens_[1]))) {
                        ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
                      }
                      p_indicator_logger_->AddIndicator(_this_indicator_, atof(tokens_[1]), _this_readiness_required_);
                    }
                  } else
                    p_indicator_logger_->AddIndicator(_this_indicator_, _this_readiness_required_);
                } else {
                  ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
                }
              }
            }
          } break;
          case kModelCreationPhaseIndicatorEnded: {
            // called for modelmath objects that might need a signal that all inputs have been registered and linkages
            // can be finalized now.
          } break;
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }

  return p_indicator_logger_;
}

IndicatorStats* ModelCreator::CreateIndicatorStats(
    DebugLogger& t_dbglogger_, const Watch& cr_watch_, BulkFileWriter& _bulk_file_writer_,
    EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
    const std::string& _output_filename_, const unsigned int t_msecs_to_wait_to_print_again_,
    const unsigned long long t_l1events_timeout_, const unsigned int t_num_trades_to_wait_print_again_,
    const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
    const std::vector<SecurityMarketView*>& t_core_indep_smv_vec_, const std::vector<double>& t_c3_required_cutoff_,
    unsigned int regime_mode_to_print_, HFSAT::IndicatorLogger::DatagenStats_t samples_print_, bool live_trading,
    std::map<int, std::pair<std::string, std::string> > t_stat_samples_map_live) {
  IndicatorStats* p_indicator_stats_ = nullptr;

  SecurityMarketView* p_dep_market_view_ = nullptr;
  PriceType_t dep_base_pricetype_ = kPriceTypeMidprice;
  PriceType_t dep_pred_pricetype_ = kPriceTypeMidprice;
  SetIndicatorListMap();
  SignalAlgo_t signal_algo_ = kSignalAlgoLinear;

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (model_infile_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);

    ModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;
    // SignalAlgo_t signal_algo_ = kSignalAlgoLinear ; // unused in this mode

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_ ..
                                                                               // in tokenizing the string contents are
                                                                               // changed
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }

        switch (current_model_creation_phase_) {
          case kModelCreationPhaseInit: {
            if (strcmp(tokens_[0], "MODELINIT") == 0) {  // MODELINIT
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelInitLineLessArgs);
              }
              if (strcmp(tokens_[1], "DEPBASE") == 0) {
                if ((tokens_.size() >= 3) && (strcmp(tokens_[2], "NONAME") == 0)) {  // MODELINIT DEPBASE NONAME

                  p_indicator_stats_ = new IndicatorStats(
                      t_dbglogger_, cr_watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                      p_dep_market_view_, dep_base_pricetype_, dep_pred_pricetype_, _output_filename_,
                      t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                      t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_core_indep_smv_vec_,
                      t_c3_required_cutoff_, regime_mode_to_print_, samples_print_, 900, false, live_trading,
                      t_stat_samples_map_live);
                } else {  // MODELINIT DEPBASE ZN0 Midprice MktSizeWPrice
                  if (tokens_.size() < 5) {
                    ExitVerbose(kModelCreationDepBaseLineLessArgs);
                  }
                  p_dep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens_[2]);
                  dep_base_pricetype_ = StringToPriceType_t((std::string)tokens_[3]);
                  dep_pred_pricetype_ = StringToPriceType_t((std::string)tokens_[4]);

                  p_indicator_stats_ = new IndicatorStats(
                      t_dbglogger_, cr_watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                      p_dep_market_view_, dep_base_pricetype_, dep_pred_pricetype_, _output_filename_,
                      t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                      t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_core_indep_smv_vec_,
                      t_c3_required_cutoff_, regime_mode_to_print_, samples_print_, 900, false, live_trading,
                      t_stat_samples_map_live);
                }
              }
              current_model_creation_phase_ = kModelCreationPhasePreModel;
            }
          } break;
          case kModelCreationPhasePreModel: {
            // MODELMATH LINEAR CHANGE
            // MODELMATH LINEAR RETURNS
            // MODELMATH NODEPLINEAR CHANGE
            // MODELMATH CLASSIFIER CHANGE
            // MODELMATH CART CHANGE
            // MODELMATH NEURALNETWORK CHANGE

            if (strcmp(tokens_[0], "MODELMATH") == 0) {
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelMathLineLessArgs);
              }
              signal_algo_ = GetSignalAlgo(tokens_[1]);  // unused in this mode
              current_model_creation_phase_ = kModelCreationPhasePostModel;
            }
          } break;
          case kModelCreationPhasePostModel: {
            if (strcmp(tokens_[0], "USE_IMPLIED_PRICE_FOR_NIK") == 0 && p_dep_market_view_->shortcode() == "NK_0") {
              BaseImpliedPrice* _this_indicator_ = BaseImpliedPrice::GetUniqueInstance(
                  t_dbglogger_, cr_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NK_0")),
                  *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NKM_0")), dep_base_pricetype_);

              _this_indicator_->SubscribePrice(dep_pred_pricetype_);
              p_indicator_stats_->SetImpliedPriceIndicator(_this_indicator_);
            }
            if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorStarted;
            } else {
              if (strcmp(tokens_[0], "MODELARGS") == 0) {
                // common modelmath arguments like :
                // MODELARGS number_of_layers_in_neural_network
                // MODELARGS num_trees max_nodes
              }
            }
          } break;
          case kModelCreationPhaseIndicatorStarted: {
            if (strcmp(tokens_[0], "INDICATOREND") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorEnded;
              p_indicator_stats_->set_basepx_pxtype();
              p_indicator_stats_->FinishCreation();
            } else {
              if (strcmp(tokens_[0], "INDICATOR") == 0) {
                if (tokens_.size() < 3) {
                  ExitVerbose(kModelCreationIndicatorLineLessArgs);
                }
                // INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...
                CommonIndicator* _this_indicator_ =
                    GetIndicatorFromTokens(t_dbglogger_, cr_watch_, tokens_, dep_base_pricetype_);

                if (_this_indicator_ != nullptr) {
                  bool _this_readiness_required_ = false;
                  if (p_dep_market_view_ != nullptr) {
                    _this_readiness_required_ = GetReadinessRequired(p_dep_market_view_->shortcode(), tokens_);
                  }
                  double weight = 0.0;
                  if (signal_algo_ == kSignalAlgoSIGLR) {
                    double _this_alpha_ = 0.0;
                    double _this_beta_ = 0.0;
                    std::istringstream iss(tokens_[1]);
                    std::string token;
                    getline(iss, token, ':');
                    _this_alpha_ = atof(token.c_str());
                    getline(iss, token, ':');
                    _this_beta_ = atof(token.c_str());
                    if (std::isnan(_this_alpha_) || std::isnan(_this_beta_)) {
                      ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
                      weight = _this_alpha_;
                    }
                  } else {
                    weight = atof(tokens_[1]);
                  }
                  p_indicator_stats_->indicator_weights_.push_back(weight);
                  p_indicator_stats_->AddIndicatorName(tokens_[2]);
                  p_indicator_stats_->AddIndicator(_this_indicator_, _this_readiness_required_);
                } else {
                  ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
                }
              }
            }
          } break;
          case kModelCreationPhaseIndicatorEnded: {
            // called for modelmath objects that might need a signal that all inputs have been registered and linkages
            // can be finalized now.
          } break;
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }

  return p_indicator_stats_;
}

// we should really stop creating further listener and use the exisiting ones with param
// indicator logger is the base class
IndicatorLogger* ModelCreator::CreateLogger(
    DebugLogger& t_dbglogger_, const Watch& cr_watch_, BulkFileWriter& _bulk_file_writer_,
    EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
    const std::string& _output_filename_, const unsigned int t_msecs_to_wait_to_print_again_,
    const unsigned long long t_l1events_timeout_, const unsigned int t_num_trades_to_wait_print_again_,
    const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
    const std::vector<SecurityMarketView*>& t_core_indep_smv_vec_, const std::vector<double>& t_c3_required_cutoff_,
    unsigned int regime_mode_to_print_, int type_) {
  IndicatorLogger* p_logger_ = nullptr;
  SecurityMarketView* p_dep_market_view_ = nullptr;
  PriceType_t dep_base_pricetype_ = kPriceTypeMidprice;
  PriceType_t dep_pred_pricetype_ = kPriceTypeMidprice;
  SetIndicatorListMap();

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (model_infile_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);

    ModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;
    // SignalAlgo_t signal_algo_ = kSignalAlgoLinear ; // unused in this mode

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_ ..
                                                                               // in tokenizing the string contents are
                                                                               // changed
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }

        switch (current_model_creation_phase_) {
          case kModelCreationPhaseInit: {
            if (strcmp(tokens_[0], "MODELINIT") == 0) {  // MODELINIT
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelInitLineLessArgs);
              }
              if (strcmp(tokens_[1], "DEPBASE") == 0) {
                if ((tokens_.size() >= 3) && (strcmp(tokens_[2], "NONAME") == 0)) {  // MODELINIT DEPBASE NONAME

                  if (type_ == 1) {
                    p_logger_ = new SumVarsLogger(
                        t_dbglogger_, cr_watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                        p_dep_market_view_, dep_base_pricetype_, dep_pred_pricetype_, _output_filename_,
                        t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                        t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_core_indep_smv_vec_,
                        t_c3_required_cutoff_, regime_mode_to_print_);
                  }
                } else {  // MODELINIT DEPBASE ZN0 Midprice MktSizeWPrice
                  if (tokens_.size() < 5) {
                    ExitVerbose(kModelCreationDepBaseLineLessArgs);
                  }
                  p_dep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens_[2]);
                  dep_base_pricetype_ = StringToPriceType_t((std::string)tokens_[3]);
                  dep_pred_pricetype_ = StringToPriceType_t((std::string)tokens_[4]);

                  if (type_ == 1) {
                    p_logger_ = new SumVarsLogger(
                        t_dbglogger_, cr_watch_, _bulk_file_writer_, r_economic_events_manager_, _model_filename_,
                        p_dep_market_view_, dep_base_pricetype_, dep_pred_pricetype_, _output_filename_,
                        t_msecs_to_wait_to_print_again_, t_l1events_timeout_, t_num_trades_to_wait_print_again_,
                        t_to_print_on_economic_times_, t_sample_on_core_shortcodes_, t_core_indep_smv_vec_,
                        t_c3_required_cutoff_, regime_mode_to_print_);
                  }
                }
              }
              current_model_creation_phase_ = kModelCreationPhasePreModel;
            }
          } break;
          case kModelCreationPhasePreModel: {
            // MODELMATH LINEAR CHANGE
            // MODELMATH LINEAR RETURNS
            // MODELMATH NODEPLINEAR CHANGE
            // MODELMATH CLASSIFIER CHANGE
            // MODELMATH CART CHANGE
            // MODELMATH NEURALNETWORK CHANGE

            if (strcmp(tokens_[0], "MODELMATH") == 0) {
              if (tokens_.size() < 2) {
                ExitVerbose(kModelCreationModelMathLineLessArgs);
              }
              // signal_algo_ = GetSignalAlgo ( tokens_[1] ); // unused in this mode
              current_model_creation_phase_ = kModelCreationPhasePostModel;
            }
          } break;
          case kModelCreationPhasePostModel: {
            if (strcmp(tokens_[0], "USE_IMPLIED_PRICE_FOR_NIK") == 0 && p_dep_market_view_->shortcode() == "NK_0") {
              BaseImpliedPrice* _this_indicator_ = BaseImpliedPrice::GetUniqueInstance(
                  t_dbglogger_, cr_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NK_0")),
                  *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView("NKM_0")), dep_base_pricetype_);

              _this_indicator_->SubscribePrice(dep_pred_pricetype_);
              p_logger_->SetImpliedPriceIndicator(_this_indicator_);
            }
            if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorStarted;
            } else {
              if (strcmp(tokens_[0], "MODELARGS") == 0) {
                // common modelmath arguments like :
                // MODELARGS number_of_layers_in_neural_network
                // MODELARGS num_trees max_nodes
              }
            }
          } break;
          case kModelCreationPhaseIndicatorStarted: {
            if (strcmp(tokens_[0], "INDICATOREND") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseIndicatorEnded;
              p_logger_->set_basepx_pxtype();
              p_logger_->FinishCreation();
            } else {
              if (strcmp(tokens_[0], "INDICATOR") == 0) {
                if (tokens_.size() < 3) {
                  ExitVerbose(kModelCreationIndicatorLineLessArgs);
                }
                // INDICATOR _this_weight_ _indicator_string_ _indicator_arg_1 _indicator_arg_2 ...

                CommonIndicator* _this_indicator_ =
                    GetIndicatorFromTokens(t_dbglogger_, cr_watch_, tokens_, dep_base_pricetype_);

                if (_this_indicator_ != nullptr) {
                  bool _this_readiness_required_ = false;
                  if (p_dep_market_view_ != nullptr) {
                    _this_readiness_required_ = GetReadinessRequired(p_dep_market_view_->shortcode(), tokens_);
                  }
                  if (std::isnan(atof(tokens_[1]))) {
                    ExitVerbose(kModelCreationIndicatorWeightNAN, "Indicator weight is NAN\n");
                  }
                  p_logger_->AddIndicator(_this_indicator_, atof(tokens_[1]), _this_readiness_required_);
                } else {
                  ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2]);
                }
              }
            }
          } break;
          case kModelCreationPhaseIndicatorEnded: {
            // called for modelmath objects that might need a signal that all inputs have been registered and linkages
            // can be finalized now.
          } break;
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }

  return p_logger_;
}

void ModelCreator::LinkupIndicatorLoggerToOnReadySources(IndicatorLogger* p_indicator_logger_,
                                                         std::vector<std::string>& shortcodes_affecting_this_model_,
                                                         std::vector<std::string>& ors_source_needed_vec_) {
  if (p_indicator_logger_ != nullptr) {
    // subscribe_OnReady to all the shortcodes affecting this model
    for (auto i = 0u; i < shortcodes_affecting_this_model_.size(); i++) {
      (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcodes_affecting_this_model_[i]))
          ->subscribe_OnReady(p_indicator_logger_);
    }
    // subscribe to PromOM OnGlobalPositionChange events for sources in ors_source_needed_vec_
    // most indicators are global_position based and hence they would have been added as listeners already
    // now if we add model_math we could be giving control model math object to use the updated values of
    // the indicators to compute new target price and alert basetrading
    for (auto i = 0u; i < ors_source_needed_vec_.size(); i++) {
      PromOrderManager* p_this_prom_order_manager_ = PromOrderManager::GetCreatedInstance(ors_source_needed_vec_[i]);
      if (p_this_prom_order_manager_ != nullptr) {  // should be the case always
        p_this_prom_order_manager_->AddGlobalPositionChangeListener(p_indicator_logger_);
      }
    }
  }
}

void ModelCreator::LinkupIndicatorStatsToOnReadySources(IndicatorStats* p_indicator_stats_,
                                                        std::vector<std::string>& shortcodes_affecting_this_model_,
                                                        std::vector<std::string>& ors_source_needed_vec_) {
  if (p_indicator_stats_ != nullptr) {
    // subscribe_OnReady to all the shortcodes affecting this model
    for (auto i = 0u; i < shortcodes_affecting_this_model_.size(); i++) {
      (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcodes_affecting_this_model_[i]))
          ->subscribe_OnReady(p_indicator_stats_);
    }
    // subscribe to PromOM OnGlobalPositionChange events for sources in ors_source_needed_vec_
    // most indicators are global_position based and hence they would have been added as listeners already
    // now if we add model_math we could be giving control model math object to use the updated values of
    // the indicators to compute new target price and alert basetrading
    for (auto i = 0u; i < ors_source_needed_vec_.size(); i++) {
      PromOrderManager* p_this_prom_order_manager_ = PromOrderManager::GetCreatedInstance(ors_source_needed_vec_[i]);
      if (p_this_prom_order_manager_ != nullptr) {  // should be the case always
        p_this_prom_order_manager_->AddGlobalPositionChangeListener(p_indicator_stats_);
      }
    }
  }
}

void ModelCreator::LinkupLoggerToOnReadySources(IndicatorLogger* p_logger_,
                                                std::vector<std::string>& shortcodes_affecting_this_model_,
                                                std::vector<std::string>& ors_source_needed_vec_) {
  if (p_logger_ != nullptr) {
    for (auto i = 0u; i < shortcodes_affecting_this_model_.size(); i++) {
      (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcodes_affecting_this_model_[i]))
          ->subscribe_OnReady(p_logger_);
    }
    for (auto i = 0u; i < ors_source_needed_vec_.size(); i++) {
      PromOrderManager* p_this_prom_order_manager_ = PromOrderManager::GetCreatedInstance(ors_source_needed_vec_[i]);
      if (p_this_prom_order_manager_ != nullptr) {  // should be the case always
        p_this_prom_order_manager_->AddGlobalPositionChangeListener(p_logger_);
      }
    }
  }
}
}
