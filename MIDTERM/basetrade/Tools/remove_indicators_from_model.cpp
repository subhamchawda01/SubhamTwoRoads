/*
 * remove_indicators_from_forest_model.cpp
 *
 *  Created on: Aug 12, 2015
 *      Author: archit
 */

#include "dvctrade/CommonTradeDataStructures/forest.hpp"
#include "dvctrade/CommonInterfaces/indicator_set.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvccode/Utils/model_utils.hpp"

void RemoveIndicatorByIndSubStr(HFSAT::IndicatorSet& _ind_set_, const std::string& _ind_sub_str_) {
  const std::vector<HFSAT::Indicator*>& p_ind_vec_ = _ind_set_.GetIndPtrVec();
  for (auto i = 0u; i < p_ind_vec_.size(); i++) {
    if (p_ind_vec_[i]->line_.find(_ind_sub_str_) != std::string::npos) {
      _ind_set_.RemoveIndicatorByIdx(i);
    }
  }
}

void RemoveIndicatorByShc(HFSAT::IndicatorSet& _ind_set_, const std::string& _shc_) {
  std::string substr_ = " " + _shc_ + " ";
  RemoveIndicatorByIndSubStr(_ind_set_, substr_);
}

void RemoveIndicatorByExchange(HFSAT::IndicatorSet& _ind_set_, HFSAT::ExchSource_t _exch_) {
  const std::vector<HFSAT::Indicator*>& p_ind_vec_ = _ind_set_.GetIndPtrVec();
  for (auto i = 0u; i < p_ind_vec_.size(); i++) {
    if (HFSAT::IndicatorUtil::IndicatorHasExch(p_ind_vec_[i]->line_, _exch_)) {
      _ind_set_.RemoveIndicatorByIdx(i);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cout << "USAGE: " << argv[0] << " modelfile remove_by(IND_IDX|SHC|IND_SUBSTR|EXCH) remove_1 remove_2 ....\n";
    std::cout << "Example: " << argv[0] << " modelfile EXCH ICE LIFFE\n";
    std::cout << "Example: " << argv[0] << " modelfile SHC FBTP_0 FBTS_0\n";
    std::cout << "Example: " << argv[0] << " modelfile IND_SUBSTR SimpleTrend\n";
    std::cout << "Example: " << argv[0] << " modelfile IND_IDX 8 9 11\n";
    exit(1);
  }

  std::string forest_file_name_ = std::string(argv[1]);
  std::string type_ = std::string(argv[2]);

  HFSAT::PcaWeightsManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateUTC());

  std::string modelmath_str_ = HFSAT::ModelUtils::GetModelMathStr(forest_file_name_);
  HFSAT::IndicatorSet* p_ind_set_;

  if (modelmath_str_ == "RANDOMFOREST" || modelmath_str_ == "TREEBOOSTING") {
    p_ind_set_ = new HFSAT::Forest(forest_file_name_);
  } else {
    p_ind_set_ = new HFSAT::ModelLineSet(forest_file_name_);
  }

  for (unsigned int i = 3; int(i) < argc; i++) {
    if (type_ == "IND_IDX") {
      int ind_idx_ = atoi(argv[i]);
      p_ind_set_->RemoveIndicatorByIdx(ind_idx_);
    } else if (type_ == "SHC") {
      RemoveIndicatorByShc(*p_ind_set_, std::string(argv[i]));
    } else if (type_ == "IND_SUBSTR") {
      RemoveIndicatorByIndSubStr(*p_ind_set_, std::string(argv[i]));
    } else if (type_ == "EXCH") {
      RemoveIndicatorByExchange(*p_ind_set_, HFSAT::StringToExchSource(std::string(argv[i])));
    }
  }
  std::cout << p_ind_set_->ToString();
  return 0;
}
