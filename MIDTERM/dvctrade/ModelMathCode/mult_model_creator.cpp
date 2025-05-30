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
#include "dvctrade/ModelMath/signal_algo.hpp"
#include "dvctrade/ModelMath/mult_model_creator.hpp"
#include "dvctrade/InitCommon/strategy_desc.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvctrade/ModelMath/signal_algo.hpp"

namespace HFSAT {
std::map<std::string, BaseMultipleModelMath*> MultModelCreator::shortcode_basemodelmath_map_;
std::map<std::string, BaseModelMath*> MultModelCreator::shortcode_futmodelmath_map_;
std::map<unsigned int, std::pair<int, int> > MultModelCreator::sec_id_product_index_map_;
std::vector<std::vector<std::string> > MultModelCreator::individual_indicator_token_strings_vec_;
std::vector<std::vector<std::string> > MultModelCreator::global_indicator_token_strings_vec_;
std::vector<BaseMultipleModelMath*> MultModelCreator::multiple_model_math_vec_;
std::vector<BaseModelMath*> MultModelCreator::fut_model_math_vec_;
SignalAlgo_t MultModelCreator::signal_algo_;
std::vector<std::string> MultModelCreator::indicator_order_vec_;
std::vector<std::string> MultModelCreator::underlying_vec_;
std::vector<SecurityMarketView*> MultModelCreator::dep_market_view_vec_;
std::vector<PriceType_t> MultModelCreator::dep_pricetype_vec_;
std::map<std::string, std::vector<std::string> > MultModelCreator::shortcode_const_map_;
std::map<std::string, std::vector<SecurityMarketView*> > MultModelCreator::shortocde_const_smv_map_;
std::map<std::string, std::string> MultModelCreator::shortcode_model_weights_map_;
std::map<std::string, std::string> MultModelCreator::shortcode_paramfile_map_;
std::vector<std::string> MultModelCreator::banned_securities_list_;

std::string MultModelCreator::GetParamFromDate(std::string _dated_param_filename_, int _date_) {
  std::string param_file_;
  std::ifstream param_infile_;
  param_infile_.open(_dated_param_filename_.c_str(), std::ifstream::in);
  if (!param_infile_.is_open()) {
    ExitVerbose(kStrategyDescParamFileMissing);
  } else {
    const unsigned int kParamLineBufferLen = 1024;
    char readline_buffer_[kParamLineBufferLen];

    // Loop only to check first valid : whether its a dated paramlist file or normal param file
    while (param_infile_.good()) {
      bzero(readline_buffer_, kParamLineBufferLen);
      param_infile_.getline(readline_buffer_, kParamLineBufferLen);
      if (param_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kParamLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if ((tokens_.size() < 1) || (tokens_[0][0] == '#'))  // skip empty and commented lines
          continue;

        if (strcmp(tokens_[0], "PARAMVALUE") == 0) {  // This is normal param file, not dated list
          param_file_ = _dated_param_filename_;
          return param_file_;
        }

        param_file_ = std::string(tokens_[1]);
        break;
      }
    }

    while (param_infile_.good()) {
      bzero(readline_buffer_, kParamLineBufferLen);
      param_infile_.getline(readline_buffer_, kParamLineBufferLen);
      if (param_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kParamLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if ((tokens_.size() < 1) || (tokens_[0][0] == '#'))  // skip empty and commented lines
          continue;

        int date_ = std::atoi(tokens_[0]);  // Date are supposed to be in ascending order
        if (date_ > _date_) break;
        param_file_ = std::string(tokens_[1]);
      }
    }
  }
  return param_file_;
}

void MultModelCreator::CollectShortCodes(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                         const std::string _model_filename_,
                                         std::vector<std::string>& _rw_shortcodes_affecting_this_model_,
                                         std::vector<std::string>& ors_source_needed_vec_,
                                         std::vector<std::string>& dependent_vec_) {
  SetIndicatorListMap();

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);

  PriceType_t dep_base_pricetype_ = kPriceTypeMidprice;  // Can be changed to be defined per underlying ??

  banned_securities_list_ = NSESecurityDefinitions::LoadBannedSecurities(cr_watch_.YYYYMMDD());

