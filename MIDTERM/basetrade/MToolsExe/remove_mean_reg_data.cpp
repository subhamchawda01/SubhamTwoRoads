/**
   \file MToolsExe/remove_mean_reg_data.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/CDef/math_utils.hpp"
#include "basetrade/MToolsExe/call_read_reg_data.hpp"

using namespace HFSAT;

/// Exec used to read a matrix of data, compute means, stdevs from the columns,
/// reread data, removing mean from the columns, print the resulting values in the given output file
///
/// Reads command line arguments
/// Read lines and in an online fashion
/// compute mean & stdev of all columns.
/// Read the file again and print values minus precomputed means
int main(int argc, char** argv) {
  // local variables
  int lines_read_ = 0;

  std::vector<double> col_sum_;
  std::vector<double> col_squared_sum_;

  // command line processing
  if (argc < 4) {
    std::cerr << argv[0] << " r_wmean_reg_data_filename  rw_reg_data_filename  rw_stats_reg_data_filename "
              << std::endl;
    exit(0);
  }

  std::string r_wmean_reg_data_filename_ = argv[1];
  std::string rw_reg_data_filename_ = argv[2];
  std::string rw_stats_reg_data_filename_ = argv[3];

  // read data
  {
    std::ifstream infile_;
    infile_.open(r_wmean_reg_data_filename_.c_str());
    if (!infile_.is_open()) {
      std::cerr << " Input data file " << r_wmean_reg_data_filename_ << " did not open " << std::endl;
      exit(0);
    }

    const unsigned int kLineBufferLen = 10240;
    char readline_buffer_[kLineBufferLen];
    bzero(readline_buffer_, kLineBufferLen);
    int num_cols_ = -1;

    while (infile_.good()) {
      bzero(readline_buffer_, kLineBufferLen);
      infile_.getline(readline_buffer_, kLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      // simple file format : "DEPENDANT INDEP_1 INDEP_2 ... INDEP_V"

      if (tokens_.size() <= 0) continue;

      if (num_cols_ == -1) {  // first line
        num_cols_ = tokens_.size();
        if (num_cols_ < 1) {
          infile_.close();

          std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 1 " << std::endl;
          exit(0);
        }

        col_sum_.resize(num_cols_, 0);
        col_squared_sum_.resize(num_cols_, 0);
      }

      // make sure this line has the number of words we are expecting
      if (num_cols_ != (int)(tokens_.size())) {
        infile_.close();
        std::cerr << " A line in the input data has a different number of tokens " << tokens_.size()
                  << " than the first " << num_cols_ << std::endl;
        exit(0);
      } else {
        lines_read_++;
        for (auto i = 0u; i < col_sum_.size(); i++) {
          double this_col_value_ = atof(tokens_[i]);
          if (std::isnan(this_col_value_)) {
            std::cerr << "Reading " << r_wmean_reg_data_filename_ << " nan in column " << i
                      << " lines_read: " << lines_read_ << std::endl;
            this_col_value_ = 0;
          }
          col_sum_[i] += this_col_value_;
          col_squared_sum_[i] += HFSAT::GetSquareOf(this_col_value_);
        }
      }
    }
    infile_.close();
  }

  if (lines_read_ <= 2) exit(1);

  std::vector<double> col_mean_(col_sum_.size(), 0);
  std::vector<double> col_stdev_(col_sum_.size(), 0);
  std::vector<double> col_sharpe_(col_sum_.size(), 0);

  for (auto i = 0u; i < col_sum_.size(); i++) {
    col_mean_[i] = col_sum_[i] / (double)lines_read_;
    if (lines_read_ < 2) {
      col_stdev_[i] = 0;
      col_sharpe_[i] = 0;
    } else {
      col_stdev_[i] = sqrt((col_squared_sum_[i] - ((double)lines_read_ * HFSAT::GetSquareOf(col_mean_[i]))) /
                           (double)(lines_read_ - 1));
      if (std::isnan(col_stdev_[i])) {
        col_stdev_[i] = 1;
        std::cerr << "Processing " << r_wmean_reg_data_filename_ << " nan in column " << i
                  << " col_mean: " << col_mean_[i] << " lines_read: " << lines_read_ << std::endl;
      }

#define MIN_INDICATOR_STDEV_VALUE 0.000005
      if (col_stdev_[i] < MIN_INDICATOR_STDEV_VALUE) {
        col_stdev_[i] = MIN_INDICATOR_STDEV_VALUE;  // to avoid div_by_zero nans
      }
#undef MIN_INDICATOR_STDEV_VALUE

      col_sharpe_[i] = col_mean_[i] / col_stdev_[i];
    }
  }

  // read data
  {
    std::ofstream out_reg_file_(rw_reg_data_filename_.c_str(), std::ofstream::out);
    if (!out_reg_file_.is_open()) {
      std::cerr << " Output data file " << rw_reg_data_filename_ << " did not open for writing " << std::endl;
      exit(0);
    }

    std::ifstream infile_;
    infile_.open(r_wmean_reg_data_filename_.c_str());
    if (!infile_.is_open()) {
      std::cerr << " Input data file " << r_wmean_reg_data_filename_ << " did not open " << std::endl;
      exit(0);
    }

    const unsigned int kLineBufferLen = 10240;
    char readline_buffer_[kLineBufferLen];
    bzero(readline_buffer_, kLineBufferLen);
    int num_cols_ = -1;

    while (infile_.good()) {
      bzero(readline_buffer_, kLineBufferLen);
      infile_.getline(readline_buffer_, kLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      // simple file format : "DEPENDANT INDEP_1 INDEP_2 ... INDEP_V"

      if (tokens_.size() <= 0) continue;

      if (num_cols_ == -1) {  // first line
        num_cols_ = tokens_.size();
        if (num_cols_ < 1) {
          infile_.close();

          std::cerr << "First line has very few tokens " << tokens_.size() << " should be at least 1 " << std::endl;
          exit(0);
        }
      }

      // make sure this line has the number of words we are expecting
      if (num_cols_ != (int)(tokens_.size())) {
        infile_.close();
        std::cerr << " A line in the input data has a different number of tokens " << tokens_.size()
                  << " than the first " << num_cols_ << std::endl;
        exit(0);
      } else {
        lines_read_++;
        for (auto i = 0u; i < col_sum_.size(); i++) {
          double this_col_value_ = atof(tokens_[i]);
          if (i > 0) {
            out_reg_file_ << ' ';
          }
          out_reg_file_ << (this_col_value_ - col_mean_[i]);  // printing value minus precomputed mean
        }
        out_reg_file_ << '\n';
      }
    }
    infile_.close();
    out_reg_file_.close();
  }

  {
    std::ofstream out_stats_reg_file_(rw_stats_reg_data_filename_.c_str(), std::ofstream::out);
    if (!out_stats_reg_file_.is_open()) {
      std::cerr << " Output stats data file " << rw_stats_reg_data_filename_ << " did not open for writing "
                << std::endl;
      exit(0);
    }

    for (auto i = 0u; i < col_sum_.size(); i++) {
      out_stats_reg_file_.width(8);
      out_stats_reg_file_ << col_mean_[i];
      out_stats_reg_file_ << " ";
      out_stats_reg_file_.width(8);
      out_stats_reg_file_ << col_stdev_[i];
      out_stats_reg_file_ << " ";
      out_stats_reg_file_.width(8);
      out_stats_reg_file_ << fabs(col_sharpe_[i]);
      out_stats_reg_file_ << std::endl;
    }
    out_stats_reg_file_.close();
  }
}
