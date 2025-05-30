//This is used to request all the data from the IBKRDataManager which were requested today
#include <iostream>
#include <sstream>
#include <vector>

#include "dvccode/IBUtils/ComboProductHandler.hpp"
#include "dvccode/IBUtils/AddUpdateComboProductMap.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/cboe_security_definition.hpp"
#include "dvccode/Utils/get_todays_date_utc.hpp"

std::unordered_map<std::string,std::string> combinedSymbToUniqueId;
std::unordered_map<std::string,std::string> uniqueIdToCombinedSymb;

void LoadComboShcMapFile(){
  

  // std::string 
  std::string tradingdate_ = DateUtility::getTodayDateUTC();
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/files/tmp/comboProductMap." << tradingdate_;
  std::string comboProductMap_ = t_temp_oss_.str();

  // Check if the file exists
  std::ifstream infile_check(comboProductMap_);
  if (!infile_check.good()) {
    // File does not exist
    std::cerr<<"No such file exists :"<<comboProductMap_<<std::endl;
    exit(-1);
  }
  infile_check.close();

  // Open the file for reading
  std::ifstream infile(comboProductMap_);
  if (!infile) {
      std::cerr << "Failed to open file: " << comboProductMap_ << std::endl;
      exit(-1);
  }

  std::string line;
  while (std::getline(infile, line)) {
    std::istringstream iss(line);
    std::string comboSymb, uniqueId;

    // Use ',' as the delimiter
    if (std::getline(iss, comboSymb, ',') && std::getline(iss, uniqueId)) {
      combinedSymbToUniqueId[comboSymb] = uniqueId;
      uniqueIdToCombinedSymb[uniqueId] = comboSymb;
    }
  }

  infile.close();

}

int main(){
    combinedSymbToUniqueId.clear();
    uniqueIdToCombinedSymb.clear();
    std::string control_ip = "127.0.0.1";
    int32_t control_port=51517;
    int32_t control_combo_port=51217;
    HFSAT::DebugLogger dbglogger_(10240);
    // HFSAT::ExchangeSymbolManager::SetUniqueInstance(20250114);
    // HFSAT::SecurityDefinitions::GetUniqueInstance(20250114).SetExchangeType("CBOE");
    // HFSAT::SecurityDefinitions::GetUniqueInstance(20250114).LoadCBOESecurityDefinitions();
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate()).LoadCBOESecurityDefinitions();

    // std::cout<<HFSAT::SecurityDefinitions::ConvertDataSourceNametoExchSymbol("CBOE_SPXW_CE_5835.00_20250114")<<std::endl;
    // std::cout<<HFSAT::SecurityDefinitions::GetShortCodeListForStepValue
    LoadComboShcMapFile();
    for(auto symb_shc:combinedSymbToUniqueId){
        std::cout<<"Data symb: "<<symb_shc.first<<std::endl;
        std::cout<<"Shortcode : "<<symb_shc.second<<std::endl;
        HFSAT::IBUtils::AddUpdateComboProductMap::GetUniqueInstance(dbglogger_,control_ip,control_port,control_ip,control_combo_port).updateRequiredMapsForCombo(symb_shc.first,symb_shc.second);
    }
    
}