  if (!model_infile_.is_open()) {
    ExitVerbose(kModelCreationCouldNotOpenModelFile);
  } else {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    MultModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if ((tokens_.size() < 1) || (tokens_[0][0] == '#')) {  // skip empty and commented lines
          continue;
        }
        switch (current_model_creation_phase_) {
          case kModelCreationPhaseInit:
            if (strcmp(tokens_[0], "MODELINIT") == 0) {  // MODELINIT
              if (tokens_.size() < 3) {
                ExitVerbose(kModelCreationModelInitLineLessArgs);
              }
              dep_base_pricetype_ = StringToPriceType_t(tokens_[2]);
              current_model_creation_phase_ = kModelCreationPhasePreModel;
            }
            break;
          case kModelCreationPhasePreModel:
            if (strcmp(tokens_[0], "PARAMSHCPAIR") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseShortcodeCollection;
            }
            break;

          case kModelCreationPhaseShortcodeCollection:
            if (strcmp(tokens_[0], "INDVIDUAL_INDICATORS") == 0 || strcmp(tokens_[0], "IINDICATORS") == 0 ||
                strcmp(tokens_[0], "INDIVIDUAL_INDICATORS") == 0) {
              if (underlying_vec_.size() == 0) {
                ExitVerbose(kShortcodeSecurityMarketViewMapNoSMVInMap);
              }

              current_model_creation_phase_ = kModelCreationPhaseIndividualIndicatorsStarted;
              break;
            }
            if (tokens_.size() < 5) {
              ExitVerbose(kModelCreationIndicatorLineLessArgs);
            }
            {
              std::string this_shortcode_ = std::string(tokens_[0]);
              std::string base_name_ = this_shortcode_.substr(4, this_shortcode_.size() - 9);
              if (std::find(banned_securities_list_.begin(), banned_securities_list_.end(), base_name_) !=
                  banned_securities_list_.end()) {
                continue;
              }
              if (strcmp(tokens_[2], "DUMMY") != 0)
                ModelCreator::CollectShortCodes(t_dbglogger_, cr_watch_, tokens_[2],
                                                _rw_shortcodes_affecting_this_model_, ors_source_needed_vec_, false,
                                                false);
            }
            underlying_vec_.push_back(tokens_[0]);
            dep_pricetype_vec_.push_back(dep_base_pricetype_);
            shortcode_paramfile_map_[tokens_[0]] = GetParamFromDate(tokens_[1], cr_watch_.YYYYMMDD());
            shortcode_model_weights_map_[tokens_[0]] = tokens_[3];
            VectorUtils::UniqueVectorAdd(_rw_shortcodes_affecting_this_model_, (std::string)tokens_[0]);
            VectorUtils::UniqueVectorAdd(dependent_vec_, (std::string)tokens_[0]);
            shortcode_const_map_[tokens_[0]] = std::vector<std::string>();
            {
              std::vector<std::string> options_ = GetOptionsToTrade(tokens_);
              for (auto i = 0u; i < options_.size(); i++) {
                VectorUtils::UniqueVectorAdd(_rw_shortcodes_affecting_this_model_, (std::string)options_[i]);
                VectorUtils::UniqueVectorAdd(dependent_vec_, (std::string)options_[i]);
                shortcode_const_map_[tokens_[0]].push_back(options_[i]);
              }
            }
            break;

          case kModelCreationPhaseIndividualIndicatorsStarted:
            if (strcmp(tokens_[0], "GLOBAL_INDICATORS") == 0 || strcmp(tokens_[0], "GINDICATORS") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseGlobalIndicatorsStarted;
            } else {
              std::vector<std::string> tokens1_;
              for (auto i = 0u; i < tokens_.size(); i++) {
                tokens1_.push_back(tokens_[i]);
              }
              std::string dummy_underlying_shc_ = "SHC";
              for (auto i = 0u; i < underlying_vec_.size(); i++) {
                std::string dummy_const_shc_ = "CONST_SHC";
                std::vector<std::string> const_vec_ = shortcode_const_map_[underlying_vec_[i]];
                for (unsigned int j = 0; j < const_vec_.size(); j++) {
                  std::vector<const char*> vc;
                  for (auto _token_ : tokens1_) {
                    if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0)
                      vc.push_back(underlying_vec_[i].c_str());
                    else if (strcmp(_token_.c_str(), dummy_const_shc_.c_str()) == 0)
                      vc.push_back(const_vec_[j].c_str());
                    else
                      vc.push_back(_token_.c_str());
                  }
                  (CollectShortCodeFunc(tokens1_[2]))(_rw_shortcodes_affecting_this_model_, ors_source_needed_vec_, vc);
                }
              }
            }
            break;
          case kModelCreationPhaseGlobalIndicatorsStarted:
            if (strcmp(tokens_[0], "INDICATORSEND") == 0) {  // INDICATOREND
              current_model_creation_phase_ = kModelCreationPhaseEnded;
            } else {
              std::vector<std::string> tokens1_;
              for (auto i = 0u; i < tokens_.size(); i++) {
                tokens1_.push_back(tokens_[i]);
              }
              std::string dummy_underlying_shc_ = "SHC";
              for (auto i = 0u; i < underlying_vec_.size(); i++) {
                std::vector<const char*> vc;
                for (auto _token_ : tokens1_) {
                  if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0)
                    vc.push_back(underlying_vec_[i].c_str());
                  else
                    vc.push_back(_token_.c_str());
                }
                (CollectShortCodeFunc(tokens_[2]))(_rw_shortcodes_affecting_this_model_, ors_source_needed_vec_, vc);
              }
            }
            break;
          case kModelCreationPhaseEnded:
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }
}

