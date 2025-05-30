/**
   \file ModelMathCode/md_model_creator.cpp

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
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/ModelMath/mult_model_creator.hpp"
#include "dvctrade/ModelMath/md_model_creator.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"

namespace HFSAT {

std::vector<SecurityMarketView*> MDModelCreator::dep_market_view_vec_;
std::vector<PriceType_t> MDModelCreator::baseprice_vec_;

void MDModelCreator::CollectMDModelShortCodes(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              std::string _instruction_filename_,
                                              std::vector<std::string>& dependant_shortcodes_,
                                              std::vector<std::string>& source_shortcodes_) {
  //  order of intialization for indicators
  SetIndicatorListMap();
  std::ifstream model_infile_;
  model_infile_.open(_instruction_filename_.c_str(), std::ifstream::in);

  std::vector<std::string> futures_;
  std::map<std::string, std::vector<std::string> > futures_2_options_maps_;
  // model_infile_.open(_model_filename_.c_str(), std::ifstream::in);
  // simply navigate through files and collect shortcodes
  if (!model_infile_.is_open()) {
    std::cerr << _instruction_filename_ << "\n";
    ExitVerbose(kModelCreationCouldNotOpenModelFile);
  } else {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    MDModelCreationPhases_t current_model_creation_phase_ = kMDModelCreationPhaseModelInit;
    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() < 1 || tokens_[0][0] == '#') {
          continue;
        }
        switch (current_model_creation_phase_) {
          case kMDModelCreationPhaseModelInit:
            if (strcmp(tokens_[0], "UNDERLYING") == 0) {
              current_model_creation_phase_ = kMDModelCreationPhaseShortCodes;
            } else {
              ExitVerbose(kModelCreationModelInitLineLessArgs, "UNDERLYING");
            }
            break;
          case kMDModelCreationPhaseShortCodes:
            if (tokens_.size() >= 3) {
              futures_.push_back(tokens_[0]);
              futures_2_options_maps_[tokens_[0]] = std::vector<std::string>();
              // VectorUtils::UniqueVectorAdd(dependant_shortcodes_, (std::string)tokens_[0]);
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
              current_model_creation_phase_ = kMDModelCreationPhaseIIndicators;
            } else {
              ExitVerbose(kModelCreationNoModelWeightIndicator, "IINDICATORS Missing");
            }
            break;
          case kMDModelCreationPhaseIIndicators:
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
              current_model_creation_phase_ = kMDModelCreationPhaseGIndicators;
            } else {
              ExitVerbose(kModelCreationModelMathLineLessArgs, "GINDICATORS Missing");
            }
            break;
          case kMDModelCreationPhaseGIndicators:
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
              current_model_creation_phase_ = kMDModelCreationPhaseEnd;
            } else {
              ExitVerbose(kModelCreationModelMathLineLessArgs, "MODELEND Missing");
            }
            break;
          case kMDModelCreationPhaseModelEnd:
            break;
          default:
            break;
        }
      }
    }
    model_infile_.close();
  }
}

CommonIndicator* MDModelCreator::GetIndicatorFromTokens(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                        const std::vector<const char*>& tokens_,
                                                        PriceType_t _dep_base_pricetype_) {
  return (GetUniqueInstanceFunc(tokens_[2]))(t_dbglogger_, cr_watch_, tokens_, _dep_base_pricetype_);
}

MDIndicatorLogger* MDModelCreator::CreateMDIndicatorLogger(DebugLogger& t_dbglogger_, const Watch& cr_watch_,
                                                           // BulkFileWriter& _bulk_file_writer_,
                                                           const std::string& _instruction_file_,
                                                           // const std::string& _output_file_,
                                                           const unsigned int t_msecs_to_wait_to_print_again_,
                                                           const unsigned int t_num_trades_to_wait_print_again_) {
  // SetIndicatorListMap();
  // number of lines = time + number_of_dependants + number_of_iindicator * number_of_dependants +
  // number_of_gindicators_
  std::vector<std::string> futures_;
  std::map<std::string, std::vector<std::string> > futures_2_options_maps_;

  std::ifstream model_infile_;
  model_infile_.open(_instruction_file_.c_str(), std::ifstream::in);
  MDIndicatorLogger* p_md_indicator_logger_ = nullptr;

  if (!model_infile_.is_open()) {
    std::cerr << _instruction_file_ << "\n";
    ExitVerbose(kModelCreationCouldNotOpenModelFile);
  } else {
    const unsigned int kModelLineBufferLen = 1024;
    char readline_buffer_[kModelLineBufferLen];
    bzero(readline_buffer_, kModelLineBufferLen);
    MDModelCreationPhases_t current_model_creation_phase_ = kMDModelCreationPhaseModelInit;
    SecurityMarketView* dep_market_view_ = nullptr;
    PriceType_t dep_base_pricetype_ = kPriceTypeImpliedVol;

    while (model_infile_.good()) {
      bzero(readline_buffer_, kModelLineBufferLen);
      model_infile_.getline(readline_buffer_, kModelLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kModelLineBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();
        if (tokens_.size() < 1 || tokens_[0][0] == '#') {
          continue;
        }
        switch (current_model_creation_phase_) {
          case kMDModelCreationPhaseModelInit:
            if (strcmp(tokens_[0], "UNDERLYING") == 0) {
              p_md_indicator_logger_ =
                  new MDIndicatorLogger(t_dbglogger_, cr_watch_,
                                        //_bulk_file_writer_,
                                        //_output_file_,
                                        t_msecs_to_wait_to_print_again_, t_num_trades_to_wait_print_again_);
              current_model_creation_phase_ = kMDModelCreationPhaseShortCodes;
            } else {
              ExitVerbose(kModelCreationModelInitLineLessArgs, "UNDERLYING");
            }
            break;
          case kMDModelCreationPhaseShortCodes:
            if (tokens_.size() >= 3) {
              futures_.push_back(tokens_[0]);
              futures_2_options_maps_[tokens_[0]] = std::vector<std::string>();
              dep_base_pricetype_ = StringToPriceType_t((std::string)tokens_[1]);
              baseprice_vec_.push_back(dep_base_pricetype_);
              // not adding future here
              // p_md_indicator_logger_->AddDependant(tokens_[0], tokens_[1]);
              {
                std::vector<std::string> options_ = HFSAT::MultModelCreator::GetOptionsToTrade(tokens_);
                for (auto i = 0u; i < options_.size(); i++) {
                  dep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(options_[i]);
                  p_md_indicator_logger_->AddDependant(dep_market_view_, dep_base_pricetype_);
                  futures_2_options_maps_[tokens_[0]].push_back(options_[i]);
                  dep_market_view_->SetBasePriceType(dep_base_pricetype_);
                  dep_market_view_vec_.push_back(dep_market_view_);
                }
              }
            } else if (strcmp(tokens_[0], "IINDICATORS") == 0) {
              current_model_creation_phase_ = kMDModelCreationPhaseIIndicators;
            } else {
              ExitVerbose(kModelCreationNoModelWeightIndicator, "IINDICATORS Missing");
            }
            break;
          case kMDModelCreationPhaseIIndicators:
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
                  CommonIndicator* _this_indicator_ =
                      GetIndicatorFromTokens(t_dbglogger_, cr_watch_, vc, baseprice_vec_[i]);
                  p_md_indicator_logger_->AddUnweightedIndicator(_this_indicator_, false);
                }
              }
            } else if (strcmp(tokens_[0], "GINDICATORS") == 0) {
              current_model_creation_phase_ = kMDModelCreationPhaseGIndicators;
            } else {
              ExitVerbose(kModelCreationModelMathLineLessArgs, "GINDICATORS Missing");
            }
            break;
          case kMDModelCreationPhaseGIndicators:
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
                CommonIndicator* _this_indicator_ =
                    GetIndicatorFromTokens(t_dbglogger_, cr_watch_, vc, baseprice_vec_[i]);
                p_md_indicator_logger_->AddUnweightedIndicator(_this_indicator_, true);
              }
            } else if (strcmp(tokens_[0], "MODELEND") == 0) {
              current_model_creation_phase_ = kMDModelCreationPhaseEnd;
            } else {
              ExitVerbose(kModelCreationModelMathLineLessArgs, "MODELEND Missing");
            }
            break;
          case kMDModelCreationPhaseModelEnd:
            break;
          default:
            break;
        }
      }
    }
    model_infile_.close();
    return p_md_indicator_logger_;
  }
  return p_md_indicator_logger_;
}

void MDModelCreator::LinkupLoggerToOnReadySources(MDIndicatorLogger* p_md_logger_,
                                                  std::vector<std::string>& shortcodes_affecting_this_model_) {
  if (p_md_logger_ != nullptr) {
    for (auto i = 0u; i < shortcodes_affecting_this_model_.size(); i++) {
      (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(shortcodes_affecting_this_model_[i]))
          ->subscribe_OnReady(p_md_logger_);
    }
  }
}
}
