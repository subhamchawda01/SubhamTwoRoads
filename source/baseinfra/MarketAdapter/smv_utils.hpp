/**
   \file MarketAdapter/smv_utils.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MARKETADAPTER_SMV_UTILS_HPP
#define BASE_MARKETADAPTER_SMV_UTILS_HPP

#include <iostream>
#include <string>

#include "dvccode/CommonDataStructures/vector_utils.hpp"

#define ORS_REPLY_PRODUCTS_FILE "/spare/local/files/ors_reply_products.txt"

namespace HFSAT {

namespace SMVUtils {

class SMVUtils {
 private:
  std::map<std::string, bool> ors_reply_subscription_;

  void LoadOrsReplyProducts() {
    std::ifstream ors_reply_products_stream_;
    ors_reply_products_stream_.open(ORS_REPLY_PRODUCTS_FILE, std::ios::in);

    if (!ors_reply_products_stream_.is_open()) {
      return;
    }

    char line_buffer_[1024];

    while (ors_reply_products_stream_.good()) {
      memset(line_buffer_, 0, 1024);
      ors_reply_products_stream_.getline(line_buffer_, 1024);

      HFSAT::PerishableStringTokenizer st_(line_buffer_, 1024);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() < 1 || tokens_[0][0] == '#') continue;  // ignore comments and empty line

      std::string shortcode_ = tokens_[0];

      ors_reply_subscription_[shortcode_] = true;
    }
  }

 public:
  explicit SMVUtils() { LoadOrsReplyProducts(); }

  static inline SMVUtils& GetUniqueInstance() {
    static SMVUtils uniqueinstance_;
    return uniqueinstance_;
  }

  //  bool SubscribeToOrsReply(const std::string& this_shortcode_) {
  //    bool subscribe_to_ors_reply_ = false;
  //
  //    if (ors_reply_subscription_.find(this_shortcode_) != ors_reply_subscription_.end()) {
  //      subscribe_to_ors_reply_ = true;
  //    }
  //
  //    return subscribe_to_ors_reply_;
  //  }

  static inline double* GetTradeBasePriceParams(const char* filename, const std::string& t_shortcode_) {
    std::ifstream t_l1_size_infile_;
    t_l1_size_infile_.open(filename, std::ifstream::in);
    double* result = new double[5];
    bool shc_found_ = false;
    if (t_l1_size_infile_.is_open()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      while (t_l1_size_infile_.good()) {
        bzero(readline_buffer_, kL1AvgBufferLen);
        t_l1_size_infile_.getline(readline_buffer_, kL1AvgBufferLen);
        PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
        const std::vector<const char*>& tokens_ = st_.GetTokens();

        if (tokens_.size() >= 3) {
          std::string file_shortcode_ = tokens_[0];
          if ((t_shortcode_.compare(file_shortcode_) == 0)) {
            result[0] = atof(tokens_[1]);
            result[1] = atof(tokens_[2]);
            if (tokens_.size() >= 4)
              result[2] = atof(tokens_[3]);
            else
              result[2] = 500;
            if (tokens_.size() >= 5)
              result[3] = atof(tokens_[4]);
            else
              result[3] = 200;
            if (tokens_.size() >= 6)
              result[4] = atof(tokens_[5]);
            else
              result[4] = 0.4;
            shc_found_ = true;
          }
        }
      }
    }

    if (shc_found_ == false) {  // ensure that trade-px is never used, i.e. alpha remains at 0
      result[0] = 1000;
      result[1] = 10;
      result[2] = 0;
      result[3] = 200;
      result[4] = 0.25;
      // std::cerr << "Error GetTradeBasePriceParams: " << t_shortcode_ << " does not exist in the params file: " <<
      // filename << " ..Exiting" << std::endl;
      // std::exit(1);
    }

    return result;
  }
};
}
}

#endif  // BASE_MARKETADAPTER_SMV_UTILS_HPP