void MultModelCreator::LinkupModelMathToOnReadySources(BaseMultipleModelMath* p_this_model_math_) {
  std::vector<std::string> shortcodes_affecting_this_model_ = p_this_model_math_->GetShortCodeAffectingModel();
  std::vector<std::string> ors_source_needed_vec_ = p_this_model_math_->GetORSShortCodeNeeded();

  if (p_this_model_math_ != NULL) {
    // subscribe_OnReady to all the shortcodes affecting this model
    for (auto i = 0u; i < shortcodes_affecting_this_model_.size(); i++) {
      (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcodes_affecting_this_model_[i]))
          ->subscribe_OnReady(p_this_model_math_);
    }
  }
}

std::vector<BaseMultipleModelMath*> MultModelCreator::CreateModelMathComponent(DebugLogger& t_dbglogger_,
                                                                               const Watch& cr_watch_,
                                                                               const std::string& _model_filename_,
                                                                               int trading_start_mfm,
                                                                               int trading_end_mfm, int runtime_id) {
  std::string map_key_ = _model_filename_;
  std::ostringstream temp_oss_;
  temp_oss_ << _model_filename_ << " ";

  std::ifstream model_infile_;
  model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  if (model_infile_.is_open()) {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    MultModelCreationPhases_t current_model_creation_phase_ = kModelCreationPhaseInit;
    PriceType_t dep_base_pricetype_ = kPriceTypeMidprice;
    std::vector<std::string> core_shortcodes_;
    core_shortcodes_.clear();
    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        /*if (dbglogger_.CheckLoggingLevel(DBG_MODEL_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << readline_buffer_ <<
                                      << DBGLOG_ENDL_FLUSH;
                                      }*/
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if ((tokens_.size() < 1) || (tokens_[0][0] == '#')) {  // skip empty and commented lines
          continue;
        }

        switch (current_model_creation_phase_) {
          case kModelCreationPhaseInit:
            if (strcmp(tokens_[0], "MODELINIT") == 0) {  // MODELINIT
              if (tokens_.size() < 3) {
                ExitVerbose(kModelCreationModelInitLineLessArgs);
              }
              signal_algo_ = GetSignalAlgo(tokens_[1]);
              dep_base_pricetype_ = StringToPriceType_t(tokens_[2]);
              current_model_creation_phase_ = kModelCreationPhasePreModel;
            }
            break;

          case kModelCreationPhasePreModel:
            if (strcmp(tokens_[0], "PARAMSHCPAIR") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseShortcodeCollection;
            }
            break;

          case kModelCreationPhaseShortcodeCollection:

            if (strcmp(tokens_[0], "INDVIDUAL_INDICATORS") == 0 || strcmp(tokens_[0], "INDIVIDUAL_INDICATORS") == 0 ||
                strcmp(tokens_[0], "IINDICATORS") == 0) {
              if (dep_market_view_vec_.size() == 0) {
                ExitVerbose(kShortcodeSecurityMarketViewMapNoSMVInMap);
              }

              current_model_creation_phase_ = kModelCreationPhaseIndividualIndicatorsStarted;
              break;
            }

            if (tokens_.size() < 5) {
              ExitVerbose(kModelCreationIndicatorLineLessArgs);
            }

            // Defining the scope here as new variable can't be initialized without it in a case statement
            {
              // reading underlying here
              std::string this_shortcode_ = std::string(tokens_[0]);
              std::string base_name_ = this_shortcode_.substr(4, this_shortcode_.size() - 9);
              if (std::find(banned_securities_list_.begin(), banned_securities_list_.end(), base_name_) !=
                  banned_securities_list_.end()) {
                continue;
              }
              SecurityMarketView* dep_market_view__ =
                  ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(tokens_[0]);
              dep_market_view__->SetBasePriceType(dep_base_pricetype_);
              dep_market_view_vec_.push_back(dep_market_view__);
              GetCoreShortcodes(this_shortcode_, core_shortcodes_);
              shortocde_const_smv_map_[tokens_[0]] = std::vector<SecurityMarketView*>();

              // making the options iv model math
              BaseMultipleModelMath* p_this_model_math_ = NULL;

              switch (signal_algo_) {
                case kSignalAlgoLinear:  // linearmodel with tgt a derivative of dependant price
                {
                  if (dep_market_view__ == NULL) {
                    ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                  }

                  if (tokens_.size() < 3) {
                    ExitVerbose(kModelCreationLinearModelMathLineLessArgs);
                  }
                  p_this_model_math_ =
                      new LinearMultModelAggregator(t_dbglogger_, cr_watch_, dep_market_view__,
                                                    shortcode_paramfile_map_[tokens_[0]], dep_base_pricetype_);
                } break;

                case kSignalAlgoOnlineLinear: {
                  if (dep_market_view__ == NULL) {
                    ExitVerbose(kModelCreationLinearModelMathNeedsNonNullDependant);
                  }
                  if (tokens_.size() < 4) {
                    ExitVerbose(kModelCreationLinearModelMathLineLessArgs);
                  }
                  p_this_model_math_ =
                      new LinearMultModelAggregator(t_dbglogger_, cr_watch_, dep_market_view__,
                                                    shortcode_paramfile_map_[tokens_[0]], dep_base_pricetype_);

                } break;
                default:
                  break;
              }

              // Making the fut model
              BaseModelMath* f_this_model_math_ = NULL;
              if (strcmp(tokens_[2], "DUMMY") != 0)
                f_this_model_math_ = ModelCreator::CreateModelMathComponent(
                    t_dbglogger_, cr_watch_, tokens_[2], nullptr, trading_start_mfm, trading_end_mfm, runtime_id);

              // collect options/contracts, either specified based on logic or hard shortcode strings
              int num_underlying_ = dep_market_view_vec_.size();

              // LogicIdentifier NumberOfOptions StepIncreaments
              // OTMExATM_CS 4 1
              // OTMExATM_CS 6 2
              // LogicIndentfier ( CS::CurrentSchema, AS::AllSchema PS::PreviousSchema )
              {
                std::vector<std::string> options_ = GetOptionsToTrade(tokens_);

                for (auto i = 0u; i < options_.size(); i++) {
                  SecurityMarketView* const_market_view__ =
                      ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(options_[i]);
                  const_market_view__->SetBasePriceType(dep_base_pricetype_);
                  shortocde_const_smv_map_[tokens_[0]].push_back(const_market_view__);
                  p_this_model_math_->AddShortCode(const_market_view__);
                  sec_id_product_index_map_[const_market_view__->security_id()] =
                      std::make_pair(num_underlying_ - 1, i);
                }
              }
              // adding underlying to secid, setting modelmath index
              sec_id_product_index_map_[dep_market_view__->security_id()] = std::make_pair(num_underlying_ - 1, -1);
              p_this_model_math_->SetModelMathIndex(num_underlying_ - 1);
              shortcode_basemodelmath_map_[tokens_[0]] = p_this_model_math_;
              shortcode_futmodelmath_map_[tokens_[0]] = f_this_model_math_;
              multiple_model_math_vec_.push_back(p_this_model_math_);
              fut_model_math_vec_.push_back(f_this_model_math_);
            }
            break;

          case kModelCreationPhaseIndividualIndicatorsStarted:
            if (strcmp(tokens_[0], "GLOBAL_INDICATORS") == 0 || strcmp(tokens_[0], "GINDICATORS") == 0) {
              current_model_creation_phase_ = kModelCreationPhaseGlobalIndicatorsStarted;
            } else {
              std::vector<std::string> this_indicator_tokens_vec_;
              for (unsigned index_ = 0; index_ < tokens_.size(); index_++) {
                this_indicator_tokens_vec_.push_back(tokens_[index_]);
              }
              individual_indicator_token_strings_vec_.push_back(this_indicator_tokens_vec_);
            }
            break;

          case kModelCreationPhaseGlobalIndicatorsStarted:
            if (strcmp(tokens_[0], "INDICATORSEND") == 0) {  // INDICATOREND
              current_model_creation_phase_ = kModelCreationPhaseEnded;
            } else {
              std::vector<std::string> this_indicator_tokens_vec_;
              for (unsigned index_ = 0; index_ < tokens_.size(); index_++) {
                this_indicator_tokens_vec_.push_back(tokens_[index_]);
              }
              global_indicator_token_strings_vec_.push_back(this_indicator_tokens_vec_);
            }
            break;

          case kModelCreationPhaseEnded:
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }
  return multiple_model_math_vec_;
}

void MultModelCreator::CreateIndicatorInstances(DebugLogger& t_dbglogger_, const Watch& cr_watch_) {
  //    static int count_call_ = 0;
  //    count_call_++;
  indicator_order_vec_ = GetIndicatorOrder();

  std::vector<bool> individual_instance_created_;
  individual_instance_created_.resize(individual_indicator_token_strings_vec_.size(), false);

  std::vector<bool> global_instance_created_;
  global_instance_created_.resize(global_indicator_token_strings_vec_.size(), false);

  CommonIndicator* _this_indicator_ = NULL;
  // Two instances of indicator initialization has to be done. First to maintain indicator
  // order globally and second to add indicator according to order in model file

  for (unsigned index_ = 0; index_ < indicator_order_vec_.size() + 1; index_++) {
    std::vector<unsigned> individual_indicator_index_vec_;
    std::vector<unsigned> global_indicator_index_vec_;
    if (index_ == indicator_order_vec_.size()) {
      // Having extra case if there is no entry for indicator in IndicatorOrder file
      for (unsigned i = 0; i < individual_instance_created_.size(); i++) {
        if (!individual_instance_created_[i]) {
          ;
          individual_indicator_index_vec_.push_back(i);
        }
      }
      for (unsigned i = 0; i < global_instance_created_.size(); i++) {
        if (!global_instance_created_[i]) {
          ;
          global_indicator_index_vec_.push_back(i);
        }
      }
    } else {
      individual_indicator_index_vec_ =
          VectorUtils::LinearSearchValueIdxVec(individual_indicator_token_strings_vec_, indicator_order_vec_[index_]);
      global_indicator_index_vec_ =
          VectorUtils::LinearSearchValueIdxVec(global_indicator_token_strings_vec_, indicator_order_vec_[index_]);
    }

    for (unsigned idx_ = 0; idx_ < individual_indicator_index_vec_.size(); idx_++) {
      unsigned indicator_index_ = individual_indicator_index_vec_[idx_];
      //	    std::cerr << "KP " << count_call_ << " " << index_ << " " << idx_ << " " << indicator_index_ << " "
      //<< model_entry_type_vec_[indicator_index_] << "\n";
      if (indicator_index_ == individual_indicator_token_strings_vec_.size()) {
        // No indicator of this name
      } else {
        std::vector<std::string> tokens_ = individual_indicator_token_strings_vec_[indicator_index_];

        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }

        std::string dummy_underlying_shc_ = "SHC";
        for (auto i = 0u; i < underlying_vec_.size(); i++) {
          PriceType_t dep_base_pricetype_ = dep_pricetype_vec_[i];
          std::string dummy_const_shc_ = "CONST_SHC";
          std::vector<std::string> const_vec_ = shortcode_const_map_[underlying_vec_[i]];
          for (unsigned int j = 0; j < const_vec_.size(); j++) {
            std::vector<const char*> vc;
            for (auto _token_ : tokens_) {
              if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0)
                vc.push_back(underlying_vec_[i].c_str());
              else if (strcmp(_token_.c_str(), dummy_const_shc_.c_str()) == 0)
                vc.push_back(const_vec_[j].c_str());
              else
                vc.push_back(_token_.c_str());
            }
            _this_indicator_ = GetIndicatorFromTokens(t_dbglogger_, cr_watch_, vc, dep_base_pricetype_);
          }
        }
        individual_instance_created_[indicator_index_] = true;
      }
    }

    for (unsigned idx_ = 0; idx_ < global_indicator_index_vec_.size(); idx_++) {
      unsigned indicator_index_ = global_indicator_index_vec_[idx_];
      //	    std::cerr << "KP " << count_call_ << " " << index_ << " " << idx_ << " " << indicator_index_ << " "
      //<< model_entry_type_vec_[indicator_index_] << "\n";
      if (indicator_index_ == global_indicator_token_strings_vec_.size()) {
        // No indicator of this name
      } else {
        std::vector<std::string> tokens_ = global_indicator_token_strings_vec_[indicator_index_];

        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }

        std::string dummy_underlying_shc_ = "SHC";
        for (auto i = 0u; i < underlying_vec_.size(); i++) {
          PriceType_t dep_base_pricetype_ = dep_pricetype_vec_[i];
          std::vector<const char*> vc;
          for (auto _token_ : tokens_) {
            if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0)
              vc.push_back(underlying_vec_[i].c_str());
            else
              vc.push_back(_token_.c_str());
          }
          _this_indicator_ = GetIndicatorFromTokens(t_dbglogger_, cr_watch_, vc, dep_base_pricetype_);
        }
        global_instance_created_[indicator_index_] = true;
      }
    }
  }

  std::string dummy_underlying_shc_ = "SHC";
  for (unsigned int product_index_ = 0; product_index_ < underlying_vec_.size(); product_index_++) {
    std::vector<std::string> this_shortcodes_affecting_;
    std::vector<std::string> this_ors_source_needed_vec_;
    PriceType_t dep_base_pricetype_ = dep_pricetype_vec_[product_index_];
    std::vector<std::string> const_vec_ = shortcode_const_map_[underlying_vec_[product_index_]];

    std::string dummy_const_shc_ = "CONST_SHC";
    for (unsigned int const_index_ = 0; const_index_ < const_vec_.size(); const_index_++) {
      for (unsigned indicator_index_ = 0; indicator_index_ < individual_indicator_token_strings_vec_.size();
           indicator_index_++) {
        std::vector<std::string> tokens_ = individual_indicator_token_strings_vec_[indicator_index_];

        if (tokens_.size() < 3) {
          ExitVerbose(kModelCreationIndicatorLineLessArgs);
        }

        // product_index = file_id
        // const_index_ = col number
        // indicator_index_ = row_number
        // each row corresponds to weights one indicator of all options
        std::vector<double> t_dummy_argument_ = std::vector<double>();
        double _this_weight_ = GetTokenFromFile(shortcode_model_weights_map_[underlying_vec_[product_index_].c_str()],
                                                indicator_index_, const_index_, t_dummy_argument_);

        std::vector<const char*> vc;
        for (auto _token_ : tokens_) {
          if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0)
            vc.push_back(underlying_vec_[product_index_].c_str());
          else if (strcmp(_token_.c_str(), dummy_const_shc_.c_str()) == 0)
            vc.push_back(const_vec_[const_index_].c_str());
          else
            vc.push_back(_token_.c_str());
        }
        _this_indicator_ = GetIndicatorFromTokens(t_dbglogger_, cr_watch_, vc, dep_base_pricetype_);
        (CollectShortCodeFunc(tokens_[2]))(this_shortcodes_affecting_, this_ors_source_needed_vec_, vc);

        for (unsigned j = 0; j < this_shortcodes_affecting_.size(); j++) {
          SecurityMarketView* this_smv_ =
              ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(this_shortcodes_affecting_[j]);
          this_smv_->subscribe_MktStatus(_this_indicator_);
        }

        if (_this_indicator_ != NULL) {
          bool _this_readiness_required_ = false;
          multiple_model_math_vec_[product_index_]->AddIndividualIndicator(_this_indicator_, _this_weight_,
                                                                           _this_readiness_required_, const_index_);
        } else {
          ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2].c_str());
        }
      }
    }

    for (unsigned indicator_index_ = 0; indicator_index_ < global_indicator_token_strings_vec_.size();
         indicator_index_++) {
      std::vector<std::string> tokens_ = global_indicator_token_strings_vec_[indicator_index_];

      if (tokens_.size() < 3) {
        ExitVerbose(kModelCreationIndicatorLineLessArgs);
      }

      //    std::vector<std::string> const_vec_ = shortcode_const_map_[underlying_vec_[product_index_]];
      std::vector<double> this_weight_vector_(shortcode_const_map_[underlying_vec_[product_index_]].size(), 1.0);

      /*for(auto i = 0u; i < this_weight_vector_.size(); i++) {
        std::cout << this_weight_vector_[i] << " "; } std::cout << "\n";*/

      std::vector<const char*> vc;
      for (auto _token_ : tokens_) {
        if (strcmp(_token_.c_str(), dummy_underlying_shc_.c_str()) == 0)
          vc.push_back(underlying_vec_[product_index_].c_str());
        else
          vc.push_back(_token_.c_str());
      }
      _this_indicator_ = GetIndicatorFromTokens(t_dbglogger_, cr_watch_, vc, dep_base_pricetype_);
      (CollectShortCodeFunc(tokens_[2]))(this_shortcodes_affecting_, this_ors_source_needed_vec_, vc);

      for (unsigned j = 0; j < this_shortcodes_affecting_.size(); j++) {
        SecurityMarketView* this_smv_ =
            ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(this_shortcodes_affecting_[j]);
        this_smv_->subscribe_MktStatus(_this_indicator_);
      }

      if (_this_indicator_ != NULL) {
        bool _this_readiness_required_ = false;
        multiple_model_math_vec_[product_index_]->AddGlobalIndicator(_this_indicator_, this_weight_vector_,
                                                                     _this_readiness_required_);
      } else {
        ExitVerbose(kModelCreationIndicatorLineNull, tokens_[2].c_str());
      }
    }

    multiple_model_math_vec_[product_index_]->SetShortCodeAffectingModel(this_shortcodes_affecting_);
    multiple_model_math_vec_[product_index_]->SetORSShortCodeNeeded(this_ors_source_needed_vec_);
    multiple_model_math_vec_[product_index_]->SetNumIndividualIndicators(
        individual_indicator_token_strings_vec_.size());
    multiple_model_math_vec_[product_index_]->FinishCreation();
  }

  // asking to create fut model indicator instances
  ModelCreator::CreateIndicatorInstances(t_dbglogger_, cr_watch_);
}

