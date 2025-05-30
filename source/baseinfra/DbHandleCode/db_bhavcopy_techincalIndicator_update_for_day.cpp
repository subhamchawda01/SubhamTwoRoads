/**
   \file DbHandleCode/db_bhavcopy_update_for_day.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include <iostream>
#include <algorithm>
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "baseinfra/MinuteBar/db_update_nse.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MinuteBar/technical_indicator_generator.hpp"
#include "baseinfra/MinuteBar/db_cash_corporate_action.hpp"

#define NSE_BHAVCOPY_FILENAME_PREFIX "NSE_Files/BhavCopy/fo"

void LoadBhavCopyParams(const int t_date_, HFSAT::DbUpdateNse &db_update_nse) {
  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";
  std::vector<std::string> month_vec_ = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  int this_date = t_date_;

  // bhavcopy files for the date D are generated by the same date_stamp unlike nse_contract files
  // get the previous business day and load the data
  // this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_bhav_copy_file_name_oss_ << NSE_BHAVCOPY_FILENAME_PREFIX << "/" << prev_date_.substr(4, 2)
                               << prev_date_.substr(2, 2) << "/"
                               << "/fo" << prev_date_.substr(6, 2) << month_vec_[std::stoi(prev_date_.substr(4, 2)) - 1]
                               << prev_date_.substr(0, 4) << "bhav.csv";

  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();
  std::cout << "READING: " << nse_bhav_copy_file_name_ << std::endl;


  if (HFSAT::FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));


        std::vector<char *> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        std::string expr_date_;
        std::string trimmed_str_;
        // trim the inst_type field: contains spaces

	
        if (tokens_.size() >= 15) HFSAT::PerishableStringTokenizer::TrimString(tokens_[0], trimmed_str_, ' ');
        if ( tokens_.size() >= 15 && tokens_[0][0] != '#' && tokens_[0][0] != 'I' ) {


          //HFSAT::NSEInstrumentType_t inst_type_ = HFSAT::NSESecurityDefinitions::StringToInst(tokens_[0]);
          std::string underlying_ = tokens_[1];
          expr_date_ = HFSAT::NSESecurityDefinitions::GetDateInYYYYMMDDBhav(tokens_[2]);
          // if invalid date, don't add this entry
          if (expr_date_ == "") continue;
          int expr = std::atoi(expr_date_.c_str());
          double strike_price_ = atof(tokens_[3]);
          std::string call_or_put_ = tokens_[4];
          std::string _this_shortcode_;
           //  std::cout << "Underlying "<< underlying_ <<std::endl;
          std::string expiry_ = expr_date_.substr(0, 4) + "-" + expr_date_.substr(4, 2) + "-" + expr_date_.substr(6, 2);
          // std::cout << "Underlying "<< underlying_ << " EXP "<< expiry_ << " ST " << strike_price_ << " CALL " <<
          //  call_or_put_ <<std::endl;

          if (call_or_put_ == std::string("CE")) {
            _this_shortcode_ =
                HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(underlying_, expr, strike_price_, 1);
          } else if (call_or_put_ == std::string("PE")) {
            _this_shortcode_ =
                HFSAT::NSESecurityDefinitions::GetShortcodeFromCanonical(underlying_, expr, strike_price_, 0);
          } else {

            int index = HFSAT::NSESecurityDefinitions::GetContractNumberFromExpiry(expr, HFSAT::NSE_STKFUT, (char *)underlying_.c_str());
            if (index != 0 && (index != 1 && index != 2)) continue;
            _this_shortcode_ = "NSE_" + underlying_ + "_FUT" + std::to_string(index);
          }
          // std::cout<<"SHORTCODE: " << _this_shortcode_ <<std::endl;
	  
	  if( _this_shortcode_ == "INVALID"){

              db_update_nse.PrepareFutOptBhavCopyMultipleRows(
              "INVALID", tokens_[0], tokens_[1], expiry_, atof(tokens_[3]), tokens_[4], atof(tokens_[5]),
              atof(tokens_[6]), atof(tokens_[7]), atof(tokens_[8]), atof(tokens_[9]), atof(tokens_[10]),
              atof(tokens_[11]), atof(tokens_[12]), atof(tokens_[13]), tokens_[14],underlying_, expr_date_,
              strike_price_,0);
              continue;
          }
	  
          bool Kexist = HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(_this_shortcode_);
          if (Kexist == false) {
            std::cout << "kGetContractSpecificationMissingCode FOR " << _this_shortcode_ << std::endl;
            continue;
          }
          int lotsize = HFSAT::SecurityDefinitions::GetContractMinOrderSize(_this_shortcode_, t_date_);
          std::string exch_sym = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);
          // db_update_nse.UpdateBhavCopyFo
	  
	if( (trimmed_str_ == "FUTIDX") || (trimmed_str_ == "FUTSTK") ){
          db_update_nse.PrepareFoBhavCopyMultipleRows(
              exch_sym, tokens_[0], tokens_[1], expiry_, atof(tokens_[3]), tokens_[4], atof(tokens_[5]),
              atof(tokens_[6]), atof(tokens_[7]), atof(tokens_[8]), atof(tokens_[9]), atof(tokens_[10]),
              atof(tokens_[11]), atof(tokens_[12]), atof(tokens_[13]), tokens_[14], _this_shortcode_, expr_date_,
              strike_price_, lotsize);
	}
	  
	  db_update_nse.PrepareFutOptBhavCopyMultipleRows(
               exch_sym, tokens_[0], tokens_[1], expiry_, atof(tokens_[3]), tokens_[4], atof(tokens_[5]),
               atof(tokens_[6]), atof(tokens_[7]), atof(tokens_[8]), atof(tokens_[9]), atof(tokens_[10]),
               atof(tokens_[11]), atof(tokens_[12]), atof(tokens_[13]), tokens_[14], _this_shortcode_, expr_date_,
               strike_price_, lotsize);
        }
      }  // end while
    }

    db_update_nse.ExecuteBhavCopyFoMultipleRows();
    db_update_nse.UpdateDb("NSE_MTBT_BHAV_ONLY");
    db_update_nse.ExecuteBhavCopyFutOptMultipleRows();
    nse_bhavcopy_file_.close();
  }
 
  else {
    std::cerr << "Fatal error - could not read NSE Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
}

void LoadCMBhavCopyParams(const int t_date_, HFSAT::DbUpdateNse &db_update_nse) {
  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";
  std::vector<std::string> month_vec_ = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  int this_date = t_date_;

  // bhavcopy files for the date D are generated by the same date_stamp unlike nse_contract files
  // get the previous business day and load the data
  // this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_bhav_copy_file_name_oss_ << "NSE_Files/Margin_Files/Exposure_Files/cm" << prev_date_.substr(6, 2)
                               << month_vec_[std::stoi(prev_date_.substr(4, 2)) - 1] << prev_date_.substr(0, 4)
                               << "bhav.csv";
  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();
  std::cout << "CM BHAV COPY FILE: " << nse_bhav_copy_file_name_ << std::endl;
  if (HFSAT::FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char *> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        std::string expr_date_;
        std::string trimmed_str_;
        // trim the inst_type field: contains spaces
        if (tokens_.size() >= 9) HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str_, ' ');
        if (tokens_.size() >= 9 && tokens_[0][0] != '#' && (trimmed_str_ == "EQ")) {
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[0], trimmed_str_, ' ');
          std::string _this_shortcode_ = "NSE_" + trimmed_str_;
          // DB CALL
          std::string exp = std::to_string(HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(_this_shortcode_));
          if (exp == "-1") continue;
          std::string expiry_ = exp.substr(0, 4) + "-" + exp.substr(4, 2) + "-" + exp.substr(6, 2);
          // std::cout << "Underlying "<< _this_shortcode_ << " EXP "<< expiry_ <<std::endl;
          int strike_price = HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCode(_this_shortcode_);
          int lotsize = HFSAT::SecurityDefinitions::GetContractMinOrderSize(_this_shortcode_, t_date_);
          std::string exch_sym = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);
          //        std::cout<<"Exch_sym:  "<<  exch_sym <<std::endl;
          //          db_update_nse.UpdateBhavCopyCm
          db_update_nse.PrepareCmBhavCopyMultipleRows(
              exch_sym, tokens_[0], tokens_[1], atof(tokens_[2]), atof(tokens_[3]), atof(tokens_[4]), atof(tokens_[5]),
              atof(tokens_[6]), atof(tokens_[7]), atoi(tokens_[8]), atof(tokens_[9]), tokens_[10], atoi(tokens_[11]),
              tokens_[12], _this_shortcode_, expiry_, strike_price, lotsize);
          // std::cout << "sc, close:: " << shortcode_ << ", " <<  shortcode_2_last_close_[trimmed_str_] << "\n";
        }
      }  // end while
    }
    db_update_nse.ExecuteBhavCopyCmMultipleRows();
    nse_bhavcopy_file_.close();
  } else {
    std::cerr << "Fatal error - could not read NSE Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " input_date_YYYYMMDD " << std::endl;
    exit(0);
  }
  int tradingdate_ = atoi(argv[1]);
  HFSAT::DbUpdateNse &db_update_nse = HFSAT::DbUpdateNse::GetUniqueInstance(argv[1], false, true, true);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  std::cout << "FO BHAV COPY LOADING..." << std::endl;
  LoadBhavCopyParams(tradingdate_, db_update_nse);
  // LoadCDBhavCopyParams(tradingdate_);
  db_update_nse.UpdateDb("NSE_MTBT");
  std::cout << "CM BHAV COPY LOADING..." << std::endl;
  LoadCMBhavCopyParams(tradingdate_, db_update_nse);
  db_update_nse.UpdateDb("NSE_MTBT");
  db_update_nse.RemoveEntryForIndicator();  // removing technical indicator entries for current day
  HFSAT::CashCorporateAction::GetUniqueInstance();
  HFSAT::TechnicalsIndicators &techincal_indicator = HFSAT::TechnicalsIndicators::GetUniqueInstance(argv[1]);
  techincal_indicator.PushIndicatorToDB();
  techincal_indicator.DumpIndicatorToDB();


  return 0;
}
