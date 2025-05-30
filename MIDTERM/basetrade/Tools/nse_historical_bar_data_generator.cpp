#include <iostream> 
#include <vector>
#include <inttypes.h>
#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "baseinfra/MarketAdapter/nse_bar_data_generator.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

class BarDataPersistantManager : public HFSAT::BarUpdateListener {
 private:
  int number_sec_id;
  std::map<std::string, std::ofstream*> secid_to_outputstream;
  std::map<int, std::string> sec_id_to_underlying;
  std::vector<std::string> secid_to_hash_string;
  std::vector<bool> secid_to_isfut;
  HFSAT::SecurityNameIndexer &sec_symbol_indexer;
 public:
  BarDataPersistantManager(HFSAT::SecurityNameIndexer &_sec_symbol_indexer_, 
		  std::string _output_dir_, int num_sec_id, std::ios_base::openmode _flags_ = std::ios::out) : 
    number_sec_id(num_sec_id),
    secid_to_outputstream(),
    secid_to_hash_string(),
    secid_to_isfut(),
    sec_symbol_indexer(_sec_symbol_indexer_) {
//    secid_to_outputstream.resize(num_sec_id);
    secid_to_hash_string.resize(num_sec_id);
    secid_to_isfut.resize(num_sec_id);

//
/*  int nearest_monthly_expiry_ =
      HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
          "NSE_BANKNIFTY_C0_A");
  int nearest_weekly_expiry_ =
      HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
          "NSE_BANKNIFTY_C0_A_W");
  int next_weekly_expiry_ =
      HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
          "NSE_BANKNIFTY_C1_A_W");
  bool is_monthly_expiry_week = false;
  bool is_pre_monthly_expiry_week = false;
  if (nearest_weekly_expiry_ > nearest_monthly_expiry_) {
    is_monthly_expiry_week = true;
  } else if (next_weekly_expiry_ > nearest_monthly_expiry_) {
    is_pre_monthly_expiry_week = true;
  }
*/
/*
  std::cout << "nearest_monthly_expiry_: " << nearest_monthly_expiry_ << "\n"
    	    << "nearest_weekly_expiry_: " << nearest_weekly_expiry_ << "\n"
	    << "next_weekly_expiry_: " << next_weekly_expiry_ << "\n"
	    << "is_monthly_expiry_week: " << is_monthly_expiry_week << "\n"
	    << "is_pre_monthly_expiry_week: " << is_pre_monthly_expiry_week << std::endl;
*/

    for (int32_t ii = 0; ii < num_sec_id; ii++) {
      std::string shortcode = sec_symbol_indexer.GetShortcodeFromId(ii);
      std::string basename = shortcode.substr(shortcode.find_first_of("_") + 1);
      std::string underlying = basename.substr(0, basename.find_first_of("_"));
      std::string output_file = _output_dir_ + underlying;
      //std::cout << "shortcode, underlying: " << ii << " " << shortcode << " : " << underlying << " base: " << basename << std::endl;
      if(secid_to_outputstream.find(underlying) == secid_to_outputstream.end()){
	//std::cout << "shortcode Not Present: " << shortcode << std::endl;
        secid_to_outputstream[underlying] = new std::ofstream(output_file.c_str(), _flags_);
        *(secid_to_outputstream[underlying]) << std::fixed << std::setprecision(2);
      }
      sec_id_to_underlying[ii] = underlying;
      secid_to_hash_string[ii] =  underlying + "_FF_0_0";
      
      string exp_;
      vector<string> tokens_from_shortcode_;
      HFSAT::PerishableStringTokenizer::StringSplit(shortcode, '_',
                                                    tokens_from_shortcode_);
      bool is_option_ = (tokens_from_shortcode_.size() >= 4) ? true : false;
      bool is_equity_ = (tokens_from_shortcode_.size() == 2) ? true : false;
      
      bool is_spot_idx_ = HFSAT::NSESecurityDefinitions::IsSpotIndex(shortcode);

      if(is_spot_idx_){
	secid_to_hash_string[ii] = tokens_from_shortcode_[1] + "_FF_0_0" ;
        // std::cout << "SPOT IDX " << secid_to_hash_string[ii] << std::endl;
	secid_to_isfut[ii] = true;	
	//exp_ = to_string(
        //    HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode));
  // Case when FUTURE
      }else if(is_equity_){
        // std::cout << "EQUITY" << std::endl;
        secid_to_hash_string[ii] = tokens_from_shortcode_[1] + "_FF_0_0" ;
        secid_to_isfut[ii] = true;
      }else if (!is_option_) {
        // std::cout << "FUT" << std::endl;
      string temp = tokens_from_shortcode_[2];
      string expiry_id = temp.substr(temp.size() - 1, 1);
        secid_to_hash_string[ii] = tokens_from_shortcode_[1] + "_FF_0_" + expiry_id;
        secid_to_isfut[ii] = true;
        exp_ = to_string(
            HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(shortcode));
      }
      // Case when OPTION
      else {
        // std::cout << "OPTION" << std::endl;
      string temp = tokens_from_shortcode_[2];
      string expiry_id = temp.substr(temp.size() - 1, 1);
        
        string option_type;

        if (tokens_from_shortcode_[2][0] == 'C') {
          option_type = "CE";
        } else if (tokens_from_shortcode_[2][0] == 'P') {
          option_type = "PE";
        } else {
          std::cout
              << "ALERT : Received an invalid option code : "
              << " for symbol: " << shortcode << " ... Not processing further"
              << std::endl;
          return;
        }
        string strike_ = to_string(
            HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(
                shortcode));
//        std::cout << "shortcode, strike: " << shortcode << " , " << strike_ << std::endl;
        bool is_weekly_option =
            HFSAT::NSESecurityDefinitions::IsWeeklyOption(shortcode);

        secid_to_hash_string[ii] = tokens_from_shortcode_[1] + "_" + option_type + "_" +
                      strike_ + "_" + expiry_id;
        if (is_weekly_option) {
           secid_to_hash_string[ii] += "_W";
        }
        secid_to_isfut[ii] = false;
      }
    }
  }

