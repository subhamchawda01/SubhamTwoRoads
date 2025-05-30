/**
    \file MToolsExe/get_rep_matrix.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/

#include "basetrade/Math/matrix_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "basetrade/MTools/iterative_regress.hpp"
#include "basetrade/MToolsExe/call_read_reg_data.hpp"

/// Exec used to read a matrix of data and print the replication of each
/// Reads command line arguments
/// Compute mean & stdev of all independants
/// Normalize independants
/// Print CorrelationMatrix
int main(int argc, char** argv) {
  // local variables
  std::vector<std::vector<double> > t_in_matrix_;
  std::string infilename_ = "";

  // command line processing
  if (argc < 2) {
    std::cerr << argv[0] << " input_file_name " << std::endl;
    exit(0);
  }

  infilename_ = argv[1];

  // read data
  HFSAT::ReadInData(infilename_, t_in_matrix_);
  HFSAT::VectorUtils::CalcAndRemoveMeanFromSeriesVec(t_in_matrix_);

  const double min_correlation_ = 0.02;
  const double max_indep_correlation_ = 0.02;
  const unsigned int max_model_size_ = 10u;
  const double tstat_cutoff = 5;

  for (auto i = 0u; i < t_in_matrix_.size(); i++) {
    std::vector<double> train_data_dependant_(t_in_matrix_[i]);
    std::vector<std::vector<double> > train_data_independants_(t_in_matrix_);
    train_data_independants_.erase(train_data_independants_.begin() + i);
    std::vector<double> train_data_orig_dependant_ = train_data_dependant_;

    std::vector<HFSAT::IndexCoeffPair_t> finmodel_;
    std::vector<double> initial_correlations_(train_data_independants_.size(), 0);  // setting to 0 disables check

    std::vector<int> in_included_(train_data_independants_.size(), 0);  ///< included [ i ] = 0 if eligible to be
    /// selected , 1 if the indep(i) is already in
    /// the model, -1 if it is ineligible to be
    /// selected
    // mark all indices eligible to be selected

    // we have to send a yhat to regression to compute it incrementally
    // and cannot calculate it later since
    // we change the indicators while computing coefficients
    std::vector<double> predicted_values_(train_data_independants_[0].size(), 0.0);

    // square matrix of num_indiccators with 0.0
    std::vector<std::vector<double> > independant_orthogonalization_matrix_(train_data_independants_.size());
    for (unsigned int row_idx_ = 0u; row_idx_ < independant_orthogonalization_matrix_.size(); row_idx_++) {
      for (unsigned int col_idx_ = 0u; col_idx_ < train_data_independants_.size(); col_idx_++) {
        independant_orthogonalization_matrix_[row_idx_].push_back(0.0);
      }
    }

    std::vector<std::vector<double> > train_data_independants_original_ = train_data_independants_;
    HFSAT::FSLR_SO_NoMeanData(train_data_dependant_, train_data_independants_, 0, in_included_, finmodel_,
                              min_correlation_, max_indep_correlation_, initial_correlations_, max_model_size_,
                              predicted_values_, independant_orthogonalization_matrix_,
                              train_data_independants_original_, tstat_cutoff);

    std::cout << "REPLICATING " << i << " " << 0;
    for (unsigned int fidx = 0; fidx < finmodel_.size(); fidx++) {
      unsigned int true_orig_index_ =
          (finmodel_[fidx].origindex_ < i) ? (finmodel_[fidx].origindex_) : (finmodel_[fidx].origindex_ + 1);
      std::cout << ' ' << true_orig_index_ << ' ' << finmodel_[fidx].coeff_;
    }
    std::cout << std::endl;

    double model_rsquared_ = 0;
    double model_correlation_ = 0;
    double stdev_final_dependant_ = 0;
    double stdev_model_ = 0;
    HFSAT::ComputeModelStatisticsOrigNormalized(train_data_orig_dependant_, train_data_dependant_, model_rsquared_,
                                                model_correlation_, stdev_final_dependant_, stdev_model_);

    std::cout << "STATS " << i << " " << model_rsquared_ << " " << model_correlation_ << " " << stdev_final_dependant_
              << " " << stdev_model_ << std::endl;
  }
}
