/**
   \file Indicators/pca_weights_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#ifndef BASE_INDICATORS_PCA_WEIGHTS_MANAGER
#define BASE_INDICATORS_PCA_WEIGHTS_MANAGER

#include <vector>
#include <string>
#include <map>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"


namespace HFSAT {

typedef struct {
  double eigenval_;
  std::vector<double> eigenvec_components_;
} EigenConstituents_t;

typedef std::vector<EigenConstituents_t> EigenConstituentsVec;
typedef std::map<std::string, EigenConstituentsVec> ShortcodeEigenConstituentsVecMap;
typedef std::map<std::string, EigenConstituentsVec>::iterator ShortcodeEigenConstituentsVecMapIter_t;
typedef std::map<std::string, EigenConstituentsVec>::const_iterator ShortcodeEigenConstituentsVecMapCIter_t;

/// We will use this to load the PCA related variables
/// specified number of leading eigenvectors and eigenvalues
/// Will be used to compute aggregate portfolio price using these values

class PcaWeightsManager {
 protected:
  // eigencompo means both eigenvals and eigenvectors
  ShortcodeEigenConstituentsVecMap shortcode_eigencompo_map_;

  // just to return something by reference
  EigenConstituentsVec empty_;
  std::vector<double> empty_stdev_;

  std::map<std::string, double> stdevs_for_shortcodes_map_;
  std::map<std::string, double> stdevs_for_shortcodes_map_longevity_;
  std::map<std::string, std::vector<double> > stdevs_for_portfolio_constituent_map_;
  std::map<std::string, std::vector<std::string> > portfolio_shortcode_to_constituent_shortcode_map_;

  static PcaWeightsManager *p_uniqueinstance_;
  int trading_date_;

  PcaWeightsManager(int _YYYYMMDD_) {
    trading_date_ = _YYYYMMDD_;
    LoadPCAConstituentInfo();
    LoadStdevInfo();
    LoadStdevInfoLongevity();
    LoadPCAInfoFile();
    LoadREGConstituentInfo();
    LoadREGInfoFile();
  }

 public:
  static PcaWeightsManager &GetUniqueInstance() {
    if (p_uniqueinstance_ == NULL) {
      std::cerr << "PcaWeightsManager::GetUniqueInstance called before PcaWeightsManager::SetUniqueInstance. Using "
                   "today's date as trading_date_" << std::endl;
      p_uniqueinstance_ = new PcaWeightsManager(DateTime::GetCurrentIsoDateLocal());
    }
    return *p_uniqueinstance_;
  }

  static void SetUniqueInstance(int _YYYYMMDD_) {
    // ideally every exec should set this in the main function
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new PcaWeightsManager(_YYYYMMDD_);
    } else {
      if (p_uniqueinstance_->trading_date_ != _YYYYMMDD_) {
        std::cerr << "PcaWeightsManager::SetUniqueInstance already called earlier with date: "
                  << p_uniqueinstance_->trading_date_ << ". While this date is: " << _YYYYMMDD_
                  << ". Continuing with the old date." << std::endl;
      }
    }
  }

  ~PcaWeightsManager() {}

  const EigenConstituentsVec &GetEigenConstituentVec(const std::string &_portfolio_descriptor_shortcode_) const {
    ShortcodeEigenConstituentsVecMapCIter_t _citer_ = shortcode_eigencompo_map_.find(_portfolio_descriptor_shortcode_);
    if (_citer_ != shortcode_eigencompo_map_.end()) {
      return _citer_->second;
    }
    std::cout << "Error coming here.....";
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, _portfolio_descriptor_shortcode_.c_str());
    return empty_;
  }

  const std::vector<double> &GetPortfolioStdevs(const std::string &_portfolio_descriptor_shortcode_) const {
    std::map<std::string, std::vector<double> >::const_iterator _citer_ =
        stdevs_for_portfolio_constituent_map_.find(_portfolio_descriptor_shortcode_);
    if (_citer_ != stdevs_for_portfolio_constituent_map_.end()) {
      return _citer_->second;
    }
    ExitVerbose(kPCAWeightManagerMissingPortFromMap, _portfolio_descriptor_shortcode_.c_str());
    return empty_stdev_;
  }

  double GetShortcodeStdevs(const std::string &_shortcode_) const {
    std::map<std::string, double>::const_iterator _citer_ = stdevs_for_shortcodes_map_.find(_shortcode_);
    if (_citer_ != stdevs_for_shortcodes_map_.end()) {
      return _citer_->second;
    }
    std::ostringstream temp_oss_;
    temp_oss_ << "Add stdev for shc_ " << _shortcode_
              << " in /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt";
    ExitVerbose(kPCAWeightManagerMissingshortcodeFromStdevMap, temp_oss_.str().c_str());
    return -1;
  }

  double GetShortcodeStdevsLongevity(const std::string &_shortcode_) const {
    std::map<std::string, double>::const_iterator _citer_ = stdevs_for_shortcodes_map_longevity_.find(_shortcode_);
    if (_citer_ != stdevs_for_shortcodes_map_longevity_.end()) {
      return _citer_->second;
    }
    std::ostringstream temp_oss_;
    temp_oss_ << "Add stdev for shc_ " << _shortcode_
              << " in /spare/local/tradeinfo/PCAInfo/shortcode_stdev_longevity.txt";
    ExitVerbose(kPCAWeightManagerMissingshortcodeFromStdevMap, temp_oss_.str().c_str());
    return -1;
  }

  inline int GetPortfolioShortCodeIndexer(const std::string &_portfolio_descriptor_shortcode_,
                                          const std::string &shortcode_) const {
    std::map<std::string, std::vector<std::string> >::const_iterator t_ps2csmap_citer_ =
        portfolio_shortcode_to_constituent_shortcode_map_.find(_portfolio_descriptor_shortcode_);
    if (t_ps2csmap_citer_ == portfolio_shortcode_to_constituent_shortcode_map_.end()) {
      //	  ExitVerbose ( kPCAWeightManagerMissingPortFromMap, ( _portfolio_descriptor_shortcode_ ).c_str( ) );
      return -1;
    }

    const std::vector<std::string> &constituent_shortcode_vec_ = t_ps2csmap_citer_->second;

    for (unsigned int tidx = 0u; tidx < constituent_shortcode_vec_.size(); tidx++) {
      if (constituent_shortcode_vec_[tidx].compare(shortcode_) == 0) {
        return (int)tidx;
      }
    }
    //      ExitVerbose ( kPCAWeightManagerMissingPortFromMap, ( _portfolio_descriptor_shortcode_ + " " + shortcode_
    //      ).c_str( ) );
    return -1;
  }

  inline bool IsPortfolioShortcode(const std::string &_portfolio_descriptor_shortcode_) {
    std::map<std::string, std::vector<std::string> >::const_iterator t_ps2csmap_citer_ =
        portfolio_shortcode_to_constituent_shortcode_map_.find(_portfolio_descriptor_shortcode_);
    if (t_ps2csmap_citer_ == portfolio_shortcode_to_constituent_shortcode_map_.end()) {
      return false;
    }
    return true;
  }

  void GetPortfolioShortCodeVec(const std::string &_portfolio_descriptor_shortcode_,
                                std::vector<std::string> &shortcode_vec_) const {
    std::map<std::string, std::vector<std::string> >::const_iterator t_ps2csmap_citer_ =
        portfolio_shortcode_to_constituent_shortcode_map_.find(_portfolio_descriptor_shortcode_);
    if (t_ps2csmap_citer_ == portfolio_shortcode_to_constituent_shortcode_map_.end()) {
      ExitVerbose(kPCAWeightManagerMissingPortFromMap, (_portfolio_descriptor_shortcode_).c_str());
      return;
    }

    const std::vector<std::string> &constituent_shortcode_vec_ = t_ps2csmap_citer_->second;

    shortcode_vec_.clear();
    for (unsigned int tidx = 0; tidx < constituent_shortcode_vec_.size(); tidx++) {
      shortcode_vec_.push_back(constituent_shortcode_vec_[tidx]);
      // VectorUtils::UniqueVectorAdd ( shortcode_vec_, constituent_shortcode_vec_[tidx] ) ; // not needed since it was
      // cleared above
    }
    return;
  }

  void AddPortfolioShortCodeVec(const std::string &_portfolio_descriptor_shortcode_,
                                std::vector<std::string> &shortcode_vec_) const {
    std::map<std::string, std::vector<std::string> >::const_iterator t_ps2csmap_citer_ =
        portfolio_shortcode_to_constituent_shortcode_map_.find(_portfolio_descriptor_shortcode_);
    if (t_ps2csmap_citer_ == portfolio_shortcode_to_constituent_shortcode_map_.end()) {
      ExitVerbose(kPCAWeightManagerMissingPortFromMap, (_portfolio_descriptor_shortcode_).c_str());
      return;
    }

    const std::vector<std::string> &constituent_shortcode_vec_ = t_ps2csmap_citer_->second;

    // shortcode_vec_.clear();
    for (unsigned int tidx = 0; tidx < constituent_shortcode_vec_.size(); tidx++) {
      VectorUtils::UniqueVectorAdd(shortcode_vec_, constituent_shortcode_vec_[tidx]);
    }
    return;
  }

  // Void PrintInfo
  void PrintPCAInfo();
  bool IsPCA(const std::string &);

 protected:
  // Read the PCAInfo file of the form ( first STDEV , then followed by n Eigenvectors)
  // PORTFOLIO_STDEV UBFUT 0.002759  0.013661  0.0304297  0.0747077
  // PORTFOLIO_EIGEN UBFUT 1 0.782413 0.347773 0.544683 0.5505 0.52851
  // PORTFOLIO_EIGEN UBFUT 2 0.178972 -0.93057 0.104831 0.229266 0.265494
  void LoadPCAInfoFile();
  void LoadFromFile(const std::string &, std::map<std::string, unsigned int> &, unsigned int);

  // Read the REGInfo file with portfolio dep weights
  // PWEIGHTS 6JES:NK_0 0.04715 -1.2333
  void LoadREGInfoFile();
  void LoadFromREGFile(const std::string &, std::map<std::string, unsigned int> &, unsigned int);

  /// Read the Portfolio file with the constituents
  /// PLINE UBFUT ZT_0 ZF_0 ZN_0 ZB_0
  /// index generated {UBFUT^ZT_0 0 }, {UBFUT^ZF_0, 1}...
  void LoadPCAConstituentInfo();
  void LoadStdevInfo();
  void LoadStdevInfoLongevity();
  void LoadPCAShortCodeConstituentsFromFile(const std::string &);

  // For RegPort files
  // LoadREGConstituentInfo: Loads the constituents for the portfolios
  // PLINE PORTFOLIO_NAME DEP START_TIME END_TIME INDEP1 Sign1 INDEP2 Sign2 ..
  void LoadREGConstituentInfo();
  void LoadREGShortCodeConstituentsFromFile(const std::string &);

  bool IsCorrectMonthForIbovespa(const std::string &this_portfolio_descriptor_shortcode, int intdate);
  bool CheckAndLoadConstituentList(const std::string &portfolio_shortcode);
  bool IsWeightBasedIndexPortfolio(const std::string &portfolio);
  bool IsWeightBasedSectorPortfolio(const std::string &portfolio);
  bool CheckAndLoadConstituentWeight(const std::string &portfolio_shortcode, std::vector<double> &constituent_weights);

  // Checks if the Portfolio is RegPort (based on presence of "^" in the name)
  bool IsRegBasedSectorPortfolio(const std::string &portfolio);
};
}

#endif  // BASE_INDICATORS_PCA_WEIGHTS_MANAGER
