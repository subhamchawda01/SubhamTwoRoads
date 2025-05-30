#include <iostream>
#include "dvccode/IBUtils/ComboProductHandler.hpp"

using namespace HFSAT;

int main() {
    // Test data: pairs of strings and their associated int32_t values
    std::vector<std::pair<std::string, int32_t>> data = {
        {"CBOE_SPXW_C0_A", -18}, {"CBOE_SPXW_C0_I1", -54}, {"CBOE_SPXW_C0_O1", -36}, {"CBOE_SPXW_C0_O1", -36}, {"CBOE_SPXW_P0_O1", -18}
    };

    // Step 1: Normalize the data by HCF
    auto normalizedResult = ComboProductHandler::normalizeByHCF(data);
    std::cout << "After Normalization by HCF:\n";
    for (const auto& entry : normalizedResult) {
        std::cout << entry.first << ": " << entry.second << "\n";
    }

    // Step 2: Sort the normalized data first by int32_t value, then by string lexicographically
    auto sortedResult = ComboProductHandler::sortByIntAndString(normalizedResult);
    std::cout << "\nAfter Sorting by int32_t and string lexicographically:\n";
    for (const auto& entry : sortedResult) {
        std::cout << entry.first << ": " << entry.second << "\n";
    }
    std::string combStr= ComboProductHandler::convertComboVectorToString(sortedResult);
    std::cout<< combStr <<"\n";
    auto prevResult= ComboProductHandler::convertComboStringToVector(combStr);
    std::cout << "\nAfter reconversion:\n";
    for (const auto& entry : prevResult) {
        std::cout << entry.first << ": " << entry.second << "\n";
    }
    return 0;
}
