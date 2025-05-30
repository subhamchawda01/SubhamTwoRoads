#include "dvccode/Utils/hybrid_sec.hpp"

namespace HFSAT {

std::tr1::unordered_set<std::string> HFSAT::HybridSecurityManager::hybrid_exch_secs_;
std::tr1::unordered_map<std::string, std::string> HFSAT::HybridSecurityManager::hybrid_exch_to_actual_exch_map_;
std::tr1::unordered_map<std::string, std::string> HFSAT::HybridSecurityManager::actual_exch_to_hybrid_exch_map_;

HybridSecurityManager::HybridSecurityManager(int trading_date, bool load_dated_files)
    : trading_date_(trading_date), load_dated_files_(load_dated_files) {
  LoadSecurityMaps();
}

HybridSecurityManager::HybridSecurityManager(int trading_date) : trading_date_(trading_date), load_dated_files_(false) {
  LoadSecurityMaps();
}

void HybridSecurityManager::LoadSecurityMaps() {
  // Load the p2y data for all securities
  // First read the mappings
  std::ifstream t_hyb_to_sec_file_;
  std::string hyb_file = std::string(BASEP2YINFODIR) + "/mapping.txt";

  t_hyb_to_sec_file_.open(hyb_file.c_str(), std::ifstream::in);

  if (t_hyb_to_sec_file_.is_open()) {
    const int buf_len_ = 1024;
    char readline_buffer_[buf_len_];
    unsigned int idx_ = 0;
    while (t_hyb_to_sec_file_.good()) {
      bzero(readline_buffer_, buf_len_);

      t_hyb_to_sec_file_.getline(readline_buffer_, buf_len_);

      HFSAT::PerishableStringTokenizer st_(readline_buffer_, buf_len_);
      const std::vector<const char *> &tokens = st_.GetTokens();
      if (tokens.size() == 2) {
        std::string t_hyb_sec_ = std::string(tokens[0]);
        std::string t_act_sec_ = std::string(tokens[1]);

        // Initialize the value for this contract in maps
        hybrid_secs_.insert(t_hyb_sec_);
        sec_to_idx_map_[t_hyb_sec_] = idx_;
        hybrid_to_actual_map_[t_hyb_sec_] = t_act_sec_;
        actual_to_hybrid_map_[t_act_sec_] = t_hyb_sec_;

        HFSAT::FastPriceConvertor *fast_price_convertor_ = new HFSAT::FastPriceConvertor(
            HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(t_act_sec_, trading_date_));

        price_convertors_.push_back(fast_price_convertor_);
        std::tr1::unordered_map<int, double> t_price_to_yield_map_;
        price_to_yield_maps_.push_back(t_price_to_yield_map_);

        if (!load_dated_files_ ||
            (load_dated_files_ && !LoadFromContractDateFiles(t_act_sec_, idx_, fast_price_convertor_))) {
          // If there is no entry for this shortcode in dated files, use the default values
          LoadFromDefaultFiles(t_act_sec_, idx_, fast_price_convertor_);
        }
        ++idx_;
      }
    }
    t_hyb_to_sec_file_.close();
  } else {
    std::cerr << "failed to open mapping file\n";
  }
}

/**
 *
 * @param actual_security
 * @param index
 * @param t_fast_price_convertor
 */
void HybridSecurityManager::LoadFromDefaultFiles(std::string actual_security, int index,
                                                 HFSAT::FastPriceConvertor *t_fast_price_convertor) {
  std::ifstream t_p2y_file_;

  std::string t_pure_basename_ = actual_security.substr(0, actual_security.find("_"));
  t_p2y_file_.open((std::string(BASEP2YINFODIR) + t_pure_basename_ + "_p2y").c_str(), std::ifstream::in);

  if (t_p2y_file_.is_open()) {
    const int buf_len_ = 1024;
    char inner_readline_buffer_[buf_len_];
    bzero(inner_readline_buffer_, buf_len_);
    t_p2y_file_.getline(inner_readline_buffer_, buf_len_);
    while (t_p2y_file_.good()) {
      bzero(inner_readline_buffer_, buf_len_);
      t_p2y_file_.getline(inner_readline_buffer_, buf_len_);
      double t_price_, t_yield_;
      int status = sscanf(inner_readline_buffer_, "%lf %lf", &t_price_, &t_yield_);
      if (status == 2) price_to_yield_maps_[index][t_fast_price_convertor->GetFastIntPx(t_price_)] = t_yield_;
    }
    t_p2y_file_.close();
  } else {
    std::cerr << "failed to open yield file for " << t_pure_basename_ << "\n";
  }
}

bool HybridSecurityManager::LoadFromContractDateFiles(std::string actual_securty, int index,
                                                      HFSAT::FastPriceConvertor *t_fast_price_convertor) {
  bool value_read = false;
  std::ifstream t_p2y_file;
  std::string t_p2y_filename =
      std::string(BASEP2YINFODIR) + "/yield_data/" + std::to_string(trading_date_) + "/" + actual_securty;
  t_p2y_file.open(t_p2y_filename.c_str(), std::ifstream::in);

  if (t_p2y_file.is_open()) {
    const int buf_len = 1024;
    char readline_buffer[buf_len];

    while (t_p2y_file.good()) {
      bzero(readline_buffer, buf_len);
      t_p2y_file.getline(readline_buffer, buf_len);

      char *exch_symbol;
      double t_price, t_yield;
      int tradingdate, days_to_maturity;
      exch_symbol = new char[20];
      int status = sscanf(readline_buffer, "%s %d %lf %d %lf ", exch_symbol, &tradingdate, &t_price, &days_to_maturity,
                          &t_yield);
      if (status == 5) {
        price_to_yield_maps_[index][t_fast_price_convertor->GetFastIntPx(t_price)] = t_yield;
        value_read = true;
      }
      // was just placeholder, remove this line if we are using it somewhere else as well
      delete exch_symbol;
    }
    t_p2y_file.close();
  } /*else {
    std::cerr << "Failed to open yield_file " << t_p2y_filename << std::endl;
  }*/
  return value_read;
}
};
