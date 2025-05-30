/**
   \file MToolsCode/fslr_so_nomeandata.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#include <ctime>
#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/iterative_regress.hpp"

#define NUM_LABELS 3
#define MAX_ITERATIONS 200

namespace HFSAT {

double GetMaximumLikelihood(std::vector<std::vector<double> >& model_, int indep_index_,
                            std::vector<std::vector<double> > train_data_independents_,
                            std::vector<int> train_data_dependent_, std::vector<int> included_,
                            std::vector<int>& model_indep_indx_, std::vector<double>& new_indep_weights_) {
  // int model_size_ = model_.size();
  // int num_samples_ = train_data_dependent_.size();
  double alpha = 0.00002;  // learning rate for gradient descent
  double lambda = 0.02;
  srand((unsigned)time(0));
  // randomly initialize the weights for all the current indicators +1 (for the new indep)
  for (unsigned int index_ = 0; index_ < model_.size(); index_++) {
    std::vector<double> indep_weights_;
    for (int w_indx_ = 0; w_indx_ < NUM_LABELS; w_indx_++) {
      double t_weight_ = (rand() % 200) / 100.0 - 1.0;
      model_[index_][w_indx_] = t_weight_;
      std::cout << t_weight_ << "  ";
    }
    std::cout << "\n";
  }
  model_indep_indx_[model_.size() - 1] = indep_index_;
  double log_lh_ = 0.0;
  std::vector<std::vector<double> > exp_linear_sums_;
  exp_linear_sums_.resize(train_data_dependent_.size());
  std::vector<double> norm_fac_(train_data_dependent_.size(), 0.0);
  // initialize linear_sums_ and exp_linear_sums_
  for (auto i = 0u; i < train_data_dependent_.size(); i++) {
    exp_linear_sums_[i].resize(NUM_LABELS);
  }
  for (auto i = 0u; i < train_data_dependent_.size(); i++) {
    for (unsigned int l = 0; l < NUM_LABELS; l++) {
      exp_linear_sums_[i][l] = 0.0;
    }
  }

  for (int iter_ = 0; iter_ < MAX_ITERATIONS; iter_++) {
    log_lh_ = 0;
    double sum_coeffs_ = 0;
    for (unsigned int i = 1; i < train_data_dependent_.size(); i++) {
      norm_fac_[i] = 0.0;
      sum_coeffs_ = 0;
      for (int l = 0; l < NUM_LABELS; l++) {
        double t_linear_sums_ = 0.0;
        for (unsigned int indep_ = 0; indep_ < model_.size(); indep_++) {
          if (indep_ != 0)
            t_linear_sums_ += model_[indep_][l] * train_data_independents_[model_indep_indx_[indep_ - 1]][i];
          else
            t_linear_sums_ += model_[indep_][l];

          sum_coeffs_ += model_[indep_][l] * model_[indep_][l];
        }
        exp_linear_sums_[i][l] = exp(t_linear_sums_);
        norm_fac_[i] += exp_linear_sums_[i][l];
      }
      for (int l = 0; l < NUM_LABELS; l++) {
        if (train_data_dependent_[i] == l) {
          log_lh_ += log(exp_linear_sums_[i][l] / norm_fac_[i]);
        }
      }
    }
    log_lh_ = -log_lh_ / train_data_dependent_.size();
    log_lh_ += 0.5 * lambda * sum_coeffs_;
    //        std::cout<<"Log Likelihood : "<<log_lh_<<"\n";

    for (unsigned int t = 0; t < model_.size(); t++) {
      for (int s = 0; s < NUM_LABELS; s++) {
        for (auto i = 0u; i < train_data_dependent_.size(); i++) {
          int t_flag_ = train_data_dependent_[i] == s ? 1 : 0;
          //                  current_model_[t][s] -=  (-alpha/train_data_dependent_.size())*(t_flag_ -
          //                  exp_linear_sums_[i][s]/norm_fac_[i])*train_data_independents_[model_indep_indx_[t]][i];
          if (t != 0)
            model_[t][s] -= -(alpha / train_data_dependent_.size()) *
                                (t_flag_ - exp_linear_sums_[i][s] / norm_fac_[i]) *
                                train_data_independents_[model_indep_indx_[t - 1]][i] +
                            lambda * model_[t][s];
          else
            model_[t][s] += alpha * (t_flag_ - exp_linear_sums_[i][s] / norm_fac_[i]);
        }
      }
    }
  }
  for (int l = 0; l < NUM_LABELS; l++) {
    new_indep_weights_[l] = (model_[model_.size() - 1][l]);
    std::cout << new_indep_weights_[l] << " ";
  }
  std::cout << "\n";
  return log_lh_;
}
// this function performs forward stagewise linear regression of nomean data by successive orthogonalization of
// independants
void FS_Logistic_Regression(std::vector<int>& train_data_dependent_,
                            std::vector<std::vector<double> >& train_data_independents_, std::vector<int>& included_,
                            const double kMaxIndepCorrelation, std::vector<std::vector<double> >& model_,
                            std::vector<int>& model_indep_indx_, const unsigned int max_model_size_) {
  if (model_.size() >= max_model_size_) return;

  int indicators_left_ = 0;
  for (auto i = 0u; i < included_.size(); i++) {
    if (included_[i] == 0) indicators_left_++;
  }
  if (indicators_left_ <= 0) {
    return;
  }

  std::vector<double> sum_square_independents_(train_data_independents_.size(),
                                               -1);  ///< ith value = Sum_j ( x_i_j * x_i_j )
  double sum_square_independents_chosen_index_ = 0;  ///< Sum_j ( x_ci_j * x_ci_j )
  double min_min_lh_ = -100000000.0;
  double min_lh_ = 0.0;
  std::vector<double> min_indep_weights_;
  int chosen_index_ = -1;

  std::vector<double> new_indep_weights_(NUM_LABELS, 0.0);
  model_.push_back(new_indep_weights_);
  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independents_.size(); indep_index_++) {
    if (included_[indep_index_] == 0) {
      std::vector<double> new_indep_weights_(NUM_LABELS, 0.0);
      min_lh_ = GetMaximumLikelihood(model_, indep_index_, train_data_independents_, train_data_dependent_, included_,
                                     model_indep_indx_, new_indep_weights_);
      std::cout << "Objective function for index " << indep_index_ << " is " << min_lh_ << "\n";
      if (min_lh_ > min_min_lh_) {
        min_min_lh_ = min_lh_;
        chosen_index_ = indep_index_;
        min_indep_weights_ = new_indep_weights_;
      }
    }
  }

  included_[chosen_index_] = 1;
  std::cout << "chosen index " << chosen_index_ << "\n";
  for (int l = 0; l < NUM_LABELS; l++) {
    model_[model_.size() - 1][l] = min_indep_weights_[l];
  }
  model_indep_indx_[model_.size() - 1] = chosen_index_;

  for (unsigned int t = 0; t < model_.size(); t++) {
    std::cout << model_indep_indx_[t] << "\n";
  }

  std::cout << "weights \n";
  for (unsigned int indep_index_ = 0; indep_index_ < model_.size(); indep_index_++) {
    for (int l = 0; l < NUM_LABELS; l++) {
      std::cout << model_[indep_index_][l] << "  ";
    }
    std::cout << "\n";
  }

  std::vector<std::vector<double> > output_prob_;
  std::vector<int> output_labels_(train_data_dependent_.size(), -1);
  std::vector<double> t_exps_(NUM_LABELS, 0);
  int max_label_ = -1;
  double max_sum_ = -1000000000;
  std::cout << model_indep_indx_.size() << "\n";
  std::cout << model_.size() << "  " << model_[0].size() << "\n";
  for (auto i = 0u; i < train_data_dependent_.size(); i++) {
    for (int l = 0; l < NUM_LABELS; l++) {
      double t_sum_ = 0;
      for (unsigned int t = 0; t < model_.size(); t++) {
        t_sum_ += model_[t][l] * train_data_independents_[model_indep_indx_[t]][i];
      }
      if (t_sum_ > max_sum_) {
        max_sum_ = t_sum_;
        max_label_ = l;
      }
    }
    output_labels_[i] = max_label_;
  }
  double match_count_ = 0.0;
  for (auto i = 0u; i < train_data_dependent_.size(); i++) {
    if (output_labels_[i] == train_data_dependent_[i]) match_count_++;
  }
  double accuracy_ = match_count_ / train_data_dependent_.size();
  std::cout << "Current Accuracy : " << accuracy_ * 100 << "%\n";

  for (unsigned int indep_index_ = 0; indep_index_ < train_data_independents_.size(); indep_index_++) {
    if (included_[indep_index_] == 0) {  // still eligible to be selected
      double sum_chosen_indep_ = 0;
      for (unsigned int dataline_num_ = 0; dataline_num_ < train_data_independents_[chosen_index_].size();
           dataline_num_++) {
        sum_chosen_indep_ += (train_data_independents_[chosen_index_][dataline_num_]) *
                             (train_data_independents_[indep_index_][dataline_num_]);
      }
      // also adding correlation based removal of independents not selected yet
      if (sum_square_independents_[indep_index_] <= 0) {  // not computed yet
        sum_square_independents_[indep_index_] = VectorUtils::CalcL2Norm(train_data_independents_[indep_index_]);
      }
      double corr_chosen_indep_ = sum_chosen_indep_ / (sqrt(sum_square_independents_chosen_index_) *
                                                       sqrt(sum_square_independents_[indep_index_]));
      if (fabs(corr_chosen_indep_) > kMaxIndepCorrelation) {  // marking very correlated indicators to the one that was
                                                              // just selected, as ineligible
                                                              //      included_[indep_index_] = -1;
        //		std::cout<<"Indicator "<<indep_index_<<" rendered useless\n";
        // std::cout << " Marked " << indep_index_ << " ineligible since correlation with " << chosen_index_ << " = " <<
        // corr_chosen_indep_ << std::endl;
      }
    }
  }

  FS_Logistic_Regression(train_data_dependent_, train_data_independents_, included_, kMaxIndepCorrelation, model_,
                         model_indep_indx_, max_model_size_);
}
}
