/**
   \file Indicators/product_family_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#ifndef PRODUCT_FAMILY_MANAGER_HPP_
#define PRODUCT_FAMILY_MANAGER_HPP_

#include <vector>
#include <string>
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

// Helper class for ImpliedPriceMPS indicators
// hard coding everything for now
namespace HFSAT {
class ProductFamilyManager {
 public:
  static void GetProductFamilyWeights(std::string family_name, std::vector<double>& weights) {
    if (!family_name.compare("IN_BAX_ALL")) {
      for (auto i = 0u; i <= 6; ++i) weights[i] = 1.0;    // BAX_i
      for (unsigned int i = 7; i <= 14; ++i) weights[i] = 10.0;  // total 8 spreads
    } else if (!family_name.compare("IN_LFI_ALL")) {
      for (auto i = 0u; i <= 6; ++i) weights[i] = 1.0;    // LFI_i
      for (unsigned int i = 7; i <= 12; ++i) weights[i] = 10.0;  // total 6 spreads
    } else {
      // weights for some other family
    }
  }

  static void GetProductFamilyShortcodes(std::string family_name, std::vector<std::string>& shortcodes) {
    if (!family_name.compare("IN_BAX_ALL")) {
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "BAX_0");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "BAX_1");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "BAX_2");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "BAX_3");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "BAX_4");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "BAX_5");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "BAX_6");
      // VectorUtils::UniqueVectorAdd ( shortcodes, ( std::string ) "BAX_7" ) ;

      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX0_BAX1");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX0_BAX2");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX0_BAX3");

      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX1_BAX2");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX2_BAX3");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX3_BAX4");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX4_BAX5");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_BAX5_BAX6");
    } else if (!family_name.compare("IN_LFI_ALL")) {
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "LFI_0");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "LFI_1");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "LFI_2");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "LFI_3");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "LFI_4");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "LFI_5");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "LFI_6");
      // VectorUtils::UniqueVectorAdd ( shortcodes, ( std::string ) "LFI_7" ) ;

      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_LFI0_LFI1");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_LFI1_LFI2");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_LFI2_LFI3");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_LFI3_LFI4");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_LFI4_LFI5");
      VectorUtils::UniqueVectorAdd(shortcodes, (std::string) "SP_LFI5_LFI6");
    } else {
      // Add more such families and butterflies
    }
  }
};
}

#endif /* PRODUCT_FAMILY_MANAGER_HPP_ */
