/**
    \file Tools/get_exchange_symbol.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717

*/

#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <time.h>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"

void ParseCommandLineParams(const int argc, const char **argv, std::string &shortcode_, int &input_date_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode input_date_YYYYMMDD" << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
  }
}

/// input arguments : input_date
int main(int argc, char **argv) {
  std::string shortcode_ = "";
/*  int input_date_ = 20110101;
  ParseCommandLineParams(argc, (const char **)argv, shortcode_, input_date_);
  if (strncmp(shortcode_.c_str(), "NSE_", 4) == 0) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadNSESecurityDefinitions();
  } else if (shortcode_.substr(0, 3) == "HK_") {
    HFSAT::SecurityDefinitions::GetUniqueInstance(input_date_).LoadHKStocksSecurityDefinitions();
  }
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  printf("%s", HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_));
*/
//
  std::ifstream notis_api_file;
  notis_api_file.open(argv[1], std::ifstream::in);
  bool is_cm_ = false;
  if(std::string(argv[2]) == "CM"){
    is_cm_ = true;
  }else if (std::string(argv[2]) == "FO"){
    is_cm_ = false;
  }

  if (!notis_api_file.is_open()) {
    std::cerr << "UNABLE TO OPEN CHANNEL INFO FILE FOR READING :" << argv[1] << std::endl;
    exit(0); //return;
  }
//  std::cout << "FILE: " << argv[1] << std::endl;
  char readline_buffer_[1024];
  while (!notis_api_file.eof()) {
    memset(readline_buffer_, 0, sizeof(readline_buffer_));
    notis_api_file.getline(readline_buffer_, sizeof(readline_buffer_));
    //HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
    
    std::vector<char*> tokens_;

    char readline_buffer_copy_[1024];
    memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
    strcpy(readline_buffer_copy_, readline_buffer_);
    //std::cout << "LINE: " << readline_buffer_copy_ << std::endl;

    HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);

    //const std::vector<const char *> &tokens_ = t_tokenizer_.GetTokens();
    if (tokens_.size() >=21 && tokens_[0][0] != '#') {
      //std::cout << "VALID LINE: " << readline_buffer_copy_ << std::endl;
      //for(uint32_t i = 0; i< tokens_.size(); i++){
	//std::cout << i << " : " << tokens_[i] << std::endl;
      //}
      char mkt_ = tokens_[1][0];
      std::string trd_no_ = tokens_[2];
      uint32_t sec = std::stol(tokens_[3])/65536 + 315513000; //315532800;  
      static char trade_date_[64] = "";
      static char trade_time_[64] = "";
      memset(trade_date_, '\0', 64);
      memset(trade_time_, '\0', 64);
      time_t now_ = 0; //std::time(1610961406); //(long int)sec);
      now_ = sec + 19800; // for ist time
      strftime(trade_date_, 64, "%d%b%Y", localtime(&now_)); 
      trade_date_[3] = trade_date_[3] - 32;
      trade_date_[4] = trade_date_[4] - 32;
      strftime(trade_time_, 64, "%H:%M:%S", localtime(&now_)); 
      int32_t trd_qty_ = std::stol(tokens_[5]);
      double trd_price_ = std::stod(tokens_[6])/100;
      char bs_ = (tokens_[7][0] == '1') ? 'B' : 'S';
      std::string ord_no_ = tokens_[8];
      std::string branch_no_ = tokens_[9];
      std::string user_id_ = tokens_[10];
      std::string client_type_ = (tokens_[11][0] == '1' ) ? "CLI" : "PRO";
      std::string client_acc_no_ = tokens_[12];
      std::string cp_code_ = tokens_[13];


      int32_t act_type_ = std::stol(tokens_[14]);
      int32_t trans_code_ = std::stol(tokens_[15]);
      if(act_type_ != 2 || trans_code_ != 6001){
        std::cout << "ERROR: ord_no: " << ord_no_ 
       	    << " act_type: " << act_type_ 
      	    << " trans_code: " << trans_code_ << std::endl;
        continue;
      }

      if(is_cm_){
        std::string ctcl_id_ = tokens_[21];
        std::string symbol_ = tokens_[24];
        std::string series_ = tokens_[25];
        double trade_val_ = (double)(trd_qty_) * trd_price_;
        std::cout << std::fixed << trade_date_ << "," << mkt_ << "," << trd_no_ << "," << trade_time_ << ","
		  << symbol_ << "," << series_ << "," << trd_qty_ << "," << trd_price_ << ","
		  << trade_val_ << "," << bs_ << "," << ord_no_ << "," << branch_no_ << ","
		  << user_id_ << "," << client_type_ << "," << client_acc_no_ << "," << cp_code_ << ",,"
		  << ctcl_id_ <<std::endl;

      }else{
        /*int32_t act_type_ = std::stol(tokens_[14]);
        int32_t trans_code_ = std::stol(tokens_[15]);
        if(act_type_ != 2 || trans_code_ != 6001){
          std::cout << "ERROR: ord_no: " << ord_no_ 
	 	    << " act_type: " << act_type_ 
		    << " trans_code: " << trans_code_ << std::endl;
          continue;
        }*/
        std::string ctcl_id_ = tokens_[18];
        std::string status_ = tokens_[19];
        std::string symbol_ = tokens_[21];
        std::string instr_ = tokens_[23];
//
        uint32_t exp_sec = std::stol(tokens_[24]) + 315532800;  
        static char exp_date_[64] = "";
        //static char exp_time_[64] = "";
        memset(exp_date_, '\0', 64);
        //memset(exp_time_, '\0', 64);
        time_t exp_now_ = 0; //std::time(1610961406); //(long int)sec);
        exp_now_ = exp_sec;
        strftime(exp_date_, 64, "%d%b%Y", localtime(&exp_now_)); 
        exp_date_[3] = exp_date_[3] - 32;
        exp_date_[4] = exp_date_[4] - 32;
        //strftime(exp_time_, 64, "%H:%M:%S", localtime(&exp_now_)); 

//
        double strike_px_ = std::stod(tokens_[25]) / 100;
        std::string opt_type_ = tokens_[26];
        double trade_val_ = (double)(trd_qty_) * trd_price_;
        
        std::cout << std::fixed << std::setprecision(2) << trade_date_ << "," << mkt_ << "," << trd_no_ << "," << trade_time_ << ","
		  << instr_ << "," << symbol_ << "," << exp_date_ << "," << strike_px_ << "," << opt_type_ << ",         "
		  << trd_qty_ << ", " << trd_price_ << ", " << trade_val_ << "," << bs_ << "," << ord_no_ << "," 
		  << branch_no_ << "," << user_id_ << "," << client_type_ << "," << client_acc_no_ << "," 
		  << cp_code_ << "," << cp_code_ << ",,"
		  << status_ << "," << ctcl_id_ << std::endl;
      }
    }
  }
  notis_api_file.close();
//
}
