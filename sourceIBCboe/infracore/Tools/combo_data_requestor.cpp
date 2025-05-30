#include <iostream>
#include <sstream>
#include <vector>

#include "dvccode/IBUtils/ComboProductHandler.hpp"
#include "dvccode/IBUtils/AddUpdateComboProductMap.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

inline void addAndDecideATM(std::ostringstream &internal_symbol_str, int atm,const std::string &option){
    if(atm<0){
        if(option == "C0"){
            internal_symbol_str << "I" << (atm*(-1));
        }else{
            internal_symbol_str << "O" << (atm*(-1));
        }
    }else if(atm==0){
        internal_symbol_str << "A";
    }else{
        if(option == "C0"){
            internal_symbol_str << "O" << atm;
        }else{
            internal_symbol_str << "I" << atm;
        }
    }
}
void addTheOptionLegWeightPair(std::vector<std::pair<std::string,int>> &comboProduct,const std::string &option,int atm,int weight){
    std::ostringstream leg;
    leg << "CBOE_SPXW_"<<option<<"_";
    addAndDecideATM(leg,atm,option);
    comboProduct.push_back(make_pair(leg.str(),weight));
}
std::string getTheComboString(int atm, int above,int below){
    std::vector<std::pair<std::string,int>> comboProduct;
    addTheOptionLegWeightPair(comboProduct,"C0",atm,-1);
    addTheOptionLegWeightPair(comboProduct,"C0",atm+above,1);
    addTheOptionLegWeightPair(comboProduct,"P0",atm,-1);
    addTheOptionLegWeightPair(comboProduct,"P0",atm-below,1);

    std::vector<std::pair<std::string,int>> comboProduct_sorted=HFSAT::ComboProductHandler::normalizeAndsortByIntAndString(comboProduct);
    return HFSAT::ComboProductHandler::convertComboVectorToString(comboProduct_sorted);
}
std::string getTheComboStringCallSpread(int atm, int above,int below){
    std::vector<std::pair<std::string,int>> comboProduct;
    addTheOptionLegWeightPair(comboProduct,"C0",atm,-1);
    addTheOptionLegWeightPair(comboProduct,"C0",atm+above,1);

    std::vector<std::pair<std::string,int>> comboProduct_sorted=HFSAT::ComboProductHandler::normalizeAndsortByIntAndString(comboProduct);
    return HFSAT::ComboProductHandler::convertComboVectorToString(comboProduct_sorted);
}
std::string getTheComboStringPutSpread(int atm, int above,int below){
    std::vector<std::pair<std::string,int>> comboProduct;
    addTheOptionLegWeightPair(comboProduct,"P0",atm,-1);
    addTheOptionLegWeightPair(comboProduct,"P0",atm-below,1);

    std::vector<std::pair<std::string,int>> comboProduct_sorted=HFSAT::ComboProductHandler::normalizeAndsortByIntAndString(comboProduct);
    return HFSAT::ComboProductHandler::convertComboVectorToString(comboProduct_sorted);
}
int main(int argc, char** argv){
    std::string control_ip = "127.0.0.1";
    int32_t control_port=72317;
    int32_t control_combo_port=51217;
    HFSAT::DebugLogger dbglogger_(10240);
    int32_t atm=0;
    if(argc>1){
        atm=atoi(argv[1]);
    }
    if(argc>4){
        control_port=atoi(argv[4]);
    }
    std::cout<<"Control Port:"<<control_port<<std::endl;
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate()).LoadCBOESecurityDefinitions();
    HFSAT::ComboProductHandler::initializeComboDataRequestor(control_ip,control_port,control_ip,control_combo_port,&dbglogger_,true);
    int above =1, below =1;
    if(argc>2){
        above=atoi(argv[2]);
    }

    if(argc>3){
        below=atoi(argv[3]);
    }
    
    for(int i=atm-5;i<=atm+5;i++){
        std::cout<<"=================================New=============================="<<std::endl;
        std::string shortcode_ = "",call_spread_shortcode_= "",put_spread_shortcode_= "";
        std::string required_combo_product=getTheComboString(i,above,below);
        std::cout<<required_combo_product<<std::endl;
        // HFSAT::ComboProductHandler::getContractFromCombinedString(required_combo_product,HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
        shortcode_=HFSAT::ComboProductHandler::getShortCodeForComboProduct(required_combo_product);
        std::cout<<"Received shortcode :"<<shortcode_<<std::endl;

        required_combo_product=getTheComboStringCallSpread(i,above,below);
        std::cout<<required_combo_product<<std::endl;
        // HFSAT::ComboProductHandler::getContractFromCombinedString(required_combo_product,HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
        call_spread_shortcode_=HFSAT::ComboProductHandler::getShortCodeForComboProduct(required_combo_product);
        std::cout<<"Received shortcode :"<<call_spread_shortcode_<<std::endl;

        required_combo_product=getTheComboStringPutSpread(i,above,below);
        std::cout<<required_combo_product<<std::endl;
        // HFSAT::ComboProductHandler::getContractFromCombinedString(required_combo_product,HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
        put_spread_shortcode_=HFSAT::ComboProductHandler::getShortCodeForComboProduct(required_combo_product);
        std::cout<<"Received shortcode :"<<put_spread_shortcode_<<std::endl;
    }
    
}