CommonIndicator* MultModelCreator::GetIndicatorFromTokens(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                          const std::vector<const char*>& tokens_,
                                                          PriceType_t _dep_base_pricetype_) {
  return (GetUniqueInstanceFunc(tokens_[2]))(t_dbglogger_, cr_watch_, tokens_, _dep_base_pricetype_);
}

std::vector<std::string> MultModelCreator::GetOptionsToTrade(const std::vector<const char*>& tokens_) {
  // number_of_contracts * length_of_steps + 1;
  char futures_[40];
  strcpy(futures_, tokens_[0]);
  char* basename_ = strtok(futures_, "_");  // NSE
  if (basename_ != NULL) {
    basename_ = strtok(NULL, "_");  // BASENAME
  }
  std::vector<std::string> options_;

  if (tokens_.size() == 7 && strcmp(tokens_[4], "OTMExATM_CS") == 0) {
    unsigned int number_of_contracts = atoi(tokens_[5]);
    int length_of_steps = atoi(tokens_[6]);

    std::vector<std::string> t_call_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
        basename_, -1, 1, (number_of_contracts * length_of_steps + 1));
    std::vector<std::string> t_put_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
        basename_, -1, -1, (number_of_contracts * length_of_steps + 1));

    if (t_call_options_[0].compare(t_call_options_[0].size() - 2, 2, "_A") == 0) {
      t_call_options_.erase(t_call_options_.begin());
    }
    if (t_put_options_[0].compare(t_put_options_[0].size() - 2, 2, "_A") == 0) {
      t_put_options_.erase(t_put_options_.begin());
    }

    for (unsigned int i = (length_of_steps - 1); i < (number_of_contracts * length_of_steps); i += length_of_steps) {
      if (i < t_call_options_.size()) {
        options_.push_back(t_call_options_[i]);
      }
      if (i < t_put_options_.size()) {
        options_.push_back(t_put_options_[i]);
      }
    }
  } else if (tokens_.size() == 7 && strcmp(tokens_[4], "OTMInATM_CS") == 0) {
    unsigned int number_of_contracts = atoi(tokens_[5]);
    int length_of_steps = atoi(tokens_[6]);

    std::vector<std::string> t_call_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
        basename_, -1, 1, (number_of_contracts * length_of_steps));
    std::vector<std::string> t_put_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
        basename_, -1, -1, (number_of_contracts * length_of_steps));

    if (t_call_options_[0].compare(t_call_options_[0].size() - 2, 2, "_A") != 0) {
      t_call_options_.insert(t_call_options_.begin(),
                             std::string("NSE_") + std::string(basename_) + std::string("_C0_A"));
    }
    if (t_put_options_[0].compare(t_put_options_[0].size() - 2, 2, "_A") != 0) {
      t_put_options_.insert(t_put_options_.begin(),
                            std::string("NSE_") + std::string(basename_) + std::string("_P0_A"));
    }

    for (unsigned int i = (length_of_steps - 1); i < (number_of_contracts * length_of_steps); i += length_of_steps) {
      if (i < t_call_options_.size()) {
        options_.push_back(t_call_options_[i]);
      }
      if (i < t_put_options_.size()) {
        options_.push_back(t_put_options_[i]);
      }
    }
  } else if (tokens_.size() == 7 && strcmp(tokens_[4], "OTM_CS") == 0) {
    unsigned int number_of_contracts = atoi(tokens_[5]);
    int length_of_steps = atoi(tokens_[6]);
    std::vector<std::string> t_call_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
        basename_, -1, 1, (number_of_contracts * length_of_steps));
    std::vector<std::string> t_put_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInCurrScheme(
        basename_, -1, -1, (number_of_contracts * length_of_steps));

    for (unsigned int i = (length_of_steps - 1); i < (number_of_contracts * length_of_steps); i += length_of_steps) {
      if (i < t_call_options_.size()) {
        options_.push_back(t_call_options_[i]);
      }
      if (i < t_put_options_.size()) {
        options_.push_back(t_put_options_[i]);
      }
    }
  } else if (tokens_.size() == 7 && strcmp(tokens_[4], "OTMExATM_AS") == 0) {
    unsigned int number_of_contracts = atoi(tokens_[5]);
    int length_of_steps = atoi(tokens_[6]);
    std::vector<std::string> t_call_options_ = NSESecurityDefinitions::GetAllOptionShortcodesForUnderlying(
        basename_, -1, 1, (number_of_contracts * length_of_steps + 1));
    std::vector<std::string> t_put_options_ = NSESecurityDefinitions::GetAllOptionShortcodesForUnderlying(
        basename_, -1, -1, (number_of_contracts * length_of_steps + 1));

    if (t_call_options_[0].compare(t_call_options_[0].size() - 2, 2, "_A") == 0) {
      t_call_options_.erase(t_call_options_.begin());
    }
    if (t_put_options_[0].compare(t_put_options_[0].size() - 2, 2, "_A") == 0) {
      t_put_options_.erase(t_put_options_.begin());
    }

    for (unsigned int i = (length_of_steps - 1); i < (number_of_contracts * length_of_steps); i += length_of_steps) {
      if (i < t_call_options_.size()) {
        options_.push_back(t_call_options_[i]);
      }
      if (i < t_put_options_.size()) {
        options_.push_back(t_put_options_[i]);
      }
    }
  } else if (tokens_.size() == 7 && strcmp(tokens_[4], "OTM_AS") == 0) {
    unsigned int number_of_contracts = atoi(tokens_[5]);
    int length_of_steps = atoi(tokens_[6]);
    std::vector<std::string> t_call_options_ = NSESecurityDefinitions::GetAllOptionShortcodesForUnderlying(
        basename_, -1, 1, (number_of_contracts * length_of_steps + 1));
    std::vector<std::string> t_put_options_ = NSESecurityDefinitions::GetAllOptionShortcodesForUnderlying(
        basename_, -1, -1, (number_of_contracts * length_of_steps + 1));

    for (unsigned int i = (length_of_steps - 1); i < (number_of_contracts * length_of_steps); i += length_of_steps) {
      if (i < t_call_options_.size()) {
        options_.push_back(t_call_options_[i]);
      }
      if (i < t_put_options_.size()) {
        options_.push_back(t_put_options_[i]);
      }
    }

  } else if (tokens_.size() == 7 && strcmp(tokens_[4], "OTMExATM_PS") == 0) {
    unsigned int number_of_contracts = atoi(tokens_[5]);
    int length_of_steps = atoi(tokens_[6]);
    std::vector<std::string> t_call_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInPrevScheme(
        basename_, -1, 1, (number_of_contracts * length_of_steps + 1));
    std::vector<std::string> t_put_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInPrevScheme(
        basename_, -1, -1, (number_of_contracts * length_of_steps + 1));

    if (t_call_options_[0].compare(t_call_options_[0].size() - 2, 2, "_A") == 0) {
      t_call_options_.erase(t_call_options_.begin());
    }
    if (t_put_options_[0].compare(t_put_options_[0].size() - 2, 2, "_A") == 0) {
      t_put_options_.erase(t_put_options_.begin());
    }

    for (unsigned int i = (length_of_steps - 1); i < (number_of_contracts * length_of_steps); i += length_of_steps) {
      if (i < t_call_options_.size()) {
        options_.push_back(t_call_options_[i]);
      }
      if (i < t_put_options_.size()) {
        options_.push_back(t_put_options_[i]);
      }
    }
  } else if (tokens_.size() == 7 && strcmp(tokens_[4], "OTM_PS") == 0) {
    unsigned int number_of_contracts = atoi(tokens_[5]);
    int length_of_steps = atoi(tokens_[6]);
    std::vector<std::string> t_call_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInPrevScheme(
        basename_, -1, 1, (number_of_contracts * length_of_steps + 1));
    std::vector<std::string> t_put_options_ = NSESecurityDefinitions::GetOptionShortcodesForUnderlyingInPrevScheme(
        basename_, -1, -1, (number_of_contracts * length_of_steps + 1));

    for (unsigned int i = (length_of_steps - 1); i < (number_of_contracts * length_of_steps); i += length_of_steps) {
      if (i < t_call_options_.size()) {
        options_.push_back(t_call_options_[i]);
      }
      if (i < t_put_options_.size()) {
        options_.push_back(t_put_options_[i]);
      }
    }
  } else {
    for (unsigned int i = 4; i < tokens_.size(); i++) {
      options_.push_back(tokens_[i]);
    }
  }
  return options_;
}