  ~BarDataPersistantManager() {
    std::map<std::string, std::ofstream*>::iterator itr ;
    for (itr = secid_to_outputstream.begin(); itr != secid_to_outputstream.end(); itr++) {
      (itr->second)->flush();
      (itr->second)->close();
    }
  }

  void OnBarUpdate(int security_id, HFSAT::BarMetrics& metrics) {
    //std::cout << "OnBarUpdate: " << security_id << std::endl;
    if ( security_id >= number_sec_id ) {
      std::cerr << "Bar update for the symbol not requested : " 
	      << sec_symbol_indexer.GetSecurityNameFromId(security_id) << std::endl;
      return;
    }

//      std::string shortcode = sec_symbol_indexer.GetShortcodeFromId(security_id);
//      std::string basename = shortcode.substr(shortcode.find_first_of("_") + 1);
//      std::string underlying = basename.substr(0, basename.find_first_of("_"));
    std::string underlying_ = sec_id_to_underlying[security_id];
    if(secid_to_isfut[security_id]){
      *(secid_to_outputstream[underlying_]) << metrics.bar_time<< "\t"
              << secid_to_hash_string[security_id] << "\t"
              << metrics.start_time << "\t"
              << metrics.end_time << "\t"
              << metrics.expiry << "\t"
              << std::setprecision(2) << metrics.open << "\t"
              << metrics.close << "\t"
              << metrics.low << "\t"
              << metrics.high << "\t"
              << metrics.volume << "\t"
              << metrics.num_trades << "\t1\n";

    }else{
      *(secid_to_outputstream[underlying_]) << metrics.bar_time<< "\t"
              << secid_to_hash_string[security_id] << "\t"
              << metrics.start_time << "\t"
              << metrics.end_time << "\t"
              << metrics.expiry << "\t"
              << std::setprecision(2) << metrics.open << "\t"
              << metrics.close << "\t"
              << metrics.low << "\t"
              << metrics.high << "\t"
              << metrics.volume << "\t"
              << metrics.num_trades << "\t"
              << metrics.bid_price << "\t"
              << metrics.ask_price << "\n";
    } 
    secid_to_outputstream[underlying_]->flush();
  }
};

int main(int argc, char* argv[]) {
  if ( argc < 7 ) {
    std::cerr << "<EXEC> <TRADING DATE> <PRODUCTS FILE> <START TIME> <END TIME> <GRANULARITY> <OUTPUT DIR>" << std::endl;
    exit(0);
  }

  int tradingdate_ = atoi(argv[1]);
  std::string products_file = argv[2];
  int start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[3] + 4), argv[3]);
  int end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[4] + 4), argv[4]);
  int granulariy = atoi(argv[5]);
  std::string output_dir =  argv[6];
  std::vector<std::string> shortcode_vec_;
  int start_time = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, start_utc_hhmm_, "UTC_");
  int end_time =  HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, end_utc_hhmm_, "UTC_");
 
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  
  //parse shortcodes from file 
  std::ifstream shortcodes_filestream;
  shortcodes_filestream.open(products_file.c_str(), std::ifstream::in);
  if ( false == shortcodes_filestream.is_open()) {
    std::cerr << "Failed to open products file, Exiting" << std::endl;
    exit(0);
  }
  std::string line;
  while ( std::getline(shortcodes_filestream, line)){
    std::string shortcode(line);
    if (HFSAT::SecurityDefinitions::CheckIfContractSpecExists(shortcode, tradingdate_)) { 
      shortcode_vec_.push_back(shortcode); 
    }
  }
  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");
  CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_vec_, tradingdate_, false);
 
  common_smv_source->SetNetworkAccountInfoFilename(network_account_info_filename_);
  common_smv_source->Initialize();
 
  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  
  int num_sec_id = sec_name_indexer_.GetNumSecurityId();
  HFSAT::Watch &watch_ = common_smv_source->getWatch();
  HFSAT::SecurityMarketViewPtrVec & smv_map_ = common_smv_source->getSMVMap(); 
  BarDataPersistantManager *bardata_persistant_manager = new 
	  BarDataPersistantManager(sec_name_indexer_, output_dir, num_sec_id, std::ios::app);

  HFSAT::NSEBarDataGenerator &bar_data_generator = HFSAT::NSEBarDataGenerator::GetUniqueInstance(
		  sec_name_indexer_, watch_, granulariy, start_time, end_time, num_sec_id);
//  exit(0);
  bar_data_generator.AddBarUpdateListener(bardata_persistant_manager);
  
  for (auto item : smv_map_) {
    item->subscribe_price_type(&bar_data_generator, HFSAT::kPriceTypeMktSizeWPrice);
    item->subscribe_price_type(&bar_data_generator, HFSAT::kPriceTypeSpotIndex);
  } 
  std::cout << "RUN:" << std::endl;
  common_smv_source->Run();
  bar_data_generator.DayOver(); 
  return 0;	
}
