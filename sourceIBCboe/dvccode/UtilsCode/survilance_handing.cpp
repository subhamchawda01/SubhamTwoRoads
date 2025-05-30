#include <fstream>

#include "dvccode/Utils/survilance_handling.hpp"

namespace HFSAT {
std::unordered_map<int, std::string> SurveillanceHandling::surveillance_mapping;
SurveillanceHandling* SurveillanceHandling::unique_instance_ = nullptr;

SurveillanceHandling& SurveillanceHandling::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new SurveillanceHandling();
  }
  return *(unique_instance_);
}

// Reads last dumped saci generator from the file if present else starts from 1
SurveillanceHandling::SurveillanceHandling() {
    surveillance_mapping[1] = "Graded Surveillance Measure - Stage I";
    surveillance_mapping[2] = "Graded Surveillance Measure - Stage II";
    surveillance_mapping[3] = "Graded Surveillance Measure - Stage III";
    surveillance_mapping[4] = "Graded Surveillance Measure - Stage IV";
    surveillance_mapping[5] = "Graded Surveillance Measure - Stage V";
    surveillance_mapping[6] = "Graded Surveillance Measure - Stage VI";
    surveillance_mapping[11] = "Short Term Additional Surveillance Measure (STASM) - Stage I";
    surveillance_mapping[12] = "Short Term Additional Surveillance Measure (STASM) - Stage II";
    surveillance_mapping[13] = "Long Term Additional Surveillance Measure (LTASM) - Stage I";
    surveillance_mapping[14] = "Long Term Additional Surveillance Measure (LTASM) - Stage II";
    surveillance_mapping[15] = "Long Term Additional Surveillance Measure (LTASM) - Stage III";
    surveillance_mapping[16] = "Long Term Additional Surveillance Measure (LTASM) - Stage IV";
    surveillance_mapping[20] = "Insolvency and Bankruptcy Code (IBC) - Receipt of Disclosure or Recommenced scrip*";
    surveillance_mapping[21] = "Insolvency and Bankruptcy Code (ASM IBC) - Stage I";
    surveillance_mapping[22] = "Insolvency and Bankruptcy Code (ASM IBC) - Stage II";
    surveillance_mapping[23] = "Inter Creditor Agreement (ASM ICA) - Stage I";
    surveillance_mapping[24] = "Inter Creditor Agreement (ASM ICA) - Stage II";
    surveillance_mapping[25] = "Company with high promoter Encumbrance";
    surveillance_mapping[26] = "Company with high promoter as well as non-promoter Encumbrance";
    surveillance_mapping[30] = "Information list (unsolicited SMS)";
    surveillance_mapping[31] = "Current watch list (unsolicited SMS)";
    surveillance_mapping[32] = "Unsolicited Video";
    surveillance_mapping[33] = "Unsolicited Video and LTASM Stage IV";
    surveillance_mapping[34] = "Enhanced Surveillance Measure (ESM) - Stage I";
    surveillance_mapping[35] = "Enhanced Surveillance Measure (ESM) - Stage II";
    surveillance_mapping[36] = "ESM Stage I and GSM Stage 0";
    surveillance_mapping[37] = "ESM Stage II and GSM Stage 0";
    surveillance_mapping[50] = "LTASM Stage I and GSM Stage 0";
    surveillance_mapping[51] = "LTASM Stage II and GSM Stage 0";
    surveillance_mapping[52] = "LTASM Stage III and GSM Stage 0";
    surveillance_mapping[53] = "LTASM Stage IV and GSM Stage 0";
    surveillance_mapping[54] = "STASM Stage I and GSM Stage 0";
    surveillance_mapping[55] = "STASM Stage II and GSM Stage 0";
    surveillance_mapping[56] = "Company with high promoter as well as non-promoter Encumbrance and GSM Stage 0";
    surveillance_mapping[57] = "Company with high promoter Encumbrance and GSM Stage 0";
    surveillance_mapping[58] = "ASM IBC Stage I and GSM Stage 0";
    surveillance_mapping[59] = "ASM IBC Stage II and GSM Stage 0";
    surveillance_mapping[60] = "ASM ICA Stage I and GSM Stage 0";
    surveillance_mapping[61] = "ASM ICA Stage II and GSM Stage 0";
    surveillance_mapping[62] = "Insolvency and Bankruptcy Code (IBC) - Receipt of Disclosure or Recommenced scrip and GSM stage 0";
    surveillance_mapping[63] = "GSM stage I and Insolvency and Bankruptcy Code (IBC) - Receipt of Disclosure or Recommenced scrip";
    surveillance_mapping[64] = "GSM stage II and Insolvency and Bankruptcy Code (IBC) - Receipt of Disclosure or Recommenced scrip";
    surveillance_mapping[65] = "GSM stage III and Insolvency and Bankruptcy Code (IBC) - Receipt of Disclosure or Recommenced scrip";
    surveillance_mapping[66] = "GSM stage IV and Insolvency and Bankruptcy Code (IBC) - Receipt of Disclosure or Recommenced scrip";
    surveillance_mapping[80] = "Loss making for last 8 quarters";
    surveillance_mapping[81] = "Encumbrance of Promoters/promoter group shareholding more than 50%";
    surveillance_mapping[82] = "Scrip is in BZ/SZ series";
    surveillance_mapping[83] = "Company has failed to pay Annual listing fee";
    surveillance_mapping[84] = "Derivative contracts in the scrip to be moved out of F&O";
    surveillance_mapping[99] = "Shortlisted under Graded Surveillance Measure";
}

void SurveillanceHandling::CheckProductUnderSurvilance(std::string shortcode_){

    int survid = HFSAT::NSESecurityDefinitions::GetSurveillanceIndicator(shortcode_); 
    if ( surveillance_mapping.find(survid) != surveillance_mapping.end()){
        std::string input_;
        std::cout << "Caution: <" << surveillance_mapping[survid] << "("<< survid << ")>, would you like to continue?” Yes / No" << std::endl;
        std::cin >> input_;
        if ( input_ == "No" || input_ == "no"){
            std::cout << "Exiting... " << std::endl;
            exit(0);
        }
    }
    bool under_ban = HFSAT::NSESecurityDefinitions::IsShortcodeUnderBan(shortcode_);
    // check product is underban
    if (under_ban){
        std::string input_;
        std::cout << "Security is in BAN for Trade, would you like to continue? Yes / No" << std::endl;
        std::cin >> input_;
        if ( input_ == "No" || input_ == "no"){
            std::cout << "Exiting... " << std::endl;
            exit(0);
        }
    }
}

void SurveillanceHandling::GetSurveillanceLogging(std::string shortcode_){
    int survid = HFSAT::NSESecurityDefinitions::GetSurveillanceIndicator(shortcode_); 
    if ( surveillance_mapping.find(survid) != surveillance_mapping.end()){
        std::cout <<"Alert:: ShortCode " << shortcode_ << " " << surveillance_mapping[survid] << std::endl;
    }
}

}  // namespace HFSAT