// options weights file:
// one row line corresponds to one indicator ( so these are next to each other )
// one column line corresponds to one option
double MultModelCreator::GetTokenFromFile(std::string file_name_, unsigned int row_index_, int column_index_,
                                          std::vector<double>& row_line_) {
  double return_value_ = 1;
  if (file_name_.compare("DUMMY") == 0) {
    return return_value_;
  }
  static std::map<std::string, std::vector<double> > file_name_2_tokens_;
  static std::map<std::string, unsigned int> file_name_2_no_of_rows_;
  static std::map<std::string, unsigned int> file_name_2_no_of_columns_;

  if (file_name_2_tokens_.find(file_name_) != file_name_2_tokens_.end()) {
    // indices are sent considering 0 as starting index
    // ( row_index ) * ( no_of_columns_) + column_index_;
    unsigned int index_ = row_index_ * file_name_2_no_of_columns_[file_name_] + column_index_;
    if (file_name_2_tokens_[file_name_].size() > index_) {
      if (column_index_ > -1) {
        return_value_ = file_name_2_tokens_[file_name_][index_];
      } else {
        return_value_ = file_name_2_no_of_columns_[file_name_];
        row_line_ = std::vector<double>(file_name_2_tokens_[file_name_].begin() + index_ + 1,
                                        file_name_2_tokens_[file_name_].begin() + index_ + 1 + return_value_);
      }
    } else {
      std::cerr << "Requested token out of data size\n";
      exit(-1);
    }
  } else {
    std::ifstream wfile_;
    wfile_.open(file_name_.c_str(), std::ifstream::in);
    if (!wfile_.is_open()) {
      ExitVerbose(kStrategyDescModelFileMissing);
    } else {
      const unsigned int kBufferLen = 1024;
      char readline_buffer_[kBufferLen];

      while (wfile_.good()) {
        bzero(readline_buffer_, kBufferLen);
        wfile_.getline(readline_buffer_, kBufferLen);
        if (wfile_.gcount() > 0) {
          PerishableStringTokenizer st_(readline_buffer_, kBufferLen);
          const std::vector<const char*>& tokens_ = st_.GetTokens();

          if (tokens_.size() < 1 || tokens_[0][0] == '#') continue;

          if (file_name_2_no_of_columns_.find(file_name_) == file_name_2_no_of_columns_.end()) {
            file_name_2_no_of_columns_[file_name_] = tokens_.size();
            file_name_2_no_of_rows_[file_name_] = 0;
            file_name_2_tokens_[file_name_] = std::vector<double>();
          } else if (file_name_2_no_of_columns_[file_name_] != tokens_.size()) {
            ExitVerbose(kExitErrorCodeGeneral);
          }
          file_name_2_no_of_rows_[file_name_]++;

          for (auto i = 0u; i < tokens_.size(); i++) {
            file_name_2_tokens_[file_name_].push_back(atof(tokens_[i]));
          }
        }
      }

      wfile_.close();
      unsigned int index_ = row_index_ * file_name_2_no_of_columns_[file_name_] + column_index_;
      if (file_name_2_tokens_[file_name_].size() > index_) {
        if (column_index_ > -1) {
          return_value_ = file_name_2_tokens_[file_name_][index_];
        } else {
          return_value_ = file_name_2_no_of_columns_[file_name_];
          row_line_ = std::vector<double>(file_name_2_tokens_[file_name_].begin() + index_ + 1,
                                          file_name_2_tokens_[file_name_].begin() + index_ + 1 + return_value_);
        }
      } else {
        std::cerr << "Requested token out of data size\n";
        exit(-1);
      }
    }
  }
  return return_value_;
}
}
