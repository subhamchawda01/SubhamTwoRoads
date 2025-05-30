#include <iostream>
#include <sstream>
#include <vector>

#include "dvccode/IBUtils/ComboProductHandler.hpp"
#include "dvccode/IBUtils/AddUpdateComboProductMap.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/cboe_security_definition.hpp"

//CBOE_SPXW_PE_20250114_5720
std::string getTheDataSourceSymb(const std::string &option,const std::string &expirydate,const std::string &strikePrice){
    std::ostringstream data_source_;
    data_source_ << "CBOE_SPXW_"<<option<<"_"<<strikePrice<<"_"<<expirydate;
    return data_source_.str();
}
std::string getTheShortcodeReq(const std::string &option,const std::string &expirydate,const std::string &strikePrice){
    std::string data_source_symb=getTheDataSourceSymb(option,expirydate,strikePrice);
    std::cout<<"The data source is: "<<data_source_symb<<std::endl;
    std::string exchange_symb= HFSAT::SecurityDefinitions::ConvertDataSourceNametoExchSymbol(data_source_symb);
    std::cout<<"The exchange_symb is: "<<exchange_symb<<std::endl;
    if("INVALID" == exchange_symb){
        std::cerr<<"Exchange symb is:"<<exchange_symb<<std::endl;
        exit(-1);
    }
    return HFSAT::CBOESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchange_symb);
}
std::string getTheComboString(int numberLegs){
    std::vector<std::pair<std::string,int>> comboProduct;
    std::string option;
    std::string expirydate;
    std::string strikePrice;
    int weight;
    while(numberLegs--){
        
        std::cout<<"Enter the option :"<<std::endl;
        std::cin>>option;
        std::cout<<"Enter the expirydate :"<<std::endl;
        std::cin>>expirydate;
        std::cout<<"Enter the strikePrice :"<<std::endl;
        std::cin>>strikePrice;
        std::cout<<"Enter the weight :"<<std::endl;
        std::cin>>weight;

        std::string shc_ = getTheShortcodeReq(option,expirydate,strikePrice);
        if("INVALID" == shc_){
            std::cerr<<"Not a valid product please try again"<<std::endl;
            exit(-1);
        }
        comboProduct.push_back(make_pair(shc_,weight));

    }

    std::vector<std::pair<std::string,int>> comboProduct_sorted=HFSAT::ComboProductHandler::normalizeAndsortByIntAndString(comboProduct);
    return HFSAT::ComboProductHandler::convertComboVectorToString(comboProduct_sorted);
}
int main(int argc, char** argv){
    std::string control_ip = "127.0.0.1";
    int32_t control_port=51517;
    int32_t control_combo_port=51217;
    HFSAT::DebugLogger dbglogger_(10240);

    if(argc>1){
        control_port=atoi(argv[1]);
    }
    // HFSAT::ExchangeSymbolManager::SetUniqueInstance(20250114);
    // HFSAT::SecurityDefinitions::GetUniqueInstance(20250114).SetExchangeType("CBOE");
    // HFSAT::SecurityDefinitions::GetUniqueInstance(20250114).LoadCBOESecurityDefinitions();
    HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate()).LoadCBOESecurityDefinitions();
    HFSAT::ComboProductHandler::initializeComboDataRequestor(control_ip,control_port,control_ip,control_combo_port,&dbglogger_,true);

    // std::cout<<HFSAT::SecurityDefinitions::ConvertDataSourceNametoExchSymbol("CBOE_SPXW_CE_5835.00_20250114")<<std::endl;
    // std::cout<<HFSAT::SecurityDefinitions::GetShortCodeListForStepValue
    int numbProdct=0;
    std::cout<<"Enter the number of combo's you want to request:"<<std::endl;
    std::cin>>numbProdct;
    while(numbProdct--){
        std::string shortcode_ = "";
        int numbLegs=0;
        std::cout<<"Enter the number of legs for the combo:"<<std::endl;
        std::cin>>numbLegs;
        std::string required_combo_product=getTheComboString(numbLegs);
        std::cout<<required_combo_product<<std::endl;
        // HFSAT::ComboProductHandler::getContractFromCombinedString(required_combo_product,HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
        shortcode_=HFSAT::ComboProductHandler::getShortCodeForComboProduct(required_combo_product);
        std::cout<<"Received shortcode :"<<shortcode_<<std::endl;
    }
    
}