/**
    \file MToolsExe/calc_na_adjusted_covariance_matrix.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/file_utils.hpp"

#include "basetrade/Math/matrix_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

/// Exec used to read a matrix of data and print the covariance matrix
/// Reads command line arguments
/// Compute mean & stdev of all independants
/// Normalize independants
/// Print Covariance Matrix
int main(int argc, char** argv) {
  // local variables
  std::vector<std::vector<double> > t_in_matrix_;
  std::vector<std::vector<bool> > is_na_matrix_;
  std::string infilename_ = "";

  // command line processing
  if (argc < 2) {
    std::cerr << argv[0] << " input_file_name " << std::endl;
    exit(0);
  }

  infilename_ = argv[1];

  // read data
  std::ifstream infile_;
  infile_.open(infilename_.c_str());
  if (!infile_.is_open()) {
    std::cerr << " Input data file " << infilename_ << " did not open " << std::endl;
    exit(0);
  }

  const unsigned int kLineBufferLen = 102400;
  char readline_buffer_[kLineBufferLen];
  bzero(readline_buffer_, kLineBufferLen);
  int num_independants_ = -1;

  int line_num = 0;
  while (infile_.good()) {
    bzero(readline_buffer_, kLineBufferLen);
    infile_.getline(readline_buffer_, kLineBufferLen);
    line_num++;
    HFSAT::PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    // simple file format : "INDEP_1 INDEP_2 ... INDEP_V"

    if (tokens_.size() <= 0) continue;

    if (num_independants_ == -1) {  // first line
      num_independants_ = tokens_.size();
      if (num_independants_ < 1) {
        infile_.close();

        std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 1 " << std::endl;
        exit(0);
      }
      t_in_matrix_.resize(num_independants_);
      is_na_matrix_.resize(num_independants_);
    }

    if ((num_independants_) != (int)(tokens_.size()))  // make sure this line has the number of words we are expecting
    {
      infile_.close();
      std::cerr << " Line number: " << line_num << " in the input data has a different number of tokens "
                << tokens_.size() << " while first line has " << (num_independants_) << std::endl;
      exit(0);
    } else {
      for (auto i = 0u; i < t_in_matrix_.size(); i++) {
        if (strcmp(tokens_[i], "NA") == 0) {
          is_na_matrix_[i].push_back(true);
          t_in_matrix_[i].push_back(0);
        } else {
          is_na_matrix_[i].push_back(false);
          t_in_matrix_[i].push_back((double)atof(tokens_[i]));
        }
      }
    }
  }
  infile_.close();

  HFSAT::SquareMatrix<double> covar_matrix_(t_in_matrix_.size());

  for (unsigned int row_index_ = 0u; row_index_ < t_in_matrix_.size(); row_index_++) {
    for (unsigned int column_index_ = 0u; column_index_ < t_in_matrix_.size(); column_index_++) {
      unsigned int max_length_ = std::min(t_in_matrix_[row_index_].size(), t_in_matrix_[column_index_].size());

      double accumulated_unprocessed_row_value_ = 0;
      double accumulated_unprocessed_column_value_ = 0;

      for (unsigned int day_index_ = 0u; day_index_ < max_length_; day_index_++) {
        if ((is_na_matrix_[row_index_][day_index_]) && (is_na_matrix_[column_index_][day_index_])) {
          continue;
        } else if (is_na_matrix_[row_index_][day_index_]) {
          accumulated_unprocessed_column_value_ += t_in_matrix_[column_index_][day_index_];
        } else if (is_na_matrix_[column_index_][day_index_]) {
          accumulated_unprocessed_row_value_ += t_in_matrix_[row_index_][day_index_];
        } else {
          covar_matrix_(row_index_, column_index_) +=
              (accumulated_unprocessed_row_value_ + t_in_matrix_[row_index_][day_index_]) *
              (accumulated_unprocessed_column_value_ + t_in_matrix_[column_index_][day_index_]);
          accumulated_unprocessed_row_value_ = 0;
          accumulated_unprocessed_column_value_ = 0;
        }
      }

      if (row_index_ != column_index_) {
        covar_matrix_(column_index_, row_index_) = covar_matrix_(row_index_, column_index_);
      }
    }
  }

  std::cout << covar_matrix_.ToString4();
}
