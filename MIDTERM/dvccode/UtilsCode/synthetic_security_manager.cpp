#include "dvccode/Utils/synthetic_security_manager.hpp"

namespace HFSAT {

SyntheticSecurityManager *SyntheticSecurityManager::p_uniqueinstance_ = NULL;

SyntheticSecurityManager::SyntheticSecurityManager() { LoadSynthVecs(); }

void SyntheticSecurityManager::LoadSynthVecs() {
  std::ifstream t_hyb_to_sec_file_;
  t_hyb_to_sec_file_.open("/spare/local/tradeinfo/SyntheticInfo/mapping.txt", std::ifstream::in);
  if (t_hyb_to_sec_file_.is_open()) {
    const int buf_len_ = 1024;
    char readline_buffer_[buf_len_];
    while (t_hyb_to_sec_file_.good()) {
      bzero(readline_buffer_, buf_len_);
      t_hyb_to_sec_file_.getline(readline_buffer_, buf_len_);
      HFSAT::PerishableStringTokenizer st_(readline_buffer_, buf_len_);
      const std::vector<const char *> &tokens = st_.GetTokens();

      if (tokens.size() > 1) {
        std::string t_synth_shc_ = std::string(tokens[0]);

        std::vector<std::string> t_const_shc_vec_;
        std::vector<double> t_const_weight_vec_;
        bool reading_shc_ = false;
        bool reading_weights_ = false;

        unsigned int tokens_idx_ = 1;

        while (tokens_idx_ < tokens.size()) {
          if (!reading_shc_ && !reading_weights_) {
            if (!std::string(tokens[tokens_idx_]).compare("SHORTCODES")) {
              reading_shc_ = true;
              tokens_idx_++;
              continue;
            }
          } else if (reading_shc_) {
            if (!std::string(tokens[tokens_idx_]).compare("WEIGHTS")) {
              reading_shc_ = false;
              reading_weights_ = true;
              tokens_idx_++;
              continue;
            } else {
              t_const_shc_vec_.push_back(std::string(tokens[tokens_idx_]));
              tokens_idx_++;
            }
          } else if (reading_weights_) {
            t_const_weight_vec_.push_back(atof(tokens[tokens_idx_]));
            tokens_idx_++;
          }
        }

        if (t_const_shc_vec_.size() == t_const_weight_vec_.size() && t_const_shc_vec_.size() > 1) {
          synth_shc_to_const_shc_vec_[t_synth_shc_] = t_const_shc_vec_;
          synth_shc_to_const_weight_vec_[t_synth_shc_] = t_const_weight_vec_;
          synth_secs_.insert(t_synth_shc_);
        } else {
          std::cerr << "Failed to load weights and shc\n";
        }
      }
    }
    t_hyb_to_sec_file_.close();
  } else {
    std::cerr << "failed to open mapping file\n";
  }
}
}
