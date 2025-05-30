/**
   \file MToolsCode/mars_so.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/MTools/iterative_regress.hpp"
#include <stdlib.h>

#define kMinResidualError 1e-04
#define MAX_NUM_KNOTS 11
#define MODEL_SHARPE_THRESHOLD 0.2
#define MAX_ALLOWED_MODEL_MEAN 0.007
namespace HFSAT {

bool FindKnots(std::vector<double>& x, double** knots, uint32_t num_knots) {
  if (sizeof(*knots) < sizeof(double) * num_knots) *knots = new double[num_knots];
  uint32_t n = x.size();
  std::vector<double> tmp_arr(x.size());
  StatStream s;
  for (uint32_t i = 0; i < x.size(); i++) {
    s.insert(x[i], 1);
    tmp_arr[i] = x[i];
  }
  if (fabs(s.sharpeX()) > MODEL_SHARPE_THRESHOLD) {
    std::cerr << "HighSharpeCheckIndependent: " << s.sharpeX() << std::endl;
    return false;
  }
  std::sort(tmp_arr.begin(), tmp_arr.end());
  tmp_arr.resize(std::distance(tmp_arr.begin(), std::unique(tmp_arr.begin(), tmp_arr.end())));
  n = tmp_arr.size();
  for (uint32_t i = 0; i < num_knots; i++) {
    (*knots)[i] = tmp_arr[int(((i + 1) * 0.9 * n) / num_knots)];
    // std::cerr<<(*knots)[i]<<" ";
  }
  return n < num_knots ? false : true;
  // std::cerr<<std::endl;
}

double getVar(std::vector<double>& x) {
  StatStream s;
  for (uint32_t i = 0; i < x.size(); i++) s.insert(1, x[i]);
  std::cout << "Mean: " << s.meanY() << std::endl;
  return s.varY();
}

double modelComplexity(std::vector<Term>& model_) {
  /*
   * "Thus if there are r linearly independent basis functions in the model, and K knots were selected in the forward
   * process, the formula is M (λ) = r+cK,
   * where c = 3. (When the model is restricted to be additive—details below a penalty of c = 2 is used). Using this, we
   * choose the model along the
   * backward sequence that minimizes GCV(λ)." -- The Elements of Statistical Learning
   *
   */

  // uint32_t n = model_.size();
  // TODO: some sophisticated technique for model_complexity
  // std::vector<int> tmp (n);
  // for(uint32_t i = 0; i<n;i++) { tmp[i] = model_[i].index; }
  // std::sort(tmp.begin(), tmp.end());
  // tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());
  // return tmp.size() + 3*model_.size();   // GCV for additive model use 2 instead of 3
  return (double)model_.size() * (4.0 + 0.5);
}

void removeHighSharpeVariables(std::vector<std::vector<double> >& x, std::vector<Term>& candidates,
                               std::vector<bool>& eligible) {
  StatStream* s = new StatStream[candidates.size()];
  if (x.size() <= 0) return;
  for (uint32_t j = 0; j < x[0].size(); j++)
    for (uint32_t i = 0; i < candidates.size(); i++)
      s[i].insert(candidates[i].val_s(x, j, 1) - candidates[i].val_s(x, j, -1), 1);
  for (uint32_t i = 0; i < candidates.size(); i++)
    if (s[i].sharpeX() > MODEL_SHARPE_THRESHOLD) {
      std::cerr << "BasisFunc: " << candidates[i].toString() << " has very high Sharpe: " << s[i].sharpeX()
                << ". LimitSharpeVal: " << MODEL_SHARPE_THRESHOLD << std::endl;
      eligible[i] = false;
      // removing its siblings also
      for (int j = i - 1; j >= 0 && candidates[j].index == candidates[i].index; j--) {
        eligible[j] = false;
        std::cerr << "\t\tremoving its siblings: " << s[j].sharpeX() << "\t" << candidates[j].toString() << std::endl;
      }
      for (uint32_t j = i + 1; j < candidates.size() && candidates[j].index == candidates[i].index; i++) {
        eligible[j] = false;
        std::cerr << "\t\tremoving its siblings: " << s[j].sharpeX() << "\t" << candidates[j].toString() << std::endl;
      }
    }
}

