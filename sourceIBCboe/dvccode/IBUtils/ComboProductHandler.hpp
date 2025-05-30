#pragma once
#include <iostream>
#include <numeric>
#include <algorithm>
#include <vector>
#include <string>
#include "dvccode/IBUtils/Contract.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include <unordered_map>
namespace HFSAT{

class ComboProductHandler{
    public:
    ComboProductHandler()=default;
    ComboProductHandler(ComboProductHandler &b)=delete;
    ~ComboProductHandler()=default;
    // Function to compute HCF of all integers and modify the vector
    static std::vector<std::pair<std::string, int32_t>> normalizeByHCF(const std::vector<std::pair<std::string, int32_t>>& input);
    static std::vector<std::pair<std::string, int32_t>> sortByIntAndString(const std::vector<std::pair<std::string, int32_t>>& input);
    static std::string convertComboVectorToString(const std::vector<std::pair<std::string, int32_t>>& input);
    static std::vector<std::pair<std::string, int32_t>> convertComboStringToVector(const std::string& comboProductString);
    static std::vector<std::pair<std::string, int32_t>> normalizeAndsortByIntAndString(const std::vector<std::pair<std::string, int32_t>>& input);
    static std::string getExchSymbolOfDayFromComboShortCode(const std::string& shortcode_);
    static Contract getContractFromCombinedString(const std::string& comboProductString,const int32_t &trading_date);
    static void initializeComboDataRequestor(std::string _control_ip_, int32_t _control_port_, std::string _req_combo_data_ip_, int32_t _req_combo_data_port_, DebugLogger* _dbglogger_,bool isliveTrading, int32_t startIdx=0);
    static std::string getShortCodeForComboProduct(std::string combo_product_string);
    static std::string getShortCodeForComboProduct(std::vector<std::pair<std::string, int32_t>>& combo_product_vec_);
    static std::string getComboProductStrForShortCode(std::string combo_product_shortcode_);
    static std::vector<std::pair<std::string, int32_t>> getComboProductVecForShortCode(std::string combo_product_shortcode_);


    private:
    // Function to compute GCD (HCF) of two numbers using Euclidean algorithm
    static inline int32_t gcd(int32_t a, int32_t b);
    //Not thread safe as of now
    static std::string getComboProductIfAlreadyRequested(std::string combo_product_shortcode_);
    static std::string getShcIfAlreadyRequested(std::string combo_product_string);
    static void addShcToProductMap(std::string combo_product_string, std::string shc_);
    static std::string generateRandomShc_(std::string combo_product_string_);

    static std::string control_ip;
    static int32_t control_port;
    static std::string req_combo_data_ip;
    static int32_t req_combo_data_port;
    static DebugLogger* dbglogger_;
    static bool isliveTrading;
    static std::unordered_map <std::string, std::string> combo_prod_shc;
    static std::unordered_map <std::string, std::string> combo_shc_prod;
    static int32_t start_idx_shc;
};

}
