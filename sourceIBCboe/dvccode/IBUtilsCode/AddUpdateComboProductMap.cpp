
#include "dvccode/IBUtils/AddUpdateComboProductMap.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/cboe_combo_token_generator.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/IBUtils/ComboProductHandler.hpp"
#include "dvccode/CDef/assumptions.hpp"
namespace HFSAT {
namespace IBUtils {
    AddUpdateComboProductMap::AddUpdateComboProductMap(DebugLogger& _dbglogger_,std::string _control_ip_, int32_t _control_port_,  std::string _req_combo_data_ip_, int32_t _req_combo_data_port_)
    : dbglogger_(_dbglogger_),
    cboe_daily_token_symbol_handler_(HFSAT::Utils::CBOEDailyTokenSymbolHandler::GetUniqueInstance(
        HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
    cboe_ref_data_loader_(HFSAT::Utils::CBOERefDataLoader::GetUniqueInstance(
        HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
    control_ip(_control_ip_),
    control_port(_control_port_),
    req_combo_data_ip(_req_combo_data_ip_),
    req_combo_data_port(_req_combo_data_port_),
    cmdRequestShortCode(control_ip,control_port),
    cmdRequestComboData(req_combo_data_ip,req_combo_data_port){}

    AddUpdateComboProductMap& AddUpdateComboProductMap::GetUniqueInstance(DebugLogger& _dbglogger_,std::string _control_ip_, int32_t _control_port_,  std::string _req_combo_data_ip_, int32_t _req_combo_data_port_) {
        static AddUpdateComboProductMap unique_instance(_dbglogger_,_control_ip_,_control_port_,_req_combo_data_ip_,_req_combo_data_port_);
        return unique_instance;
    }
    
    void AddUpdateComboProductMap::updateTheInternalMaps(std::string combo_product_string, std::string shortcode_, int32_t trading_date){
        //Add these things to assumption in macro
        double min_price_increment_=CBOE_COMBO_MIN_PRICE_INCREMENT;
        double numbers_to_dollars_=CBOE_COMBO_NUMBERS_TO_DOLLARS;
        int min_order_size_=CBOE_COMBO_MIN_ORDER_SIZE;
        std::string exchange_symbol=HFSAT::ComboProductHandler::getExchSymbolOfDayFromComboShortCode(shortcode_);
        // std::cout<<shortcode_<<" "<<exchange_symbol<<std::endl;
        HFSAT::SecurityDefinitions::AddCBOEComboSecurityDefinition(shortcode_, exchange_symbol, min_price_increment_, numbers_to_dollars_, min_order_size_);
        HFSAT::Utils::CBOERefDataLoader::GetUniqueInstance(trading_date).AddComboRefData(shortcode_);
        int64_t token = HFSAT::CBOEComboTokenGenerator::GetUniqueInstance().GetTokenOrUpdate(shortcode_);
        HFSAT::Utils::CBOEDailyTokenSymbolHandler::GetUniqueInstance(trading_date).UpdateTokenAndSymbolMap(token,shortcode_.c_str(),CBOE_COMBO_SEGMENT_MARKING);
    }

    //To be called before calling the common initializer because the security id and smv is not being made here, it will return shortcode
    std::string AddUpdateComboProductMap::updateRequiredMapsForCombo(std::string combo_product_string){
      std::string shortcode_="";
      // {"ADDCOMBOSYMBOL","COMBO_CBOE_SPXW_C0_O1__1_CBOE_SPXW_C0_A__-2_CBOE_SPXW_C0_O2__1"}; //Populate this with the required comboString
      std::vector<std::string> control_command_text_={"ADDCOMBOSYMBOL",combo_product_string};

      if(cmdRequestShortCode.sendControlCommand(control_command_text_,shortcode_) && shortcode_ != ""){
        std::vector<std::string> request_combo_data_command_text_={"LISTENCOMBOPRODUCTDATA",combo_product_string,shortcode_}; //Populate this with the required comboString
        if(cmdRequestComboData.sendControlCommandNoRes(request_combo_data_command_text_)){
            updateTheInternalMaps(combo_product_string,shortcode_,HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
        }else{
            std::cerr<<"There was am error sending data request command"<<std::endl;      
        }
      }else{
        std::cerr<<"Could not get the shortcode for the required combo_product"<<std::endl;
      }

      return shortcode_;
    }

    //To be called if you have the shortcode and combo product and want to request data from IBKRDataManager
    void AddUpdateComboProductMap::updateRequiredMapsForCombo(std::string combo_product_string,std::string shortcode_){
        std::vector<std::string> request_combo_data_command_text_={"LISTENCOMBOPRODUCTDATA",combo_product_string,shortcode_}; //Populate this with the required comboString
        if(cmdRequestComboData.sendControlCommandNoRes(request_combo_data_command_text_)){
            updateTheInternalMaps(combo_product_string,shortcode_,HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
        }else{
            std::cerr<<"There was am error sending data request command"<<std::endl;      
        }
      return;
    }
}
}
