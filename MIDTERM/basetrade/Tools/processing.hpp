#include "basetrade/Tools/timed_data_to_reg_data_common.hpp"

#ifndef BASE_TOOLS_PROCESSING_HPP
#define BASE_TOOLS_PROCESSING_HPP

namespace HFSAT {
class Processing {
 protected:
  SimpleLineProcessor& simple_line_processor_;
  bool is_returns_based_;
  int msecs_to_predict_;  // same as counters_to_predict
  DailyTradeVolumeReader* trade_vol_reader;
  int last_good_offset_;
  std::vector<std::ifstream*> ifstream_vec_;
  Processing(SimpleLineProcessor& _simple_line_processor_, const bool _is_returns_based_, const int _msecs_to_predict_,
             DailyTradeVolumeReader* _trade_vol_reader)
      : simple_line_processor_(_simple_line_processor_),
        is_returns_based_(_is_returns_based_),
        msecs_to_predict_(_msecs_to_predict_),
        trade_vol_reader(_trade_vol_reader),
        last_good_offset_(0),
        ifstream_vec_() {}

 public:
  virtual void Process(int t_pred_msecs_, int t_pred_events_, bool& were_files_reset_, int _last_good_offset_,
                       bool& _found_, double& pred_value_, double base_value_) {
    std::cout << "Parent is getting called\n";
  };

  void CloseFiles() {
    for (auto i = 0u; i < ifstream_vec_.size(); i++)
      if (ifstream_vec_[i]->is_open()) ifstream_vec_[i]->close();
  }
};
}

#endif
