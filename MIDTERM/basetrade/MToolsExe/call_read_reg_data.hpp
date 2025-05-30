/**
    \file MToolsExe/call_read_reg_data.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_MTOOLS_CALL_READ_REG_DATA_H
#define BASE_MTOOLS_CALL_READ_REG_DATA_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <iterator>  // std::istream_iterator

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
typedef unsigned int uint32_t;

#define TEXT_FILE_MEMORY_LIMIT 1572 * 1024 * 1024  // 1.5GB
#define MEMORY_LIMIT 1024 * 1024 * 1024            // 1 GB
#define RANDOM_LIMIT 0xffff                        // controls resolution, easier to multiply and divide

namespace HFSAT {

// checks the file size is less than TEXT_FILE_MEMORY_LIMIT or not. return RANDOM_LIMIT if all lines to take,
// returns x/RANDOM_LIMIT if x lines to take.
uint32_t isFileGoodToGo(const std::string& infilename_, uint32_t max_number_of_lines_allowed_) {
  struct stat buf;
  stat(infilename_.c_str(), &buf);
  if (buf.st_size < TEXT_FILE_MEMORY_LIMIT) return RANDOM_LIMIT;
  std::ifstream infile_(infilename_.c_str());
  infile_.unsetf(std::ios_base::skipws);
  // count the newlines with an algorithm specialized for counting:
  uint32_t total_num_lines_infile_ =
      std::count(std::istream_iterator<char>(infile_), std::istream_iterator<char>(), '\n');
  infile_.close();
  if (total_num_lines_infile_ < max_number_of_lines_allowed_) return RANDOM_LIMIT;
  return (int)((double)RANDOM_LIMIT * max_number_of_lines_allowed_ / total_num_lines_infile_);
}

inline void ReadMultDepData(const std::string& infilename_, const unsigned int num_deps_,
                            std::vector<std::vector<double> >& train_data_dependants_,
                            std::vector<std::vector<double> >& train_data_independants_) {
  std::ifstream infile_;
  infile_.open(infilename_.c_str());
  if (!infile_.is_open()) {
    std::cerr << " Input data file " << infilename_ << " did not open " << std::endl;
    exit(kCallIterativeRegressInFileOpenError);
  }

  const unsigned int kLineBufferLen = 10240;
  char readline_buffer_[kLineBufferLen];
  bzero(readline_buffer_, kLineBufferLen);
  int num_independants_ = -1;

  unsigned int max_number_of_lines_to_read_ = 1u;  // Just to let it enter the loop
  unsigned int no_lines_read = 0u;
  uint32_t prob_to_take_this_line_ = RANDOM_LIMIT;  // => 1 yes for all

  while (infile_.good() && no_lines_read < max_number_of_lines_to_read_) {
    bzero(readline_buffer_, kLineBufferLen);
    infile_.getline(readline_buffer_, kLineBufferLen);
    PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    // simple file format : "DEPENDANT_1 ... DEPENDANT_D INDEP_1 INDEP_2 ... INDEP_V"

    if (tokens_.size() <= num_deps_) continue;

    if (num_independants_ == -1) {  // first line
      num_independants_ = tokens_.size() - num_deps_;
      if (num_independants_ < 1) {
        infile_.close();

        std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least " << num_deps_
                  << std::endl;
        exit(kCallIterativeRegressFewTokens);
      }
      train_data_independants_.resize(num_independants_);
      // 1 GB
      max_number_of_lines_to_read_ = (unsigned int)std::max(
          100, (int)(((double)MEMORY_LIMIT) / (double)((num_independants_ + num_deps_) * sizeof(double))));
      prob_to_take_this_line_ = isFileGoodToGo(infilename_, max_number_of_lines_to_read_);
      no_lines_read++;
    }

    if ((rand() & RANDOM_LIMIT) >= prob_to_take_this_line_)  // TODO - may need to modify the prob_to_take_this_line so
                                                             // that we can be precise in final number of lines read
      continue;

    if ((num_independants_ + num_deps_) !=
        tokens_.size())  // make sure this line has the number of words we are expecting
    {
      infile_.close();
      std::cerr << " A line in the input data has a different number of tokens " << tokens_.size() << " than the first "
                << (num_deps_ + num_independants_) << std::endl;
      exit(kCallIterativeRegressDiffNumTokensError);
    } else {
      unsigned int start_dep_index_ = 0;
      unsigned int start_indep_index_ = num_deps_;
      for (auto i = 0u; i < train_data_dependants_.size(); i++) {
        train_data_dependants_[i].push_back((double)atof(tokens_[i + start_dep_index_]));
      }
      for (auto i = 0u; i < train_data_independants_.size(); i++) {
        train_data_independants_[i].push_back((double)atof(tokens_[i + start_indep_index_]));
      }
      no_lines_read++;
    }
  }
  infile_.close();

  const unsigned int kMinLinesNeededIterativeRegress = 10u;
  if (no_lines_read < kMinLinesNeededIterativeRegress) {
    std::cerr << "Less than " << kMinLinesNeededIterativeRegress << " (" << no_lines_read
              << ") lines in input data file: " << infilename_ << std::endl;
    exit(kCallIterativeRegressVeryFewLines);
  }
}

inline void ReadData(const std::string& infilename_, std::vector<double>& train_data_dependant_,
                     std::vector<std::vector<double> >& train_data_independants_) {
  std::ifstream infile_(infilename_.c_str());
  if (!infile_.is_open()) {
    std::cerr << " Input data file " << infilename_ << " did not open " << std::endl;
    exit(kCallIterativeRegressInFileOpenError);
  }

  const unsigned int kLineBufferLen = 10240;
  char readline_buffer_[kLineBufferLen];
  bzero(readline_buffer_, kLineBufferLen);
  int num_independants_ = -1;

  unsigned int no_lines_to_read = 1;  // Just to let it enter the loop
  unsigned int no_lines_read = 0;

  uint32_t prob_to_take_this_line_ = RANDOM_LIMIT;  // => 1 yes for all
  while (infile_.good() && no_lines_read < no_lines_to_read) {
    bzero(readline_buffer_, kLineBufferLen);
    infile_.getline(readline_buffer_, kLineBufferLen);
    PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    // simple file format : "DEPENDANT INDEP_1 INDEP_2 ... INDEP_V"

    if (tokens_.size() <= 0) continue;

    if (num_independants_ == -1) {  // first line
      num_independants_ = tokens_.size() - 1;
      if (num_independants_ < 1) {
        infile_.close();

        std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 2 " << std::endl;
        exit(kCallIterativeRegressFewTokens);
      }
      train_data_independants_.resize(num_independants_);
      // Calculate numlines to allow to follow memory limmit.
      no_lines_to_read =
          (unsigned int)std::max(100, (int)((double)MEMORY_LIMIT / ((num_independants_ + 1) * sizeof(double))));
      prob_to_take_this_line_ = isFileGoodToGo(infilename_, no_lines_to_read);
      no_lines_read++;
    }
    if ((rand() & RANDOM_LIMIT) >= prob_to_take_this_line_)  // TODO - may need to modify the prob_to_take_this_line so
                                                             // that we can be precise in final number of lines read
      continue;

    if ((1 + num_independants_) !=
        (int)(tokens_.size()))  // make sure this line has the number of words we are expecting
    {
      infile_.close();
      std::cerr << " A line in the input data has a different number of tokens " << tokens_.size() << " than the first "
                << (1 + num_independants_) << std::endl;
      exit(kCallIterativeRegressDiffNumTokensError);
    } else {
      train_data_dependant_.push_back((double)atof(tokens_[0]));
      for (auto i = 0u; i < train_data_independants_.size(); i++) {
        train_data_independants_[i].push_back((double)atof(tokens_[i + 1]));
      }
      no_lines_read++;
    }
  }
  infile_.close();

  const unsigned int kMinLinesNeededIterativeRegress = 10;
  if (train_data_dependant_.size() < kMinLinesNeededIterativeRegress) {
    std::cerr << "Less than " << kMinLinesNeededIterativeRegress << " (" << train_data_dependant_.size()
              << ") lines in input data file: " << infilename_ << std::endl;
    exit(kCallIterativeRegressVeryFewLines);
  }
}

inline void ReadSelectedData(const std::string& input_reg_data_file_name_,
                             const std::vector<unsigned int>& chosen_indep_index_vec_,
                             std::vector<double>& train_data_dependant_,
                             std::vector<std::vector<double> >& train_data_independants_) {
  std::ifstream infile_;
  infile_.open(input_reg_data_file_name_.c_str());
  if (!infile_.is_open()) {
    std::cerr << " Input data file " << input_reg_data_file_name_ << " did not open " << std::endl;
    exit(kCallIterativeRegressInFileOpenError);
  }

  const unsigned int kLineBufferLen = 10240;
  char readline_buffer_[kLineBufferLen];
  bzero(readline_buffer_, kLineBufferLen);
  int num_independants_ = -1;
  uint32_t prob_to_take_this_line_ = RANDOM_LIMIT;  // => 1 yes for all
  uint32_t no_lines_read = 0;

  while (infile_.good()) {
    bzero(readline_buffer_, kLineBufferLen);
    infile_.getline(readline_buffer_, kLineBufferLen);
    PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    // simple file format : "DEPENDANT INDEP_1 INDEP_2 ... INDEP_V"

    if (tokens_.size() <= 0) continue;

    if (num_independants_ == -1) {  // first line
      num_independants_ = tokens_.size() - 1;
      if (num_independants_ < 1) {
        infile_.close();

        std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 2 " << std::endl;
        exit(kCallIterativeRegressFewTokens);
      }
      // 1 GB
      uint32_t max_number_of_lines_to_read_ = (unsigned int)std::max(
          100, (int)(((double)MEMORY_LIMIT) / (double)((chosen_indep_index_vec_.size()) * sizeof(double))));
      prob_to_take_this_line_ = isFileGoodToGo(input_reg_data_file_name_, max_number_of_lines_to_read_);
      no_lines_read++;
    }

    if ((rand() & RANDOM_LIMIT) >= prob_to_take_this_line_)  // TODO - may need to modify the prob_to_take_this_line so
                                                             // that we can be precise in final number of lines read
      continue;

    if ((1 + num_independants_) !=
        (int)(tokens_.size()))  // make sure this line has the number of words we are expecting
    {
      infile_.close();
      std::cerr << " A line in the input data has a different number of tokens " << tokens_.size() << " than the first "
                << (1 + num_independants_) << std::endl;
      exit(kCallIterativeRegressDiffNumTokensError);
    } else {
      train_data_dependant_.push_back((double)atof(tokens_[0]));
      for (auto i = 0u; i < chosen_indep_index_vec_.size(); i++) {
        train_data_independants_[i].push_back((double)atof(tokens_[chosen_indep_index_vec_[i]]));
      }
    }
  }
  infile_.close();
}

inline void ReadInData(const std::string& infilename_, std::vector<std::vector<double> >& train_data_independants_) {
  std::ifstream infile_;
  infile_.open(infilename_.c_str());
  if (!infile_.is_open()) {
    std::cerr << " Input data file " << infilename_ << " did not open " << std::endl;
    exit(kCallIterativeRegressInFileOpenError);
  }

  const unsigned int kLineBufferLen = 10240;
  char readline_buffer_[kLineBufferLen];
  bzero(readline_buffer_, kLineBufferLen);
  int num_independants_ = -1;

  int line_num = 0;
  uint32_t prob_to_take_this_line_ = RANDOM_LIMIT;  // => 1 yes for all
  while (infile_.good()) {
    bzero(readline_buffer_, kLineBufferLen);
    infile_.getline(readline_buffer_, kLineBufferLen);
    line_num++;
    PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    // simple file format : "INDEP_1 INDEP_2 ... INDEP_V"

    if (tokens_.size() <= 0) continue;

    if (num_independants_ == -1) {  // first line
      num_independants_ = tokens_.size();
      if (num_independants_ < 1) {
        infile_.close();

        std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 1 " << std::endl;
        exit(kCallIterativeRegressFewTokens);
      }

      train_data_independants_.resize(num_independants_);
      uint32_t num_lines_to_read_ = std::max(100, (int)((double)MEMORY_LIMIT / (num_independants_ * sizeof(double))));
      prob_to_take_this_line_ = isFileGoodToGo(infilename_, num_lines_to_read_);
    }

    if ((rand() & RANDOM_LIMIT) >= prob_to_take_this_line_)  // TODO - may need to modify the prob_to_take_this_line so
                                                             // that we can be precise in final number of lines read
      continue;

    if ((num_independants_) != (int)(tokens_.size()))  // make sure this line has the number of words we are expecting
    {
      infile_.close();
      std::cerr << " Line number: " << line_num << " in the input data has a different number of tokens "
                << tokens_.size() << " while first line has " << (num_independants_) << std::endl;
      exit(kCallIterativeRegressDiffNumTokensError);
    } else {
      for (auto i = 0u; i < train_data_independants_.size(); i++) {
        train_data_independants_[i].push_back((double)atof(tokens_[i]));
      }
    }
  }
  infile_.close();
}

inline void ReadInIndicatorIndices(const std::string& input_variable_selection_filename_,
                                   std::vector<unsigned int>& chosen_indep_index_vec_,
                                   std::vector<double>& initial_correlations_) {
  std::ifstream infile_;
  infile_.open(input_variable_selection_filename_.c_str());
  if (!infile_.is_open()) {
    std::cerr << " Input variable selection file " << input_variable_selection_filename_ << " did not open "
              << std::endl;
    exit(kCallIterativeRegressInFileOpenError);
  }

  const unsigned int kLineBufferLen = 1024;
  char readline_buffer_[kLineBufferLen];
  bzero(readline_buffer_, kLineBufferLen);

  int line_num = 0;
  while (infile_.good()) {
    bzero(readline_buffer_, kLineBufferLen);
    infile_.getline(readline_buffer_, kLineBufferLen);
    line_num++;
    PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    // simple file format : "OutCoeff 2 0.127296 InitCorrelation 0.0762235"

    if (tokens_.size() <= 3) continue;

    if (strcmp(tokens_[0], "OutCoeff") == 0) {
      int this_index = atoi(tokens_[1]);
      if (this_index >= 0) {
        chosen_indep_index_vec_.push_back((unsigned int)this_index);
        initial_correlations_.push_back(atof(tokens_[4]));
      }
    }
  }
  infile_.close();
}
}
#endif  // BASE_MTOOLS_CALL_READ_REG_DATA_H
