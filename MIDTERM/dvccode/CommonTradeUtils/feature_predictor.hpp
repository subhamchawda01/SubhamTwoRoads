#ifndef BASE_COMMONTRADEUTILS_FEATUREPREDICTOR_H
#define BASE_COMMONTRADEUTILS_FEATUREPREDICTOR_H

#include <string>
#include <vector>
#include <stdlib.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {
class FeaturePredictor {
 private:
  std::string shortcode_;
  std::string modelfilename_;
  std::map<std::vector<std::string>, double> feature_weight_map_;
  std::string start_time_;
  std::string end_time_;
  std::string prev_session_start_time_;
  std::string prev_session_end_time_;
  bool live_trading_;
  std::string target_feature_;
  int date_;
  unsigned int start_mfm_;
  unsigned int end_mfm_;
  unsigned int prev_session_start_mfm_;
  unsigned int prev_session_end_mfm_;

 public:
  FeaturePredictor(std::string modelfilename, int _date_, const bool _live_trading_);
  void InitializeModel();
  double GetModelPrediction();
};
}

#endif  // BASE_COMMONTRADEUTILS_FEATUREPREDICTOR_H