void getFastMarsCovForAll(std::vector<double>& y, std::vector<std::vector<double> >& x, std::vector<Term>& candi,
                          std::vector<double>& cov_vec, std::vector<bool>& eligible_candi_) {
  for (uint32_t j = 0; j < candi.size(); j++) {
    if (!eligible_candi_[j]) continue;
    StatStream s[3];
    for (uint32_t i = 0; i < y.size(); i++) {
      s[0].insert(candi[j].val_s(x, i, 1), y[i]);   // right hand part
      s[1].insert(candi[j].val_s(x, i, -1), y[i]);  // left hand part
      s[2].insert(x[candi[j].index][i], y[i]);      // for SLR coeff
    }
    // FSLR coeff check
    double slr_coeff = fabs(s[2].beta());
    double beta_0 = s[0].beta(), beta_1 = s[1].beta();
    if (2.5 * slr_coeff < fabs(beta_0) || 2.5 * slr_coeff < fabs(beta_1)) {
      eligible_candi_[j] = false;
      std::cerr << "HighCoeffCheckFailed: FLSRCoeff: " << slr_coeff << candi[j].toString() << std::endl;
      continue;
    }
    candi[j].setBeta(beta_0, beta_1);
    cov_vec[j] =
        (square(beta_0) * (s[0].varX() - square(s[0].meanX())) + square(beta_1) * (s[1].varX() - square(s[1].meanX())));
  }
}

double modelVal(std::vector<Term>& Model, std::vector<std::vector<double> >& x, int i) {
  double t_ = 0.0;
  for (uint32_t j = 0; j < Model.size(); j++) t_ += Model[j].val2(x, i);
  return t_;
}

double ModelSharpe(std::vector<Term>& Model, std::vector<std::vector<double> >& x) {
  if (x.size() <= 0) return -1;
  StatStream s;
  for (uint32_t j = 0; j < x[0].size(); j++) s.insert(modelVal(Model, x, j), 1);
  if (s.varX() <= 0) return -1;
  if (fabs(s.meanX()) > MAX_ALLOWED_MODEL_MEAN) {
    std::cerr << "ModelMean Check Failed: " << s.meanX() << " " << MAX_ALLOWED_MODEL_MEAN << std::endl;
    return 1.00;
  }
  return s.meanX() / sqrt(s.varX());
}

void BackwardPass(std::vector<double>& y, std::vector<std::vector<double> >& x, std::vector<Term>& Model) {
  std::cout << "Dep Var: " << getVar(y) << std::endl;
  uint32_t model_size_ = Model.size();
  float model_complexity_ = modelComplexity(Model);
  if (model_size_ <= 1) return;
  std::vector<double> t_error_sum_vec_(model_size_ + 1, 0);
  std::vector<double> t_error_vec_(model_size_ + 1, 0);
  for (uint32_t i = 0; i < y.size(); i++) {
    t_error_vec_[model_size_] = 0;
    for (uint32_t j = 0; j < model_size_; j++) {
      t_error_vec_[j] = Model[j].val2(x, i);
      t_error_vec_[model_size_] += t_error_vec_[j];
    }
    // std::cout << y[i] << t_error_vec_[model_size_]
    for (uint32_t j = 0; j < model_size_; j++) {
      t_error_sum_vec_[j] += square((y[i] - t_error_vec_[model_size_] + t_error_vec_[j]));
    }
    t_error_sum_vec_[model_size_] += square((y[i] - t_error_vec_[model_size_]));
  }
  double factor_ = square((1 - (model_complexity_ - 4.5) / y.size()));
  std::cerr << "Model_Complexity: " << model_complexity_ << "\tObs_pt_size: " << y.size()
            << "\nFactor_sub_model: " << factor_ << std::endl;
  for (uint32_t j = 0; j < model_size_; j++) {
    t_error_sum_vec_[j] /= factor_;
    std::cout << t_error_sum_vec_[j] << " ";
  }
  factor_ = square((1 - model_complexity_ / y.size()));
  t_error_sum_vec_[model_size_] /= factor_;
  std::cout << t_error_sum_vec_[model_size_] << "\n";
  std::cerr << "Factor_full_model: " << factor_ << std::endl;

  uint32_t min_index = std::min_element(t_error_sum_vec_.begin(), t_error_sum_vec_.end()) - t_error_sum_vec_.begin();
  std::cerr << t_error_sum_vec_[min_index] << " \tModelVar: " << t_error_sum_vec_[model_size_]
            << " MinIndex: " << min_index << std::endl;
  if (min_index != model_size_) {
    Model.erase(Model.begin() + min_index);
    BackwardPass(y, x, Model);
  }
}

