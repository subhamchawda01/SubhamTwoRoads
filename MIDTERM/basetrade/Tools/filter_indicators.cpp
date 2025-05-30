/**
    \file Tools/make_indicator_list.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#define MAX_TOKENS 8

// load exclude reg expression into map of 10 tokens each
//

std::vector<std::vector<std::string> > all_indicators_;
std::map<std::string, int> all_ind_map_;
std::vector<bool> exclude_vec_;
int start_idx_;
std::vector<std::string> start_lines_vec_;

int loadAndExclude(const char* shc_, const char* ex_regfile_) {
  HFSAT::BulkFileReader ex_reader_;
  char line[10240] = {0};
  ex_reader_.open(ex_regfile_);
  bool start_ = false;

  if (!ex_reader_.is_open()) {
    std::cout << "cannot open file: " << ex_regfile_ << std::endl;
    return -1;
  }
  int count = 0;
  while (ex_reader_.is_open()) {
    int iLength = ex_reader_.GetLine(line, 10240);

    if (iLength <= 0) {
      break;
    }

    HFSAT::PerishableStringTokenizer tokenize_(line, 10240);
    const std::vector<const char*> tokens_ = tokenize_.GetTokens();

    if (!start_) {
      if (tokens_.size() > 1 && strcmp(tokens_[0], "DEPENDENT_START") == 0 && strcmp(tokens_[1], shc_) == 0) {
        start_ = true;
      } else {
        continue;
      }
    }
    if (tokens_.size() > 1 && strcmp(tokens_[0], "DEPENDENT_END") == 0 && strcmp(tokens_[1], shc_) == 0) {
      break;
    }

    if (tokens_.size() <= 2 || (strcmp(tokens_[0], "#") == 0) || (strcmp(tokens_[0], "INDICATOR") != 0)) {
      continue;
    }

    std::vector<std::string> atok_(MAX_TOKENS, "_ALL_");
    for (unsigned int i = 2; i < tokens_.size(); i++) {
      if (strcmp(tokens_[i], "#") == 0) {
        break;
      }
      atok_[i - 2] = std::string(tokens_[i]);
    }

    std::cerr << "excluding " << atok_[0] << " " << atok_[1] << " " << atok_[2] << " " << atok_[3] << " " << atok_[4]
              << " " << atok_[5] << " " << atok_[6] << " " << atok_[7] << "\n";

    for (auto i = 0u; i < all_indicators_.size(); i++) {
      if (!exclude_vec_[i]) {
        for (unsigned int j = 2 + start_idx_; j < all_indicators_[i].size(); j++) {
          if (atok_[j - 2 - start_idx_].compare("_ALL_") == 0 ||
              (all_indicators_[i][j].compare(atok_[j - 2 - start_idx_]) ==
               0))  // did match || it is all => no decision can be made, it could still be excluded
          {
            // std::cout << all_indicators_[ i ][ j ] << " " << atok_[ j ] << "\n" ;
          } else  // didnt match, it couldnt be excluded, break out,  not point proceeding further
          {
            break;
          }

          if (j == all_indicators_[i].size() - 1)  // all have checked and didnt break out, qualifies for exclusion
          {
            count++;
            exclude_vec_[i] = true;
            break;
          }
        }
      }
    }
  }
  ex_reader_.close();
  return 1;
}

int readAllIndicators(const char* ind_filename_) {
  HFSAT::BulkFileReader reader_;
  reader_.open(ind_filename_);
  if (!reader_.is_open()) {
    std::cout << "cannot open file: " << ind_filename_ << std::endl;
    return -1;
  }
  int count = 0;
  while (reader_.is_open()) {
    count++;
    char line[1024] = {0};
    int len_Read = reader_.GetLine(line, 1024);
    if (len_Read <= 0) {
      break;
    }

    HFSAT::PerishableStringTokenizer tokenizer_(line, 1024);
    const std::vector<const char*> tokens_ = tokenizer_.GetTokens();

    bool start_phase_ = true;

    if (start_idx_ != 0) {
      start_phase_ = false;
    } else {
      start_lines_vec_.resize(3);
    }

    if (start_phase_) {
      if (strcmp(tokens_[0], "MODELINIT") == 0) {
        std::string temp_;
        auto i = 0u;
        for (; i < tokens_.size() - 1; i++) {
          temp_ += std::string(tokens_[i]) + " ";
        }
        temp_ += std::string(tokens_[i]);
        start_lines_vec_[0] = temp_;
        continue;
      }
      if (strcmp(tokens_[0], "MODELMATH") == 0) {
        std::string temp_;
        auto i = 0u;
        for (; i < tokens_.size() - 1; i++) {
          temp_ += std::string(tokens_[i]) + " ";
        }
        temp_ += std::string(tokens_[i]);
        start_lines_vec_[1] = temp_;
        continue;
      }
      if (strcmp(tokens_[0], "INDICATORSTART") == 0) {
        start_lines_vec_[2] = line;
        start_phase_ = false;
        continue;
      }
    }

    if ((int)tokens_.size() < (start_idx_ + 2) || strcmp(tokens_[start_idx_], "INDICATOR") != 0) {
      // std::cerr  << "didnt load " <<  count << " " << tokens_.size ( ) << " " << start_idx_ << " " << tokens_[
      // start_idx_ ] << "\n" ;
      continue;
    }

    std::vector<std::string> atok_;
    std::string key_;

    for (auto i = 0u; i < tokens_.size(); i++) {
      if (strcmp(tokens_[i], "#") == 0) {
        break;
      }
      if (i != 1 || start_idx_ != 0) {
        std::string s = std::string(tokens_[i]);
        char* endptr = 0;
        strtod(tokens_[i], &endptr);
        if (s.find('.') != std::string::npos)
          if (!(*endptr != '\0' || endptr == tokens_[i])) s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        atok_.push_back((s[s.size() - 1] == '.' ? s.substr(0, s.size() - 1) : s));
        key_ += (s[s.size() - 1] == '.' ? s.substr(0, s.size() - 1) : s) + " ";
      } else {
        atok_.push_back("1.00");
        key_ += "1.00 ";
      }
      //	  std::cerr <<  atok_[ i ] << "\n" ;
    }
    all_indicators_.push_back(atok_);
    all_ind_map_[key_] = all_indicators_.size() - 1;
    exclude_vec_.push_back(false);
  }
  reader_.close();
  return 1;
}

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << argv[0] << " SHORTCODE [-i/-c] INDICATOR_RECORD_FILE/ILIST EXCLUDE_FILENAME\n";
    exit(1);
  }

  const char* shortcode_ = argv[1];
  start_idx_ = (strcasecmp(argv[2], "-i") == 0 ? 0 : 1);
  const char* ifilename_ = argv[3];
  const char* exclude_filename_ = argv[4];

  // load all indicators
  readAllIndicators(ifilename_);
  //  std::cerr << "TOTAL COUNT READ " << all_indicators_.size ( ) << "\n" ;

  // match and exclude
  loadAndExclude(shortcode_, exclude_filename_);
  std::map<std::string, int>::iterator it_ = all_ind_map_.begin();

  if (start_idx_ == 0) {
    for (auto i = 0u; i < start_lines_vec_.size(); i++) {
      std::cout << start_lines_vec_[i] << "\n";
    }
    for (; it_ != all_ind_map_.end(); it_++) {
      if (exclude_vec_[(it_->second)]) {
        std::cerr << it_->first << "\n";
      } else {
        std::cout << it_->first << "\n";
      }
    }
    std::cout << "INDICATOREND"
              << "\n";
  } else if (start_idx_ == 1) {
    for (auto i = 0u; i < exclude_vec_.size(); i++) {
      if (exclude_vec_[i]) {
        for (unsigned int j = 0; j < all_indicators_[i].size(); j++) {
          std::cerr << all_indicators_[i][j] << " ";
        }
        std::cerr << "\n";
      } else {
        for (unsigned int j = 0; j < all_indicators_[i].size(); j++) {
          std::cout << all_indicators_[i][j] << " ";
        }
        std::cout << "\n";
      }
    }
  }
}
