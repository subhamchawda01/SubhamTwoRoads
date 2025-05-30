/**
   \file IndicatorsCode/pca_weights_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <iostream>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "dvctrade/Indicators/index_utils.hpp"

namespace HFSAT {

inline std::string ToString(int t_intdate_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << t_intdate_;
  return t_temp_oss_.str();
}

PcaWeightsManager* PcaWeightsManager::p_uniqueinstance_ = NULL;

void PcaWeightsManager::LoadPCAInfoFile() {
  std::map<std::string, unsigned int>
      date_first_seen_on_;  ///< used to make sure that we load it from the most recent results
  std::string _pca_eigen_info_filename_prefix_ = std::string(BASETRADEINFODIR) + "PCAInfo/pca_portfolio_stdev_eigen_";
  // std::string _pca_stdev_info_filename_prefix_ = std::string ( BASETRADEINFODIR ) + "PCAInfo/pca_portfolio_stdev_";
  std::string _pca_eigen_info_filename_ = _pca_eigen_info_filename_prefix_ + "DEFAULT.txt";
  // std::string _pca_stdev_info_filename_ = _pca_stdev_info_filename_prefix_ +"DEFAULT.txt";

  int t_intdate_ = DateTime::CalcPrevDay(trading_date_);
  // Try looking past 200 days
  _pca_eigen_info_filename_ = _pca_eigen_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  //_pca_stdev_info_filename_ = _pca_stdev_info_filename_prefix_ + ToString ( t_intdate_ ) + ".txt";
  // First Eigen Values
  for (unsigned int ii = 0; ii < 200; ii++) {
    if (FileUtils::exists(_pca_eigen_info_filename_)) {  // load from every fle that exists
      LoadFromFile(_pca_eigen_info_filename_, date_first_seen_on_, t_intdate_);
    }

    t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
    _pca_eigen_info_filename_ = _pca_eigen_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  }

  _pca_eigen_info_filename_ = _pca_eigen_info_filename_prefix_ + "DEFAULT.txt"; 

  if (!FileUtils::exists(_pca_eigen_info_filename_)) {
    std::cerr << "PcaWeightsManager::LoadPCAInfoFile even " << _pca_eigen_info_filename_ << " does not exist"
              << std::endl;
    ExitVerbose(kExitErrorCodeGeneral);
  }

  LoadFromFile(_pca_eigen_info_filename_, date_first_seen_on_, 0);
}

void PcaWeightsManager::LoadStdevInfo() {
  std::string _shortcode_stdev_info_filename_prefix_ = std::string(BASETRADEINFODIR) + "PCAInfo/shortcode_stdev_";
  std::map<std::string, unsigned int>
      date_first_seen_on_;  ///< used to make sure that we load it from the most recent results
  int t_intdate_ = DateTime::CalcPrevDay(trading_date_);
  std::string _shortcode_stdev_info_filename_ = _shortcode_stdev_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  for (unsigned int ii = 0; ii < 200; ii++) {
    if (FileUtils::exists(_shortcode_stdev_info_filename_)) {  // load from every fle that exists
      // std::cerr<< "Loading STDEV from File: " << _pca_stdev_info_filename_ << std::endl;
      LoadFromFile(_shortcode_stdev_info_filename_, date_first_seen_on_, t_intdate_);
    }

    t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
    _shortcode_stdev_info_filename_ = _shortcode_stdev_info_filename_prefix_ + ToString(t_intdate_) + ".txt";
  }

  _shortcode_stdev_info_filename_ = _shortcode_stdev_info_filename_prefix_ + "DEFAULT.txt";

  if (!FileUtils::exists(_shortcode_stdev_info_filename_)) {
    std::cerr << "PcaWeightsManager::LoadPCAInfoFile even " << _shortcode_stdev_info_filename_ << " does not exist"
              << std::endl;
    ExitVerbose(kExitErrorCodeGeneral);
  }

  LoadFromFile(_shortcode_stdev_info_filename_, date_first_seen_on_, 0);
}

void PcaWeightsManager::LoadStdevInfoLongevity() {
  std::string _shortcode_stdev_info_filename_ = std::string(BASETRADEINFODIR) + "PCAInfo/shortcode_stdev_longevity.txt";
  std::map<std::string, unsigned int> date_first_seen_on_;

  if (!FileUtils::exists(_shortcode_stdev_info_filename_)) {
    std::cerr << "PcaWeightsManager::LoadPCAInfoFile even " << _shortcode_stdev_info_filename_ << " does not exist"
              << std::endl;
    ExitVerbose(kExitErrorCodeGeneral);
  }

  LoadFromFile(_shortcode_stdev_info_filename_, date_first_seen_on_, 0);
}

void PcaWeightsManager::LoadFromFile(const std::string& _pca_info_filename_,
                                     std::map<std::string, unsigned int>& date_first_seen_on_,
                                     const unsigned int t_intdate_) {
  // std::cout << "LoadFromFile : " << _pca_info_filename_ << std::endl;
  std::ifstream pca_description_file;
  pca_description_file.open(_pca_info_filename_.c_str(), std::ifstream::in);
  if (pca_description_file.is_open()) {
    const int kPCAInfoDescriptionLen = 10240;
    char readline_buffer_[kPCAInfoDescriptionLen];
    bzero(readline_buffer_, kPCAInfoDescriptionLen);

    std::string portfolio_shortcode_name_ = "";
    std::string last_processed_shortcode_ = "";
    EigenConstituentsVec eigen_constitunent_vec_;
    // bool is_ready_to_pushed = false;

    while (pca_description_file.good()) {
      bzero(readline_buffer_, kPCAInfoDescriptionLen);
      pca_description_file.getline(readline_buffer_, kPCAInfoDescriptionLen);
      PerishableStringTokenizer st_(readline_buffer_, kPCAInfoDescriptionLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      // is_ready_to_pushed = false;
      if ((tokens_.size() > 2u) && (strncmp(tokens_[0], "PORTFOLIO_", 10) == 0)) {
        std::string _this_portfolio_descriptor_shortcode_ = tokens_[1];
        if (eigen_constitunent_vec_.size() > 0) eigen_constitunent_vec_.clear();
        if ((date_first_seen_on_.find(_this_portfolio_descriptor_shortcode_) == date_first_seen_on_.end()) ||
            (date_first_seen_on_[_this_portfolio_descriptor_shortcode_] <= t_intdate_)) {
          date_first_seen_on_[_this_portfolio_descriptor_shortcode_] = t_intdate_;

          if (strncmp(tokens_[0], "PORTFOLIO_EIGEN", 15) == 0) {
            // PORTFOLIO_EIGEN UBFUT 1 0.782413 0.347773 0.544683 0.5505 0.52851

            if (!IsCorrectMonthForIbovespa(_this_portfolio_descriptor_shortcode_, t_intdate_)) {
              continue;
            }

            EigenConstituents_t _this_portfolio_eigen_components_;

            if (stdevs_for_portfolio_constituent_map_.find(_this_portfolio_descriptor_shortcode_) ==
                stdevs_for_portfolio_constituent_map_.end())
              for (unsigned int ii = 0u;
                   ii < portfolio_shortcode_to_constituent_shortcode_map_[_this_portfolio_descriptor_shortcode_].size();
                   ii++) {
                if (IsWeightBasedIndexPortfolio(_this_portfolio_descriptor_shortcode_) ||
                    IsWeightBasedSectorPortfolio(_this_portfolio_descriptor_shortcode_) ||
                    IsRegBasedSectorPortfolio(_this_portfolio_descriptor_shortcode_)) {
                  stdevs_for_portfolio_constituent_map_[_this_portfolio_descriptor_shortcode_].push_back(1.0);
                } else {
                  stdevs_for_portfolio_constituent_map_[_this_portfolio_descriptor_shortcode_].push_back(
                      stdevs_for_shortcodes_map_[portfolio_shortcode_to_constituent_shortcode_map_
                                                     [_this_portfolio_descriptor_shortcode_][ii]]);
                }
              }

            if (IsWeightBasedIndexPortfolio(_this_portfolio_descriptor_shortcode_)) {
              CheckAndLoadConstituentWeight(_this_portfolio_descriptor_shortcode_,
                                            _this_portfolio_eigen_components_.eigenvec_components_);
            } else {
              _this_portfolio_eigen_components_.eigenval_ = atof(tokens_[3]);
              // Rest of the line is eigenvector components
              for (unsigned int ii = 4u; ii < tokens_.size(); ii++) {
                _this_portfolio_eigen_components_.eigenvec_components_.push_back(atof(tokens_[ii]));
              }
            }

            eigen_constitunent_vec_.push_back(_this_portfolio_eigen_components_);
            if (shortcode_eigencompo_map_.find(_this_portfolio_descriptor_shortcode_) ==
                shortcode_eigencompo_map_.end()) {
              shortcode_eigencompo_map_[_this_portfolio_descriptor_shortcode_] = eigen_constitunent_vec_;
            }
          }
        }
      }
      if ((tokens_.size() > 2u) && (strncmp(tokens_[0], "SHORTCODE_STDEV", 15) == 0) && std::string(tokens_[0]).length()==15) {
        std::string _this_shortcode_ = tokens_[1];
        if ((date_first_seen_on_.find(_this_shortcode_) == date_first_seen_on_.end()) ||
            (date_first_seen_on_[_this_shortcode_] <= t_intdate_))
          stdevs_for_shortcodes_map_[_this_shortcode_] = atof(tokens_[2]);
      }
      if ((tokens_.size() > 2u) && (strncmp(tokens_[0], "SHORTCODE_STDEV_LONGEVITY", 25) == 0)) {
        std::string _this_shortcode_ = tokens_[1];
        stdevs_for_shortcodes_map_longevity_[_this_shortcode_] = atof(tokens_[2]);
      }
    }
  }
}

// PLINE PORTFOLIO_NAME SHORTCODE1 SHORTCODE2.....
void PcaWeightsManager::LoadPCAShortCodeConstituentsFromFile(const std::string& _pca_constituent_filename) {
  std::ifstream pca_constituent_index_file_handle;
  pca_constituent_index_file_handle.open(_pca_constituent_filename.c_str(), std::ifstream::in);
  if (pca_constituent_index_file_handle.is_open()) {
    const int kPCAInfoDescriptionLen = 10240;
    char readline_buffer_[kPCAInfoDescriptionLen];
    bzero(readline_buffer_, kPCAInfoDescriptionLen);

    std::string portfolio_shortcode_name_ = "";
    while (pca_constituent_index_file_handle.good()) {
      bzero(readline_buffer_, kPCAInfoDescriptionLen);
      pca_constituent_index_file_handle.getline(readline_buffer_, kPCAInfoDescriptionLen);
      PerishableStringTokenizer st_(readline_buffer_, kPCAInfoDescriptionLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if ((tokens_.size() >= 3u) && strncmp(tokens_[0], "PLINE", 5) == 0) {
        std::string portfolio_shortcocde_ = tokens_[1];
        if (portfolio_shortcode_to_constituent_shortcode_map_.find(portfolio_shortcocde_) ==
            portfolio_shortcode_to_constituent_shortcode_map_.end()) {
          std::vector<std::string> temp;
          portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcocde_] = temp;
        }

        if (CheckAndLoadConstituentList(portfolio_shortcocde_)) {
          continue;
        }

        // PLINE + SHORTCODE
        for (unsigned int tidx = 2u; tidx < tokens_.size(); tidx++) {
          (portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcocde_]).push_back(tokens_[tidx]);
        }
      }
    }
    pca_constituent_index_file_handle.close();
  }
}

void PcaWeightsManager::LoadPCAConstituentInfo() {
  std::string _pca_constituent_filename_prefix_ = std::string(BASETRADEINFODIR) + "PCAInfo/portfolio_inputs";
  std::string _pca_default_constituent_filename_ = _pca_constituent_filename_prefix_ + "_DEFAULT";

  if (FileUtils::exists(_pca_constituent_filename_prefix_)) {
    LoadPCAShortCodeConstituentsFromFile(_pca_constituent_filename_prefix_);
  } else if (FileUtils::exists(_pca_default_constituent_filename_)) {
    LoadPCAShortCodeConstituentsFromFile(_pca_default_constituent_filename_);
  } else {
    std::cerr << "PcaWeightsManager::LoadPCAConstituentInfo even " << _pca_default_constituent_filename_
              << "doesnot exists" << std::endl;
    ExitVerbose(kExitErrorCodeGeneral);
  }
}

// Calls the loaFromREGFile for 200 working days so that every existing file is loaded

void PcaWeightsManager::LoadREGInfoFile() {
  std::map<std::string, unsigned int> date_first_seen_on_;  // map the portfolio to the most recent date
  std::string _reg_weights_info_filename_prefix_ = std::string(BASETRADEINFODIR) + "SupervisedPortInfo/reg_weights_";

  int t_intdate_ = DateTime::CalcPrevDay(trading_date_);
  std::string _reg_weights_info_filename_ = _reg_weights_info_filename_prefix_ + ToString(t_intdate_);

  // Try looking past 200 days
  for (unsigned int ii = 0; ii < 200; ii++) {
    if (FileUtils::exists(_reg_weights_info_filename_)) {  // load from every fle that exists
      LoadFromREGFile(_reg_weights_info_filename_, date_first_seen_on_, t_intdate_);
    }

    t_intdate_ = DateTime::CalcPrevDay(t_intdate_);
    _reg_weights_info_filename_ = _reg_weights_info_filename_prefix_ + ToString(t_intdate_);
  }
}

// Read the REGInfo file with portfolio dep weights given the filename, date_first_seen_on , and date for which it is
// called
// PWEIGHTS 6JES:NK_0 0.04715 -1.2333
void PcaWeightsManager::LoadFromREGFile(const std::string& _reg_info_filename_,
                                        std::map<std::string, unsigned int>& date_first_seen_on_,
                                        const unsigned int t_intdate_) {
  std::ifstream reg_description_file;
  reg_description_file.open(_reg_info_filename_.c_str(), std::ifstream::in);

  if (reg_description_file.is_open()) {
    const int kREGInfoDescriptionLen = 10240;
    char readline_buffer_[kREGInfoDescriptionLen];
    bzero(readline_buffer_, kREGInfoDescriptionLen);

    EigenConstituentsVec eigen_constitunent_vec_;

    while (reg_description_file.good()) {
      bzero(readline_buffer_, kREGInfoDescriptionLen);
      reg_description_file.getline(readline_buffer_, kREGInfoDescriptionLen);
      PerishableStringTokenizer st_(readline_buffer_, kREGInfoDescriptionLen);  // Tokenize the line
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if ((tokens_.size() > 2u) && (strncmp(tokens_[0], "PWEIGHTS", 8) == 0)) {  // Check if the first word is PWEIGHTS
        std::string _this_portfolio_descriptor_shortcode_(tokens_[1]);
        if (eigen_constitunent_vec_.size() > 0) eigen_constitunent_vec_.clear();
        EigenConstituents_t _this_portfolio_eigen_components_;

        if (_this_portfolio_eigen_components_.eigenvec_components_.size() > 0)
          _this_portfolio_eigen_components_.eigenvec_components_.clear();
        // Loads the weights for the portfolio only if either the portfolio is not present in the date_first_seen_map
        // or the given date is more recent than the date on which the portfolio was loaded
        if ((date_first_seen_on_.find(_this_portfolio_descriptor_shortcode_) == date_first_seen_on_.end())) {
          date_first_seen_on_[_this_portfolio_descriptor_shortcode_] = t_intdate_;
          // pushes 1 for the stdev of each constituent of the portfolio
          for (unsigned int ii = 0u;
               ii < portfolio_shortcode_to_constituent_shortcode_map_[_this_portfolio_descriptor_shortcode_].size();
               ii++) {
            stdevs_for_portfolio_constituent_map_[_this_portfolio_descriptor_shortcode_].push_back(1.0);
          }
          _this_portfolio_eigen_components_.eigenval_ = 1;  // sets the eigen value as 1
          for (unsigned int ii = 2u; ii < tokens_.size();
               ii++) {  // pushes the weights of the constituents in eignevec_components
            _this_portfolio_eigen_components_.eigenvec_components_.push_back(atof(tokens_[ii]));
          }
          eigen_constitunent_vec_.push_back(_this_portfolio_eigen_components_);
          // adds _this_portfolio_descriptor_shortcode_ to the shortcode_eigencompo_map_ with value
          // eigen_constitunent_vec_
          if (shortcode_eigencompo_map_.find(_this_portfolio_descriptor_shortcode_) ==
              shortcode_eigencompo_map_.end()) {
            shortcode_eigencompo_map_[_this_portfolio_descriptor_shortcode_] = eigen_constitunent_vec_;
          }
        }
      }
    }
  }
}

// Loads the constituents for the portfolios
// PLINE PORTFOLIO_NAME DEP START_TIME END_TIME INDEP1 Sign1 INDEP2 Sign2 ..
void PcaWeightsManager::LoadREGShortCodeConstituentsFromFile(const std::string& _reg_constituent_filename) {
  std::ifstream reg_constituent_index_file_handle;
  reg_constituent_index_file_handle.open(_reg_constituent_filename.c_str(), std::ifstream::in);
  if (reg_constituent_index_file_handle.is_open()) {
    const int kREGInfoDescriptionLen = 10240;
    char readline_buffer_[kREGInfoDescriptionLen];
    bzero(readline_buffer_, kREGInfoDescriptionLen);

    while (reg_constituent_index_file_handle.good()) {
      bzero(readline_buffer_, kREGInfoDescriptionLen);
      reg_constituent_index_file_handle.getline(readline_buffer_, kREGInfoDescriptionLen);
      PerishableStringTokenizer st_(readline_buffer_, kREGInfoDescriptionLen);
      const std::vector<const char*>& tokens_ = st_.GetTokens();
      if ((tokens_.size() >= 4u) && strncmp(tokens_[0], "PLINE", 5) == 0) {  // checks if the first word is PLINE
        std::string temp_portfolio_shortcocde_(tokens_[1]);
        std::string dep_portfolio_shortcocde_(tokens_[2]);
        std::string portfolio_shortcocde_ = temp_portfolio_shortcocde_ + ":" +
                                            dep_portfolio_shortcocde_;  // sets the portfolio shortcode as eg 6JES:NK_0
        if (portfolio_shortcode_to_constituent_shortcode_map_.find(portfolio_shortcocde_) ==
            portfolio_shortcode_to_constituent_shortcode_map_.end()) {
          std::vector<std::string> temp;
          portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcocde_] = temp;
        }

        // PLINE PORTFOLIO_NAME DEP START_TIME END_TIME INDEP1 Sign1 INDEP2 Sign2 .. so starts from 5 and adds 2
        for (unsigned int tidx = 5u; tidx < tokens_.size(); tidx += 2) {
          (portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcocde_]).push_back(tokens_[tidx]);
        }
      }
    }
    reg_constituent_index_file_handle.close();
  }
}
// LoadREGShortCodeConstituentsFromFile if SupervisedPortInfo/portfolio_inputs exists else throws an error
void PcaWeightsManager::LoadREGConstituentInfo() {
  std::string _reg_default_constituent_filename_ =
      std::string(BASETRADEINFODIR) + "SupervisedPortInfo/portfolio_inputs";

  if (FileUtils::exists(_reg_default_constituent_filename_)) {
    LoadREGShortCodeConstituentsFromFile(_reg_default_constituent_filename_);
  } else {
    std::cerr << "PcaWeightsManager::LoadRegConstituentInfo even " << _reg_default_constituent_filename_
              << "doesnot exists" << std::endl;
    ExitVerbose(kExitErrorCodeGeneral);
  }
}

void PcaWeightsManager::PrintPCAInfo() {
  std::map<std::string, std::vector<double> >::iterator it;
  for (it = stdevs_for_portfolio_constituent_map_.begin(); it != stdevs_for_portfolio_constituent_map_.end(); it++) {
    // For each for the instrument all print the PCA eigenvecs
    std::string portfolio_name = it->first;
    std::cout << portfolio_name << " " << portfolio_shortcode_to_constituent_shortcode_map_[portfolio_name].size()
              << std::endl;
    for (unsigned int tidx = 0u; tidx < portfolio_shortcode_to_constituent_shortcode_map_[portfolio_name].size();
         tidx++) {
      std::cout << portfolio_shortcode_to_constituent_shortcode_map_[portfolio_name][tidx] << ' ';
    }
    std::cout << std::endl;

    std::cout << "STDEV: " << portfolio_name << " " << (it->second).size() << std::endl;
    for (unsigned int ii = 0; ii < (it->second).size(); ii++) {
      std::cout << (it->second)[ii] << " ";
    }
    std::cout << std::endl;

    EigenConstituentsVec egvec = GetEigenConstituentVec(portfolio_name);
    for (unsigned int jj = 0; jj < egvec.size(); jj++) {
      std::cout << " " << egvec[jj].eigenval_ << " ";
      for (unsigned int ll = 0; ll < egvec[jj].eigenvec_components_.size(); ll++) {
        std::cout << egvec[jj].eigenvec_components_[ll] << " ";
      }
      std::cout << std::endl;
    }
  }
}

bool PcaWeightsManager::IsPCA(const std::string& t_portfolio_descriptor_shortcode_) {
  if (t_portfolio_descriptor_shortcode_.compare("SDI1F12") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1 F13") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F14") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F15") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F23") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F24") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F25") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F34") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F35") == 0 ||
      t_portfolio_descriptor_shortcode_.compare("SDI1F45") == 0 ||
      t_portfolio_descriptor_shortcode_.find(":") != std::string::npos) {
    return false;
  }
  return true;
}

bool PcaWeightsManager::IsCorrectMonthForIbovespa(const std::string& this_portfolio_descriptor_shortcode, int intdate) {
  if (this_portfolio_descriptor_shortcode.compare("BMFSIND") == 0 ||
      this_portfolio_descriptor_shortcode.compare("BMFSFrIND") == 0 ||
      this_portfolio_descriptor_shortcode.find("BMFEQFr") != std::string::npos) {
    // This will allow reading from the file of date which lies in the same interval as the trading_date
    // Other wise do not read the info for this portfolio
    // Not allowing it to read from default file
    int this_idx = ((trading_date_ % 10000) + 300) / 400;  // 1 2 3
    int this_file_idx = -1;
    if (intdate != 0) {
      this_file_idx = ((HFSAT::DateTime::CalcNextDay(intdate) % 10000) + 300) /
                      400;  // For day 1 the newest file read from would be of last month
    }
    if (this_file_idx != this_idx || ((trading_date_ / 10000) != (intdate / 10000))) {
      return false;
    }
  }
  return true;
}

bool PcaWeightsManager::CheckAndLoadConstituentList(const std::string& portfolio_shortcode) {
  if (portfolio_shortcode.compare("BMFSIND") == 0 || portfolio_shortcode.compare("BMFSFrIND") == 0) {
    // This portfolio constituent change every 4 month,
    GetConstituentListInIbovespaIndex(trading_date_,
                                      portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcode]);
    return true;
  } else if ((portfolio_shortcode.compare("NSENIFTYPORTF") == 0) ||
             (portfolio_shortcode.compare("NSEBANKNIFTYPORTF") == 0)) {
    std::string port = portfolio_shortcode;
    port = port.replace(0, 3, "");
    port = port.replace(port.end() - 5, port.end(), "");
    GetConstituentListInNIFTY(trading_date_, portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcode],
                              port);
    for (unsigned i = 0; i < portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcode].size(); i++) {
      std::string basename = portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcode][i];
      portfolio_shortcode_to_constituent_shortcode_map_[portfolio_shortcode][i] = basename + "_FUT0";
    }
    return true;
  }
  return false;
}

bool PcaWeightsManager::CheckAndLoadConstituentWeight(const std::string& portfolio_shortcode,
                                                      std::vector<double>& constitiuent_weights) {
  if (portfolio_shortcode.compare("BMFSIND") == 0 || portfolio_shortcode.compare("BMFSFrIND") == 0) {
    // This portfolio constituent change every 4 month,
    GetFractionInIbovespaIndex(trading_date_, constitiuent_weights);
    return true;
  } else if ((portfolio_shortcode.compare("NSENIFTYPORTF") == 0) ||
             (portfolio_shortcode.compare("NSEBANKNIFTYPORTF") == 0)) {
    std::string port = portfolio_shortcode;
    port = port.replace(0, 3, "");
    port = port.replace(port.end() - 5, port.end(), "");
    GetConstituentFractionInNIFTY(trading_date_, constitiuent_weights, port);
    return true;
  }
  return false;
}

bool PcaWeightsManager::IsWeightBasedIndexPortfolio(const std::string& portfolio) {
  if (portfolio.find("BMFSFrIND") != std::string::npos || portfolio.find("BMFSIND") != std::string::npos) {
    return true;
  } else if (portfolio.find("NSENIFTYPORTF") != std::string::npos) {
    return true;
  } else if (portfolio.find("NSEBANKNIFTYPORTF") != std::string::npos) {
    return true;
  }
  return false;
}

bool PcaWeightsManager::IsWeightBasedSectorPortfolio(const std::string& portfolio) {
  if (portfolio.find("BMFEQFr") != std::string::npos) {
    return true;
  }
  return false;
}

// Checks if the Portfolio is RegPort (based on presence of ":" in the name)
bool PcaWeightsManager::IsRegBasedSectorPortfolio(const std::string& portfolio) {
  if (portfolio.find(":") != std::string::npos) {
    return true;
  }
  return false;
}
}
