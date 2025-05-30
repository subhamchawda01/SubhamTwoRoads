/**
    \file MToolsExe/mtools_utils.cpp

getgetSemanticSignForAnIndicatorSemanticSignForAnIndicator    \author: (c) Copyright Two Roads Technological Solutions
Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_MTOOLS_UTILS_H
#define BASE_MTOOLS_UTILS_H

#include <string.h>
#include <vector>
#include <stdlib.h>
#include <fstream>
#include <iostream>

#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "basetrade/MTools/data_processing.hpp"

#define INDICATOR_SEMANTICS_FILE "/spare/local/tradeinfo/modellinginfo/indicator_signs.txt"
#define DEP_INDEP_SEMANTICS_FILE "/spare/local/tradeinfo/modellinginfo/dep_indep_relationships.txt"
#define HISTORICAL_CORR_FILE_PREFIX "/spare/local/tradeinfo/modellinginfo/historical_corr_"

#define IGNORE_VALUE -999

namespace HFSAT {
namespace MTOOLS_UTILS {

struct semantic_value_lookfurther_index_t {
  int semantic_value_;
  int look_further_index_;
};

// declartion //
void loadIndicatorHashCorrelations(const std::string &,
                                   std::vector<double> &);  // indicator_list_file_name && correlations holder

void loadIndicatorSemantics(const std::string &, std::vector<int> &);  // indicator_list_file_name && semantics holder
void loadDepIndepSemanticsMap(std::string, std::map<std::string, int> &);
void loadIndicatorSemanticsMap(std::map<std::string, semantic_value_lookfurther_index_t> &);
bool loadIndicatorHistoricalCorrMap(std::string shortcode_, std::map<std::string, double> &);
void loadIndicatorHistoricalCorrelations(const std::string &, std::vector<double> &);

void filterIndicatorsBasedOnHashCorrelations(const unsigned int, const std::vector<double> &, const double,
                                             const std::vector<double> &, std::vector<int> &);
void filterIndicatorsBasedOnSemantics(const unsigned int, const std::vector<double> &, const std::vector<int> &,
                                      std::vector<int> &, bool ignore_not_found_ = true);
void filterIndicatorsBasedOnHistoricalCorrelations(const unsigned int, const std::vector<double> &, const double,
                                                   const std::vector<double> &, std::vector<int> &,
                                                   bool ignore_not_found_ = true);
void LoadAvoidCheckfile(const std::string &, std::vector<int> &);

// implementation //

bool loadIndicatorHistoricalCorrMap(std::string shortcode_,
                                    std::map<std::string, double> &indicator_historical_corr_map_) {
  std::string historical_corr_filename_ = std::string(HISTORICAL_CORR_FILE_PREFIX) + shortcode_;
  if (HFSAT::FileUtils::ExistsAndReadable(historical_corr_filename_)) {
    std::ifstream historical_corr_infile_;
    historical_corr_infile_.open(historical_corr_filename_.c_str(), std::ifstream::in);
    if (!historical_corr_infile_.is_open()) {
      std::cerr << " historical_corr_file " << historical_corr_filename_ << " cannot be opened\n";
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kIListLineBufferLen = 1024;
    char readline_buffer_[kIListLineBufferLen];
    bzero(readline_buffer_, kIListLineBufferLen);

    while (historical_corr_infile_.good()) {
      bzero(readline_buffer_, kIListLineBufferLen);
      historical_corr_infile_.getline(readline_buffer_, kIListLineBufferLen);
      if (historical_corr_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kIListLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &tokens_ = st_.GetTokens();
        if (tokens_.size() < 1) {
          continue;
        }
        if (strcmp(tokens_[0], "INDICATOR") == 0) {
          if (tokens_.size() >= 4) {
            double t_corr_ = atof(tokens_[1]);
            std::string t_indicator_string_;
            for (unsigned int i = 3; i < tokens_.size(); i++) {
              if (strcmp(tokens_[i], "#") == 0) {
                break;
              }
              t_indicator_string_ = t_indicator_string_ + tokens_[i];
            }
            indicator_historical_corr_map_[t_indicator_string_] = t_corr_;
          }
        }
      }
    }
    historical_corr_infile_.close();
    return true;
  } else {
    std::cout << " historical_corr_file " << historical_corr_filename_
              << " doesn't exist. Please contact KP/AB to add it. For now historic corr check may be muted \n";
    return false;
  }
}

void loadIndicatorSignsFromIlist(const std::string &indicator_filename_, std::vector<double> &indicator_correlations_) {
  if (indicator_filename_.compare("INVALIDFILE") == 0) {
    std::cout << "INVALIDFILE specified for ilist/modelfile needed by ilist corr check\n";
    return;
  }

  if (HFSAT::FileUtils::ExistsAndReadable(indicator_filename_)) {
    // parse indicator file and get the historical correlations of possible indicator
    std::ifstream model_infile_;
    model_infile_.open(indicator_filename_.c_str(), std::ifstream::in);
    if (!model_infile_.is_open()) {
      std::cout << " input indicator list file " << indicator_filename_ << " cannot be opened " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kIListLineBufferLen = 1024;
    char readline_buffer_[kIListLineBufferLen];
    bzero(readline_buffer_, kIListLineBufferLen);
    while (model_infile_.good()) {
      bzero(readline_buffer_, kIListLineBufferLen);
      model_infile_.getline(readline_buffer_, kIListLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kIListLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }
        if (strcmp(tokens_[0], "INDICATOR") == 0) {
          if (tokens_.size() >= 3) {
            if (atof(tokens_[1]) > 0) {
              indicator_correlations_.push_back(1);
            } else if (atof(tokens_[1]) < 0) {
              indicator_correlations_.push_back(-1);
            } else {
              indicator_correlations_.push_back(0);
            }
          }
        }
      }
    }
  }
}

void loadIndicatorHistoricalCorrelations(const std::string &indicator_filename_,
                                         std::vector<double> &indicator_correlations_) {
  if (indicator_filename_.compare("INVALIDFILE") == 0) {
    std::cout << "INVALIDFILE specified for ilist/modelfile needed by hist corr check. Hist Corr Check maybe muted\n";
    return;
  }

  if (HFSAT::FileUtils::ExistsAndReadable(indicator_filename_)) {
    // parse indicator file and get the historical correlations of possible indicator
    std::ifstream model_infile_;
    model_infile_.open(indicator_filename_.c_str(), std::ifstream::in);
    if (!model_infile_.is_open()) {
      std::cerr << " input indicator list file " << indicator_filename_ << " cannot be opened " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kIListLineBufferLen = 1024;
    char readline_buffer_[kIListLineBufferLen];
    bzero(readline_buffer_, kIListLineBufferLen);

    std::map<std::string, double> indicator_historical_corr_map_;

    std::string dep_shortcode_ = "NONAME";  // placeholder for the shortcode
    bool hist_corr_map_loaded_ = false;
    while (model_infile_.good()) {
      bzero(readline_buffer_, kIListLineBufferLen);
      model_infile_.getline(readline_buffer_, kIListLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kIListLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }
        if (strcmp(tokens_[0], "MODELINIT") == 0) {
          dep_shortcode_ = std::string(tokens_[2]);
          hist_corr_map_loaded_ = loadIndicatorHistoricalCorrMap(dep_shortcode_, indicator_historical_corr_map_);
        }
        if (strcmp(tokens_[0], "INDICATOR") == 0) {
          if (tokens_.size() >= 3) {
            if (!hist_corr_map_loaded_) {
              // if map cant be loaded properly then muting hist corr check
              indicator_correlations_.push_back(0);
              continue;
            }
            std::string t_indicator_string_;
            for (unsigned int i = 2; i < tokens_.size(); i++) {
              if (strcmp(tokens_[i], "#") == 0) {
                break;
              }
              t_indicator_string_ = t_indicator_string_ + tokens_[i];
            }
            if (indicator_historical_corr_map_.find(t_indicator_string_) != indicator_historical_corr_map_.end()) {
              double t_hist_corr_ = indicator_historical_corr_map_[t_indicator_string_];
              if (t_hist_corr_ < 0) {
                indicator_correlations_.push_back(-1);
                continue;
              } else if (t_hist_corr_ > 0) {
                indicator_correlations_.push_back(1);
                continue;
              }
              indicator_correlations_.push_back(0);
              continue;

            } else {
              std::cout << "can't find historical correlation of " << t_indicator_string_ << std::endl;
              indicator_correlations_.push_back(IGNORE_VALUE);
              continue;
            }
          }
        }
      }
    }
    model_infile_.close();
  } else {
    std::cout << "file doesnot exists or unreadable " << indicator_filename_ << " .Hist Corr Check maybe muted\n";
  }
}

void loadIndicatorHashCorrelations(const std::string &indicator_filename_,
                                   std::vector<double> &indicator_correlations_) {
  if (indicator_filename_.compare("INVALIDFILE") == 0) {
    return;
  }
  if (HFSAT::FileUtils::ExistsAndReadable(indicator_filename_)) {
    // parse indicator file and get the historical correlations of possible indicator
    std::ifstream model_infile_;
    model_infile_.open(indicator_filename_.c_str(), std::ifstream::in);
    if (!model_infile_.is_open()) {
      std::cerr << " input indicator list file " << indicator_filename_ << " cannot be opened " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kIListLineBufferLen = 1024;
    char readline_buffer_[kIListLineBufferLen];
    bzero(readline_buffer_, kIListLineBufferLen);

    std::string dep_shortcode_ = "NONAME";  // placeholder for the shortcode
    while (model_infile_.good()) {
      bzero(readline_buffer_, kIListLineBufferLen);
      model_infile_.getline(readline_buffer_, kIListLineBufferLen);
      if (model_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kIListLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }
        if (strcmp(tokens_[0], "MODELINIT") == 0) {
          dep_shortcode_ = std::string(tokens_[2]);
        }
        if (strcmp(tokens_[0], "INDICATOR") == 0) {
          int hash_index_ = -1;
          for (unsigned int i = 1; i < tokens_.size(); i++) {
            if (strcmp(tokens_[i], "#") == 0) {
              hash_index_ = (int)i;
              break;
            }
          }
          if (hash_index_ == -1) {
            indicator_correlations_.push_back(0);
          } else {
            // std::cerr << " hashindex = " << hash_index_ << " sz = " << tokens_.size() << std::endl;
            if ((int)tokens_.size() > (hash_index_ + 2)) {  // format sort-value mean-value
              indicator_correlations_.push_back(atof(tokens_[hash_index_ + 2]));
              // std::cerr << " taken meancorr = " << atof ( tokens_[hash_index_ + 2] ) << std::endl;
            } else {
              if ((int)tokens_.size() > (hash_index_ + 1)) {  // format sort-value proportional to mean-value
                indicator_correlations_.push_back(atof(tokens_[hash_index_ + 1]));
                // std::cerr << " taken sortval = " << atof ( tokens_[hash_index_ + 1] ) << std::endl;
              } else {  // no other word
                indicator_correlations_.push_back(0);
              }
            }
          }
        }
      }
    }
    model_infile_.close();
  }
}

void loadIndicatorSemantics(const std::string &t_indicator_filename_, std::vector<int> &t_semantics_) {
  if (t_indicator_filename_.compare("INVALIDFILE") == 0)  // why are we here if it is INVALID, wasted function call !
  {
    std::cout << "INVALIDFILE specified for ilist/modelfile needed by semantic check. Semantic Check maybe muted\n";
    return;
  }

  std::map<std::string, int> dep_indep_semantics_map_;
  std::map<std::string, semantic_value_lookfurther_index_t> indicators_semantics_map_;
  if (HFSAT::FileUtils::ExistsAndReadable(t_indicator_filename_)) {
    // parse indicator file and get the indicator strings
    std::ifstream ilist_infile_;
    ilist_infile_.open(t_indicator_filename_.c_str(), std::ifstream::in);
    if (!ilist_infile_.is_open()) {
      std::cerr << " input indicator list file " << t_indicator_filename_ << " cannot be opened " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kIListLineBufferLen = 1024;
    char readline_buffer_[kIListLineBufferLen];
    bzero(readline_buffer_, kIListLineBufferLen);

    loadIndicatorSemanticsMap(indicators_semantics_map_);  //
    std::string dep_shortcode_ = "NONAME";                 // placeholder for the shortcode

    while (ilist_infile_.good()) {
      bzero(readline_buffer_, kIListLineBufferLen);
      ilist_infile_.getline(readline_buffer_, kIListLineBufferLen);
      if (ilist_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kIListLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }
        if (strcmp(tokens_[0], "MODELINIT") == 0) {
          dep_shortcode_ = std::string(tokens_[2]);
          loadDepIndepSemanticsMap(dep_shortcode_, dep_indep_semantics_map_);
        }
        if (strcmp(tokens_[0], "INDICATOR") == 0) {
          if (tokens_.size() > 2)  // atleast there is indicator_name
          {
            if (indicators_semantics_map_.find(tokens_[2]) != indicators_semantics_map_.end()) {
              int t_semantic_value_ = 0;
              unsigned int t_look_futher_index_ = 3;
              t_semantic_value_ = indicators_semantics_map_[tokens_[2]].semantic_value_;

              if (t_semantic_value_ == 2 || t_semantic_value_ == -2)  // dont look further
              {
                t_semantics_.push_back(t_semantic_value_ / abs(t_semantic_value_));
                continue;
              }
              if (t_semantic_value_ == 1 || t_semantic_value_ == -1)  // look futher
              {
                t_look_futher_index_ = t_look_futher_index_ + indicators_semantics_map_[tokens_[2]].look_further_index_;
                if (tokens_.size() > t_look_futher_index_) {
                  if (dep_indep_semantics_map_.find(tokens_[t_look_futher_index_]) != dep_indep_semantics_map_.end()) {
                    t_semantics_.push_back(dep_indep_semantics_map_[tokens_[t_look_futher_index_]] * t_semantic_value_);
                    continue;
                  } else {
                    t_semantics_.push_back(IGNORE_VALUE);
                    continue;
                  }
                }
              }
              t_semantics_.push_back(0);
            } else {
              t_semantics_.push_back(IGNORE_VALUE);
            }
          }
        }
      }
    }
    ilist_infile_.close();
  } else {
    std::cout << "file doesnot exists or unreadable " << t_indicator_filename_ << " .Semantic Check maybe muted\n";
  }
}

void loadDepIndepSemanticsMap(std::string dep_shortcode_, std::map<std::string, int> &indep_semantics_map_) {
  std::ostringstream t_oss_;
  t_oss_ << std::string(DEP_INDEP_SEMANTICS_FILE);
  std::string semantic_sign_filename_ = t_oss_.str();

  if (HFSAT::FileUtils::ExistsAndReadable(semantic_sign_filename_)) {
    std::ifstream semantic_infile_;
    semantic_infile_.open(semantic_sign_filename_.c_str(), std::ifstream::in);
    if (!semantic_infile_.is_open()) {
      std::cerr << "dep indep semantic file " << semantic_sign_filename_ << " failed to open " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kIListLineBufferLen = 1024;
    char readline_buffer_[kIListLineBufferLen];

    while (semantic_infile_.good()) {
      bzero(readline_buffer_, kIListLineBufferLen);
      semantic_infile_.getline(readline_buffer_, kIListLineBufferLen);

      if (semantic_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kIListLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() < 1) {  // skip empty lines
          continue;
        }
        if (strcmp(tokens_[0], dep_shortcode_.c_str()) == 0) {
          indep_semantics_map_[std::string(tokens_[1])] = atoi(tokens_[2]);
        }
      }
    }
    semantic_infile_.close();
  } else {
    std::cout << "dep indep semantic file " << semantic_sign_filename_
              << " doesn't exist. Please contact KP/AB to add it. Semantic Check may mute everything. " << std::endl;
  }
}

void loadIndicatorSemanticsMap(std::map<std::string, semantic_value_lookfurther_index_t> &indicator_semantics_map_) {
  std::ostringstream t_oss_;
  t_oss_ << std::string(INDICATOR_SEMANTICS_FILE);
  std::string semantic_sign_filename_ = t_oss_.str();

  if (HFSAT::FileUtils::ExistsAndReadable(semantic_sign_filename_)) {
    std::ifstream semantic_infile_;
    semantic_infile_.open(semantic_sign_filename_.c_str(), std::ifstream::in);
    if (!semantic_infile_.is_open()) {
      std::cerr << "indicator semantic file " << semantic_sign_filename_ << " cannot be opened " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }

    const unsigned int kIListLineBufferLen = 1024;
    char readline_buffer_[kIListLineBufferLen];

    while (semantic_infile_.good()) {
      bzero(readline_buffer_, kIListLineBufferLen);
      semantic_infile_.getline(readline_buffer_, kIListLineBufferLen);

      if (semantic_infile_.gcount() > 0) {
        PerishableStringTokenizer st_(readline_buffer_, kIListLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &tokens_ = st_.GetTokens();

        if (tokens_.size() != 4) {  // skip empty lines
          continue;
        }
        if (strcmp(tokens_[0], "INDICATOR") == 0) {
          semantic_value_lookfurther_index_t t_ivl_;
          t_ivl_.semantic_value_ = atoi(tokens_[2]);
          t_ivl_.look_further_index_ = atoi(tokens_[3]);
          indicator_semantics_map_[std::string(tokens_[1])] = t_ivl_;
        }
      }
    }
    semantic_infile_.close();
  } else {
    std::cout << "indicator semantic file " << semantic_sign_filename_
              << " doesn't exist. Please contact KP/AB to add it. Semantic Check may mute everything. " << std::endl;
  }
}

void LoadAvoidCheckfile(const std::string &filename_, std::vector<int> &avoid_check_index_vector) {
  if (filename_.compare("INVALIDFILE") == 0) return;
  if (HFSAT::FileUtils::ExistsAndReadable(filename_)) {
    // read indices from file ... there may be nothing
    std::ifstream infile_;
    infile_.open(filename_.c_str());
    if (!infile_.is_open()) {
      std::cerr << " Input data file " << filename_ << " did not open " << std::endl;
      exit(kCallIterativeRegressInFileOpenError);
    }
    char buf[2048] = {0};

    while (infile_.good()) {
      infile_.getline(buf, 2048);
      PerishableStringTokenizer st_(buf, 2048);  // Perishable string
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      for (auto i = 0u; i < tokens_.size(); ++i) {
        avoid_check_index_vector[atoi(tokens_[i])] = 1;
      }
    }

    infile_.close();
  }
}

void filterIndicatorsBasedOnHashCorrelations(const unsigned int sz_independents,
                                             const std::vector<double> &initial_correlations_,
                                             const double min_correlation_,
                                             const std::vector<double> &indicator_correlations_,
                                             std::vector<int> &in_included_) {
  bool check_historical_correlation_signs_ = false;
  if (indicator_correlations_.size() >= sz_independents) {
    check_historical_correlation_signs_ = true;
  }  // so that array indices referenced don't cause segfaults

  // removing indicators from consideration whose initial correlation is also lower thant the min_correlation_ required
  // to add an idnicator
  for (unsigned int indep_index_ = 0u; indep_index_ < sz_independents; indep_index_++) {
    if (in_included_[indep_index_] == 0) {
      if (fabs(initial_correlations_[indep_index_]) < min_correlation_) {
        in_included_[indep_index_] =
            -1;  // making indicator ineligible to be selected since it's outright correlation is very low
        continue;
      }
    }

    if (check_historical_correlation_signs_) {
      // std::cerr << "sign of correlation ( index: " << indep_index_ << " ) expected " << indicator_correlations_ [
      // indep_index_ ]
      // 		<< ", actual value " << initial_correlations_ [ indep_index_ ] << std::endl;
      if ((indicator_correlations_[indep_index_] < 0) && (initial_correlations_[indep_index_] > 0)) {
        in_included_[indep_index_] = -1;  // sign different than expected
        std::cout << "sign of correlation ( index: " << indep_index_ << " ) expected -ve "
                  << indicator_correlations_[indep_index_] << ", actual value " << initial_correlations_[indep_index_]
                  << std::endl;
      } else if ((indicator_correlations_[indep_index_] > 0) && (initial_correlations_[indep_index_] < 0)) {
        in_included_[indep_index_] = -1;  // sign different than expected
        std::cout << "sign of correlation ( index: " << indep_index_ << " ) expected +ve "
                  << indicator_correlations_[indep_index_] << ", actual value " << initial_correlations_[indep_index_]
                  << std::endl;
      }
    }
  }
}

void filterIndicatorsBasedOnSemantics(const unsigned int sz_independents,
                                      const std::vector<double> &initial_correlations_,
                                      const std::vector<int> &semantic_indicator_correlations_,
                                      std::vector<int> &in_included_, bool ignore_not_found_ /*=true*/) {
  bool check_semantic_correlation_signs_ = false;
  if (semantic_indicator_correlations_.size() >= sz_independents) {
    check_semantic_correlation_signs_ = true;
  }

  for (unsigned int indep_index_ = 0u; indep_index_ < sz_independents; indep_index_++) {
    if (check_semantic_correlation_signs_) {
      if (semantic_indicator_correlations_[indep_index_] == IGNORE_VALUE) {
        if (ignore_not_found_) {
          in_included_[indep_index_] = -1;
          std::cout << "ignoring index : " << indep_index_ << ", as no semantic sign found\n";
        }
      } else if ((initial_correlations_[indep_index_] > 0) && (semantic_indicator_correlations_[indep_index_] < 0)) {
        in_included_[indep_index_] = -1;
        std::cout << "sign of semantic correlation ( index: " << indep_index_ << " ) expected -ve "
                  << semantic_indicator_correlations_[indep_index_] << ", actual value "
                  << initial_correlations_[indep_index_] << std::endl;
      } else if ((semantic_indicator_correlations_[indep_index_] > 0) && (initial_correlations_[indep_index_] < 0)) {
        in_included_[indep_index_] = -1;  // sign different than expected
        std::cout << "sign of semantic correlation ( index: " << indep_index_ << " ) expected +ve "
                  << semantic_indicator_correlations_[indep_index_] << ", actual value "
                  << initial_correlations_[indep_index_] << std::endl;
      }
    }
  }
}

