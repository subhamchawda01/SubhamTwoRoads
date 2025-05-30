#include "dvccode/IBUtils/ComboProductHandler.hpp"
#include <sstream>
#include "dvccode/CDef/cboe_security_definition.hpp"
#include "dvccode/Utils/cboe_refdata_loader.hpp"
#include "dvccode/Utils/cboe_daily_token_symbol_handler.hpp"
#include "dvccode/IBUtils/AddUpdateComboProductMap.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
namespace HFSAT{

std::string ComboProductHandler::control_ip="";
int32_t ComboProductHandler::control_port=-1;
std::string ComboProductHandler::req_combo_data_ip="";
int32_t ComboProductHandler::req_combo_data_port=-1;
DebugLogger* ComboProductHandler::dbglogger_= nullptr;
bool ComboProductHandler::isliveTrading = false;
std::unordered_map <std::string, std::string> ComboProductHandler::combo_prod_shc;
std::unordered_map <std::string, std::string> ComboProductHandler::combo_shc_prod;
int32_t ComboProductHandler::start_idx_shc=0;
std::string ComboProductHandler::getComboProductIfAlreadyRequested(std::string combo_product_shortcode_){
    if(ComboProductHandler::combo_shc_prod.find(combo_product_shortcode_)!=ComboProductHandler::combo_shc_prod.end())
        return ComboProductHandler::combo_shc_prod[combo_product_shortcode_];
    return "INVALID";
}
std::string ComboProductHandler::getShcIfAlreadyRequested(std::string combo_product_string){
    if(ComboProductHandler::combo_prod_shc.find(combo_product_string)!=ComboProductHandler::combo_prod_shc.end())
        return ComboProductHandler::combo_prod_shc[combo_product_string];
    return "INVALID";
}
void ComboProductHandler::addShcToProductMap(std::string combo_product_string, std::string shc_){
    ComboProductHandler::combo_prod_shc[combo_product_string]=shc_;
    ComboProductHandler::combo_shc_prod[shc_]=combo_product_string;
}
std::string ComboProductHandler::getExchSymbolOfDayFromComboShortCode(const std::string& shortcode_){
    size_t pos = shortcode_.rfind('_'); // Find the last occurrence of '_'
    if (pos == std::string::npos) {
        // If '_' is not found, return an empty string or handle error
        return "";
    }
    return "CBOE_C"+shortcode_.substr(pos + 1); // Extract the substring after the last '_'
}

std::vector<std::pair<std::string, int32_t>> ComboProductHandler::normalizeByHCF(const std::vector<std::pair<std::string, int32_t>>& input) {
    if (input.empty()) return {};

    // Calculate the HCF of all integers in the vector
    int32_t hcf = input[0].second;
    if(hcf<0)hcf*=-1;
    for (size_t i = 1; i < input.size(); ++i) {
        if(input[i].second<0)
            hcf = gcd(hcf, (input[i].second)*(-1));
        else hcf = gcd(hcf, input[i].second);
    }

    // Modify the vector by dividing each integer by the HCF
    std::vector<std::pair<std::string, int32_t>> result = input;
    for (auto& entry : result) {
        entry.second /= hcf;
    }

    return result;
}
inline int32_t ComboProductHandler::gcd(int32_t a, int32_t b) {
    while (b != 0) {
        int32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}
// Sort first by int32_t value, then by string lexicographically
std::vector<std::pair<std::string, int32_t>> ComboProductHandler::sortByIntAndString(const std::vector<std::pair<std::string, int32_t>>& input) {
    std::vector<std::pair<std::string, int32_t>> result = input;

    // Sort using a custom comparator
    std::sort(result.begin(), result.end(), [](const std::pair<std::string, int32_t>& a, const std::pair<std::string, int32_t>& b) {
        // First, sort by int32_t value (non-decreasing order)
        if (a.second != b.second) {
            return a.second < b.second;
        }
        // If int32_t values are the same, sort by string lexicographically (non-decreasing order)
        return a.first < b.first;
    });

    return result;
}
std::string ComboProductHandler::convertComboVectorToString(const std::vector<std::pair<std::string, int32_t>>& input){
    if(input.size()==0){
        std::cerr<<"Please provide a non-empty combo product vector"<<std::endl;
        return "";
    }
    std::ostringstream comboInString;
    comboInString<<"COMBO";
    for(auto &shcRatio: input){
        comboInString<<"_"<<shcRatio.first<<"__"<<shcRatio.second;
    }
    return comboInString.str();
}
std::vector<std::pair<std::string, int32_t>> ComboProductHandler::convertComboStringToVector(const std::string& comboString) {
    std::vector<std::pair<std::string, int32_t>> result;

    // Ensure the string starts with "COMBO"
    const std::string prefix = "COMBO_";
    if (comboString.substr(0, prefix.size()) != prefix) {
        throw std::invalid_argument("Invalid format: string must start with 'COMBO_'");
    }

    std::istringstream ss(comboString.substr(prefix.size())); // Skip the "COMBO" part
    std::string token;

    // Parse key-value pairs
    std::string key="",value="";

    while (std::getline(ss, token, '_')) {
        key=token;
        while(std::getline(ss, token,'_')){
            if(token=="")break;
            key+="_"+token;
        }
        // std::cout<<key<<"\n";
        if (!std::getline(ss, token, '_')) {
            throw std::invalid_argument("Invalid format: missing value for key " + key);
        }

        int32_t value;
        try {
            value = std::stoi(token); // Convert value to int32_t
        } catch (const std::exception &e) {
            throw std::invalid_argument("Invalid value format: " + token);
        }

        result.emplace_back(key, value);
    }

    return result;
}

// Normalize first then Sort first by int32_t value, then by string lexicographically
std::vector<std::pair<std::string, int32_t>> ComboProductHandler::normalizeAndsortByIntAndString(const std::vector<std::pair<std::string, int32_t>>& input) {
    return ComboProductHandler::sortByIntAndString(ComboProductHandler::normalizeByHCF(input));
}

Contract ComboProductHandler::getContractFromCombinedString(const std::string& comboProductString,const int32_t &trading_date){
    std::vector<std::pair<std::string, int32_t>> comboProductVector=ComboProductHandler::convertComboStringToVector(comboProductString);
    HFSAT::Utils::CBOEDailyTokenSymbolHandler &cboe_token_handler = HFSAT::Utils::CBOEDailyTokenSymbolHandler::GetUniqueInstance(trading_date);

    Contract contract_comb;
    contract_comb.symbol = "SPX";
    contract_comb.secType = "BAG";
    contract_comb.currency = "USD";
    contract_comb.exchange = "CBOE";

    contract_comb.comboLegs.reset(new Contract::ComboLegList());
    for(auto shc_ratio_:comboProductVector){
        ComboLegSPtr leg(new ComboLeg);
        leg->conId = cboe_token_handler.GetTokenFromInternalSymbol(HFSAT::CBOESecurityDefinitions::GetDatasourcenameCBOE(shc_ratio_.first).c_str(),'F');
        if(shc_ratio_.second<0){
            leg->action = "SELL";
            leg->ratio = (shc_ratio_.second)*(-1);
        }else{
            leg->action = "BUY";
            leg->ratio = (shc_ratio_.second);
        }
        leg->exchange = "CBOE";
        contract_comb.comboLegs->push_back(leg);
    }
    // std::cout<<contract_comb.ToString()<<std::endl;
    return contract_comb;
}
std::string ComboProductHandler::generateRandomShc_(std::string combo_product_string_){
    std::ostringstream oss;
    oss << "CBOE_SPXW_COMBO_" << ComboProductHandler::start_idx_shc;
    ComboProductHandler::start_idx_shc ++;
    return oss.str();
}
std::string ComboProductHandler::getShortCodeForComboProduct(std::string combo_product_string_){
    std::string shc_=ComboProductHandler::getShcIfAlreadyRequested(combo_product_string_);
    if(shc_ != "INVALID"){
        return shc_;
    }
    if(ComboProductHandler::isliveTrading){
        shc_=HFSAT::IBUtils::AddUpdateComboProductMap::GetUniqueInstance(*dbglogger_,control_ip,control_port,req_combo_data_ip,req_combo_data_port).updateRequiredMapsForCombo(combo_product_string_);
    }else{
        shc_=ComboProductHandler::generateRandomShc_(combo_product_string_);
        HFSAT::IBUtils::AddUpdateComboProductMap::updateTheInternalMaps(combo_product_string_,shc_,HFSAT::GlobalSimDataManager::GetUniqueInstance(*dbglogger_).GetTradingDate());
    }
    ComboProductHandler::addShcToProductMap(combo_product_string_ , shc_);
    return shc_;
}
std::string ComboProductHandler::getShortCodeForComboProduct(std::vector<std::pair<std::string, int32_t>>& combo_product_vec_){
    std::string combo_product_string=ComboProductHandler::convertComboVectorToString(ComboProductHandler::normalizeAndsortByIntAndString(combo_product_vec_));
    return ComboProductHandler::getShortCodeForComboProduct(combo_product_string);
}

std::string ComboProductHandler::getComboProductStrForShortCode(std::string combo_product_shortcode_){
    return ComboProductHandler::getComboProductIfAlreadyRequested(combo_product_shortcode_);
}
std::vector<std::pair<std::string, int32_t>> ComboProductHandler::getComboProductVecForShortCode(std::string combo_product_shortcode_){
    std::string combo_product_string=ComboProductHandler::getComboProductStrForShortCode(combo_product_shortcode_);
    if("INVALID"==combo_product_string)return {};
    return ComboProductHandler::convertComboStringToVector(combo_product_string);
}
void ComboProductHandler::initializeComboDataRequestor(std::string _control_ip_, int32_t _control_port_, std::string _req_combo_data_ip_, int32_t _req_combo_data_port_, DebugLogger* _dbglogger_,bool _isliveTrading_, int32_t startIdx){
    ComboProductHandler::control_ip = _control_ip_;
    ComboProductHandler::control_port =_control_port_;
    ComboProductHandler::req_combo_data_ip= _req_combo_data_ip_;
    ComboProductHandler::req_combo_data_port= _req_combo_data_port_;
    ComboProductHandler::dbglogger_ = _dbglogger_;
    ComboProductHandler::isliveTrading = _isliveTrading_;
    ComboProductHandler::start_idx_shc = startIdx;
}


}
