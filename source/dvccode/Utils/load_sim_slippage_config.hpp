/**
   \file SimMarketMaker/load_sim_slippage_config.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <map>
#include "dvccode/CDef/nse_security_definition.hpp"

#define SIM_CONFIG_FILENAME "baseinfra/OfflineConfigs/SimConfig/sim_config.txt"
#define DEFAULT_BARDATA_SLIPPAGE_IDXFUT 5
#define DEFAULT_BARDATA_SLIPPAGE_STKFUT 5
#define DEFAULT_BARDATA_SLIPPAGE_IDXOPT 30
#define DEFAULT_BARDATA_SLIPPAGE_STKOPT 30
#define DEFAULT_BARDATA_SLIPPAGE_EQ 15

namespace HFSAT {

class SimSlippageConfig {
 private:
  static SimSlippageConfig* p_uniqueinstance_;
  static std::unordered_map<std::string, double> slippage_value_map_;
 public:
  
  SimSlippageConfig() {
    LoadSimConfigFile();
  }
  static inline SimSlippageConfig& GetUniqueInstance() {
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new SimSlippageConfig();
    }
    return *(p_uniqueinstance_);
  }
 
  static void LoadSimConfigFile() {
    std::string sim_config_filename_ = FileUtils::AppendHome(SIM_CONFIG_FILENAME);
    std::ifstream sim_config_file_;
    // For jenkins support, we need to mention file paths used from basetrade repo relative to WORKDIR.
    const char* work_dir = getenv("WORKDIR");
    if (work_dir != nullptr) {
      const char* deps_install = getenv("DEPS_INSTALL");
      if (deps_install != nullptr) {
        sim_config_filename_ = std::string(deps_install) + "/baseinfra/OfflineConfigs/SimConfig/sim_config.txt";
      }
    }
    sim_config_file_.open(sim_config_filename_.c_str(), std::ifstream::in);
    if (!sim_config_file_.is_open()) {
      std::cout << " Could not open file " << sim_config_filename_ << std::endl;
      return;
    }
    char line_[1024];
    while (sim_config_file_.good()) {
      memset(line_, 0, sizeof(line_));
      sim_config_file_.getline(line_, sizeof(line_));
      PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
      const std::vector<const char*>& tokens_ = t_tokenizer_.GetTokens();
      if (tokens_.size() >= 3) {
        if (tokens_[0][0] == '#') {
          continue;
        } else if (!strncmp(tokens_[0], "SLIPPAGE", strlen("SLIPPAGE"))) {
              slippage_value_map_[tokens_[1]] = atof(tokens_[2]);
              std::cout << "SLIPPAGE " << tokens_[1] << " " << slippage_value_map_[tokens_[1]] << std::endl;
        }
      }
    }
    sim_config_file_.close();
  }

  static std::unordered_map<std::string, double>& GetSlippageMap() { return slippage_value_map_; }
   
  static double GetSlippageValueFromShortcode( std::string const& shortcode ) {
    //std::string shortcode = HFSAT::NSESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchange_sym);
    // HFSAT::NSESecurityDefinitions::GetNSECommission(shortcode)
    std::string inst_type_str = "NONE";
    double config_slippage = 0;

    HFSAT::NSEInstrumentType_t inst_type = HFSAT::NSESecurityDefinitions::GetInstrumentTypeFromShortCode(shortcode);

    std::vector<char*> tokens_;
    char readline_buffer_copy_[1024];
    memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
    strcpy(readline_buffer_copy_, shortcode.c_str());

    HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, "_", tokens_);

    std::string symbol = std::string(tokens_[0]) + "_" + std::string(tokens_[1]);  

    if (HFSAT::NSE_IDXFUT == inst_type)
      inst_type_str = "NSE_IDXFUT";
    else if (HFSAT::NSE_STKFUT == inst_type)
      inst_type_str = "NSE_STKFUT";
    else if (HFSAT::NSE_IDXOPT == inst_type)
      inst_type_str = "NSE_IDXOPT";
    else if (HFSAT::NSE_STKOPT == inst_type)
      inst_type_str = "NSE_STKOPT";
    else if (HFSAT::NSE_EQ == inst_type)
      inst_type_str = "NSE_EQ";

    if (slippage_value_map_.find(shortcode) != slippage_value_map_.end()) { 
      config_slippage = slippage_value_map_[shortcode] ;
    } else if (slippage_value_map_.find(symbol) != slippage_value_map_.end()) { 
      config_slippage = slippage_value_map_[symbol] ;
    } else if (slippage_value_map_.find(inst_type_str) != slippage_value_map_.end()) {
      config_slippage = slippage_value_map_[inst_type_str] ;
    } else if ("NSE_IDXFUT" == inst_type_str) {
      config_slippage = DEFAULT_BARDATA_SLIPPAGE_IDXFUT ;
    } else if ("NSE_STKFUT" == inst_type_str) {
      config_slippage = DEFAULT_BARDATA_SLIPPAGE_STKFUT ;
    } else if ("NSE_IDXOPT" == inst_type_str) {
      config_slippage = DEFAULT_BARDATA_SLIPPAGE_IDXOPT ;
    } else if ("NSE_STKOPT" == inst_type_str) {
      config_slippage = DEFAULT_BARDATA_SLIPPAGE_STKOPT ;
    } else if ("NSE_EQ" == inst_type_str) {
      config_slippage = DEFAULT_BARDATA_SLIPPAGE_EQ ;
    }
    
    return config_slippage; 
  }

};

SimSlippageConfig* SimSlippageConfig::p_uniqueinstance_ = NULL;
std::unordered_map<std::string, double> SimSlippageConfig::slippage_value_map_;

}