void filterIndicatorsBasedOnHistoricalCorrelations(const unsigned int sz_independents,
                                                   const std::vector<double> &initial_correlations_,
                                                   const double min_correlation_,
                                                   const std::vector<double> &indicator_correlations_,
                                                   std::vector<int> &in_included_, bool ignore_not_found_ /*=true*/) {
  bool check_historical_correlation_signs_ = false;
  if (indicator_correlations_.size() >= sz_independents) {
    check_historical_correlation_signs_ = true;
  }  // so that array indices referenced don't cause segfaults

  // removing indicators from consideration whose initial correlation is also lower thant the min_correlation_ required
  // to add an idnicator
  for (unsigned int indep_index_ = 0u; indep_index_ < sz_independents; indep_index_++) {
    if (in_included_[indep_index_] == 0) {
      if (fabs(initial_correlations_[indep_index_]) < min_correlation_) {
        in_included_[indep_index_] =
            -1;  // making indicator ineligible to be selected since it's outright correlation is very low
        continue;
      }
    }

    if (check_historical_correlation_signs_) {
      if (indicator_correlations_[indep_index_] == IGNORE_VALUE) {
        if (ignore_not_found_) {
          in_included_[indep_index_] = -1;
          std::cout << "ignoring index : " << indep_index_ << ", as no historical correlation found\n";
        }
      } else if ((indicator_correlations_[indep_index_] < 0) && (initial_correlations_[indep_index_] > 0)) {
        in_included_[indep_index_] = -1;  // sign different than expected
        std::cout << "sign of historical correlation ( index: " << indep_index_ << " ) expected -ve "
                  << indicator_correlations_[indep_index_] << ", actual value " << initial_correlations_[indep_index_]
                  << std::endl;
      } else if ((indicator_correlations_[indep_index_] > 0) && (initial_correlations_[indep_index_] < 0)) {
        in_included_[indep_index_] = -1;  // sign different than expected
        std::cout << "sign of historical correlation ( index: " << indep_index_ << " ) expected +ve "
                  << indicator_correlations_[indep_index_] << ", actual value " << initial_correlations_[indep_index_]
                  << std::endl;
      }
    }
  }
}
}
}

#endif /* BASE_MTOOLS_UTILS_H */