bool hasEligibleCandi(std::vector<bool>& e_vec_) {
  for (uint32_t i = 0; i < e_vec_.size(); i++)
    if (e_vec_[i]) return true;
  return false;
}

bool passCorrSighCheck(std::vector<double>& init_corr_, Term& t, std::vector<bool>& eligible_term_) {
  int index = t.index;
  if (!eligible_term_[index]) return false;
  // dont add this Term if their initial corr is conflicting with current beta;
  if (init_corr_[index] * t.beta[0] < 0 || init_corr_[index] * t.beta[1] > 0) return false;
  // if ( fabs(t.beta[0]) > 10*fabs(init_corr_[index]) ||
  // 	 fabs(t.beta[1]) > 10*fabs(init_corr_[index])
  // 	 )
  //   {
  // 	//eligible_term_[index] = false;
  // 	fprintf(stderr, "TooHighBeta:\t\tInitialCorr: %f\t\t%s\n", init_corr_[index], t.toString());
  // 	return false;
  //   }
  return true;
}
// this function performs Multivariate Adaptive Regression Splines regression of non-NoMean dependant data
// pruning independants that drop below kMinCorrelationIncrement threshold along the way
void MARS_SO(std::vector<double>& train_data_dependant_, std::vector<std::vector<double> >& train_data_independants_,
             const unsigned int max_model_size_, std::vector<double>& initial_correlations_, std::vector<Term>& Model,
             ModelStats_t& model_stats_) {
  std::vector<double> orig_dependant_ = train_data_dependant_;  // copy the original
  std::vector<double*> indep_knots_(train_data_independants_.size());
  std::cerr << "Knots: \n";
  // candidate terms degree 1 terms only
  std::vector<Term> candidates_terms_;  // num_indep * max_num_knots * s(-1 or +1), i may increase
  for (uint32_t indep_index_ = 0; indep_index_ < train_data_independants_.size(); indep_index_++) {
    if (!FindKnots(train_data_independants_[indep_index_], &indep_knots_[indep_index_], MAX_NUM_KNOTS)) {
      printf("Some problem with Indep: %3d . Skipping.\n", indep_index_);
      continue;
    }
    for (uint32_t knots_index_ = 0; knots_index_ < (MAX_NUM_KNOTS / 2); knots_index_++) {
      Term t(indep_index_, MAX_NUM_KNOTS / 2 - knots_index_,
             indep_knots_[indep_index_][MAX_NUM_KNOTS - knots_index_ - 1],  // positive or right hand side
             -(MAX_NUM_KNOTS / 2 - knots_index_),
             indep_knots_[indep_index_][knots_index_]  // negetive or left hand side
             );
      candidates_terms_.push_back(t);
    }
    candidates_terms_.push_back(
        *(new Term(indep_index_, 0, indep_knots_[indep_index_][MAX_NUM_KNOTS / 2], 0,
                   indep_knots_[indep_index_][MAX_NUM_KNOTS / 2])));  // may boils down to linear model
  }

  std::vector<bool> eligible_term_(candidates_terms_.size(), true);
  std::cerr << "nCandi: " << candidates_terms_.size() << std::endl;
  std::vector<double> cov_vec_(candidates_terms_.size(), 0);
  double curr_dep_var_ = getVar(train_data_dependant_);
  double initial_dep_var_ = curr_dep_var_;
  double new_dep_var_ = 0;
  removeHighSharpeVariables(train_data_independants_, candidates_terms_, eligible_term_);
  while (Model.size() < 2 * max_model_size_ && hasEligibleCandi(eligible_term_)) {
    double min_dep_var_ = curr_dep_var_;
    int min_index_ = -1;
    // std::cerr<< " Iteration Statrts @ modelsize: " << Model.size() << " DepVar: " << curr_dep_var_ << " "
    //<< (Model.size()>0?(Model.back()).toString():"") << std::endl;
    getFastMarsCovForAll(train_data_dependant_, train_data_independants_, candidates_terms_, cov_vec_, eligible_term_);
    for (uint32_t i = 0; i < candidates_terms_.size(); i++) {
      if (eligible_term_[i] && passCorrSighCheck(initial_correlations_, candidates_terms_[i], eligible_term_)) {
        new_dep_var_ = curr_dep_var_ - cov_vec_[i];
        if (new_dep_var_ < min_dep_var_) {
          min_dep_var_ = new_dep_var_;
          min_index_ = i;
        }
      }
    }
    if (min_index_ == -1 || curr_dep_var_ - min_dep_var_ < kMinResidualError * initial_dep_var_
        // curr_dep_var_ < min_dep_var_
        ) {
      std::cerr << "There is none left to add. CurDepVar: " << curr_dep_var_ << " MinDepVar: " << min_dep_var_ << "\n";
      break;
    }
    eligible_term_[min_index_] = false;
    // Check sharpe of model ... should not have significant mean
    Model.push_back(candidates_terms_[min_index_]);
    if (fabs(ModelSharpe(Model, train_data_independants_)) > MODEL_SHARPE_THRESHOLD) {
      Term t = Model.back();
      Model.pop_back();
      std::cerr << "Increasing Sharpe. So not adding: " << t.toString() << std::endl;
    } else {
      // add them to model and remove their proj from y;
      candidates_terms_[min_index_].removeProjection(train_data_dependant_, train_data_independants_);
      // std::cerr<< "Model Size: " << Model.size() << " Residual Error: " << min_dep_var_ << " -- " <<
      // kMinResidualError << std::endl;
      curr_dep_var_ = min_dep_var_;
    }
    std::cerr << "Residual Error(adjusted): " << curr_dep_var_ / initial_dep_var_ << " "
              << candidates_terms_[min_index_].index << std::endl;
  }

  std::cerr << "\n\n-------------\nBefore BackwardPass: " << Model.size() << std::endl;
  std::cerr << std::endl;
  BackwardPass(orig_dependant_, train_data_independants_, Model);
  std::cout << "\n---------------\nAfter BackwardPass: " << Model.size() << std::endl;
  for (uint32_t i = 0; i < Model.size(); i++) std::cerr << Model[i].toString() << "\n";
  std::cerr << std::endl;
  // double squared_, correlation_, stdev_final_dep_, stdev_;
  model_stats_.getModelStats(Model, train_data_independants_, orig_dependant_);
  return;
}
}
/*
  IndexCoeffPair_t icp_t_;
  icp_t_.origindex_ = chosen_index_ ;
  icp_t_.coeff_ = dep_chosen_coeff_ ;

  double RSS_ = VectorUtils::CalcL2Norm ( train_data_dependant_ ); // this assumes ScaledVectorAddition above
  double n_2_ = train_data_dependant_.size() - 2;
  double n_1_ = train_data_dependant_.size() - 1;
  double SXX_ = VectorUtils::CalcL2Norm ( train_data_independants_ [ chosen_index_ ] );
  icp_t_.tstat_residual_dependant_ = dep_chosen_coeff_ / sqrt ( ( RSS_ / n_2_ ) / SXX_ ) ;
  icp_t_.tstat_ = dep_chosen_coeff_ / sqrt ( ( RSS_ / n_2_ ) / n_1_ ) ; // yet to be computed accurately but this should
  be an okay estimate, since SXX is n_1_ since data was normalized. The major change will come from computation of
  dep_chosen_coeff_ finally

  unsigned int this_finmodel_index_ = finmodel_.size ( ) ;
  finmodel_.push_back ( icp_t_ ) ;
*/
