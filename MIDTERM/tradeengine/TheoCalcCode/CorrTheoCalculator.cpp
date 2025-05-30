#include "tradeengine/TheoCalc/CorrTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"
/*! \brief This class handles features/filters applying to multiple products.
 * After loading relevant products, filters can be applied to enable/disable them from trading.
 */

CorrTheoCalculator::CorrTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                           HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                           int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_)
    : MasterTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                         _aggressive_get_flat_mfm_),
      corr_filter_(0) ,
      num_prod_to_take_(0) {
  dbglogger_ << watch_.tv() << " Creating CORR THEO CALCULATOR secId " << secondary_id_ << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
}
/** Loads basic parameters
*/
void CorrTheoCalculator::LoadParams() {
  MasterTheoCalculator::LoadParams();
  corr_filter_ = Parser::GetDouble(key_val_map_, "CORR_FILTER", 0.5);
  num_prod_to_take_ = Parser::GetInt(key_val_map_, "NUM_PROD_TO_TAKE", 5);
  // bool status_ = Parser::GetBool(key_val_map_, "STATUS", false);
  // status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  // filter_ptile_ = Parser::GetDouble(key_val_map_, "FILTER_PTILE", 25);
  // mom_filter_ptile_ = Parser::GetDouble(key_val_map_, "MOM_FILTER_PTILE", 25);
  // use_obv_filter_ = Parser::GetBool(key_val_map_, "USE_OBV_FILTER", false);
  // mr_filter_ = Parser::GetBool(key_val_map_, "USE_MR_FILTER", false);
}

/** Involves basic filter conditions. Fetches individual ticker values and filters according to the params given.
*/
void CorrTheoCalculator::ConfigureMidTermDetails(std::map<std::string, BaseTheoCalculator*>& theo_map_) {
  for (std::map<std::string, std::string>::iterator it = key_val_map_->begin(); it != key_val_map_->end(); ++it) {
    if (it->first.find("_THEO_IDENTIFIER") == std::string::npos || it->first.find("CORR") == std::string::npos) {
      continue;
    }
    std::string corr_theo_identifier_ = it->second;
    // Used in MACD
    if (theo_map_.find(corr_theo_identifier_) == theo_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "CORR THEO NOT FOUND (exiting) " << corr_theo_identifier_ << DBGLOG_ENDL_FLUSH;
    } 
    else {
      return_vec_.push_back(dynamic_cast<RatioTheoCalculator*>(theo_map_[corr_theo_identifier_])->GetReturnVec());
      theo_vec_.push_back(dynamic_cast<RatioTheoCalculator*>(theo_map_[corr_theo_identifier_]));
      // std::cout << "count " << corr_theo_identifier_ << std::endl;
    }
  }

  for(unsigned int i = 0;i < return_vec_.size();i++){
    std::vector<double> v(return_vec_.size());
    corr_vec_.push_back(v);
  }
  for(unsigned int i = 0;i < return_vec_.size();i++){
    // std::cout << theo_vec_[i]->GetSecondaryShc() << " " ;
    for(unsigned int j = i;j < return_vec_.size();j++){
      if( j == i){
        corr_vec_[i][j] = 1;
      }
      else{
        // std::cout << "Computing correlation " << i << " " << j << std::endl;
        corr_vec_[i][j] = ComputeCorrelation(i,j);
        corr_vec_[j][i] = corr_vec_[i][j];
        // std::cout << theo_vec_[j]->GetSecondaryShc() << " " << corr_vec_[i][j];
      }
    }
    // std::cout << std::endl;
  }


  // std::vector<std::vector<int> > indices_;
  for(unsigned int i = 0;i < corr_vec_.size();i++){
    int count = 0; int curr_index_ = -1; double max_val_ = 1; double curr_max_ = corr_filter_;
    // std::vector<int> v;
    while(count < num_prod_to_take_){
      for(unsigned int j = 0;j < corr_vec_[i].size();j++){
        if(corr_vec_[i][j] > corr_filter_ && j!= i && curr_max_ < corr_vec_[i][j] && corr_vec_[i][j] < max_val_){
          curr_max_ = corr_vec_[i][j];
          curr_index_ = j;
        }
      }
      if(curr_index_ != -1){
        count++;
        theo_vec_[i]->AddPrimarySMV(theo_vec_[curr_index_]->GetSecondarySMV());
        theo_vec_[i]->AddPrimarySMVWeight(1/theo_vec_[curr_index_]->GetPreviousDayPrice(),theo_vec_[curr_index_]->GetAvgVolume());
        // v.push_back(curr_index_);
        max_val_ = corr_vec_[i][curr_index_];
        curr_max_ = corr_filter_;
        curr_index_ = -1;
      }
      else{
        break;
      }

    }
    // indices_.push_back(v);
    theo_vec_[i]->ReInitializeDataSubscriptions();
  }
  
  // for(unsigned int i = 0;i < corr_vec_.size();i++){
  //   std::cout << theo_vec_[i]->GetSecondaryShc() <<" ";
  //   for(unsigned int j = 0;j < indices_[i].size();j++){
  //     std::cout << corr_vec_[i][indices_[i][j]] << " " << theo_vec_[indices_[i][j]]->GetSecondaryShc() << " ";
  //   // for(unsigned int j = 0;j < corr_vec_[i].size();j++){
  //   //   if(corr_vec_[i][j] > corr_filter_ && j!= i){
  //   //     std::cout << corr_vec_[i][j] << " " << theo_vec_[j]->GetSecondaryShc() <<" ";
  //   //   }
  //   }
  //   std::cout << std::endl;
  // }

}

double CorrTheoCalculator::ComputeCorrelation(int index1_, int index2_ ){
  
  //mean calculation
  double sum = std::accumulate(return_vec_[index1_].begin(), return_vec_[index1_].end(), 0.0);
  double mean_1_ = sum/return_vec_[index1_].size();

  sum = std::accumulate(return_vec_[index2_].begin(), return_vec_[index2_].end(), 0.0);
  double mean_2_ = sum/return_vec_[index2_].size();

  //std calculation
  std::vector<double> std_1_(return_vec_[index1_].size());
  std::transform(return_vec_[index1_].begin(), return_vec_[index1_].end(), std_1_.begin(),
                 std::bind2nd(std::minus<double>(), mean_1_));

  double sq_sum_1_ = std::inner_product(std_1_.begin(), std_1_.end(), std_1_.begin(), 0.0);
  double stdev_1_ = std::sqrt(sq_sum_1_ / (return_vec_[index1_].size()));

  std::vector<double> std_2_(return_vec_[index2_].size());
  std::transform(return_vec_[index2_].begin(), return_vec_[index2_].end(), std_2_.begin(),
                 std::bind2nd(std::minus<double>(), mean_2_));

  double sq_sum_2_ = std::inner_product(std_2_.begin(), std_2_.end(), std_2_.begin(), 0.0);
  double stdev_2_ = std::sqrt(sq_sum_2_ / (return_vec_[index2_].size()));

  double cov_sum_ = std::inner_product(std_1_.begin(), std_1_.end(), std_2_.begin(), 0.0);

  double correlation_ = cov_sum_/(stdev_1_*stdev_2_*return_vec_[index1_].size());

  return correlation_;
}