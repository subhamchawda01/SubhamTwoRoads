#include <iostream>
#include <sstream>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"
#include "dvctrade/Indicators/portfolio_price.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "basetrade/InitLogic/base_data_gen.hpp"
#include "dvctrade/Indicators/pca_deviation_pairs_port.hpp"

int main(int argc, char** argv) {
  //"INDICATOR WT IND_NAME  ZF_0 UBFUT 30.0 1 MktSizeWPrice"

  if (argc < 3) {
    std::cerr << "EXEC PLINE_FILENAME DATE(YYYYMMDD)" << std::endl;
    exit(0);
  }

  int yyyymmdd = atoi(argv[2]);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd);
  HFSAT::PcaWeightsManager::SetUniqueInstance(yyyymmdd);

  // Setup dbglogger, etc
  HFSAT::DebugLogger dbglogger_(1024000, 1);
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/port_wt_comparer."
                << "." << yyyymmdd;
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }
  HFSAT::Watch watch_(dbglogger_, yyyymmdd);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::ShortcodeSecurityMarketViewMap& shortcode_smv_map_ =
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();  ///< Unique Instance of map from shortcode to p_smv_

  unsigned int buffer_len = 10240;
  char buffer[buffer_len];
  std::ifstream pline_file_in_;
  pline_file_in_.open(argv[1], std::ifstream::in);
  while (pline_file_in_.is_open()) {
    if (pline_file_in_.good()) {
      bzero(buffer, buffer_len);
      pline_file_in_.getline(buffer, buffer_len);
      HFSAT::PerishableStringTokenizer st_(buffer, buffer_len);
      const std::vector<const char*>& tokens_ = st_.GetTokens();

      if (tokens_.size() >= 3) {
        std::vector<const char*> r_tokens_;
        r_tokens_.push_back("INDICATOR");
        r_tokens_.push_back("0.001");
        r_tokens_.push_back("RANDOM_INDICATOR_NAME");
        r_tokens_.push_back(tokens_[2]);
        r_tokens_.push_back(tokens_[1]);
        r_tokens_.push_back("30");
        r_tokens_.push_back("1");
        r_tokens_.push_back("MktSizeWPrice");

        std::cout << "Portfolio_shortcode: " << tokens_[1] << std::endl;
        for (unsigned int ii = 0; ii < r_tokens_.size(); ii++) {
          std::cout << r_tokens_[ii] << " ";
        }
        std::cout << std::endl;

        std::string _this_shortcode_ = std::string(tokens_[2]);

        // HFSAT::ExchSource_t   _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource
        // (_this_shortcode_ ) ;

        const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_this_shortcode_);
        sec_name_indexer_.AddString(exchange_symbol_, _this_shortcode_);
        //	  sid_to_exch_source_map_.push_back ( HFSAT::SecurityDefinitions::GetContractExchSource (tokens_[2])) ;

        // HFSAT::SecurityMarketView * p_smv_ = new HFSAT::SecurityMarketView ( dbglogger_, watch_, sec_name_indexer_,
        // _this_shortcode_, exchange_symbol_, _this_sid_, _this_exch_source_ ) ;
        // shortcode_smv_map_.AddEntry (_this_shortcode_, p_smv_);

        // Initialize other shortcodes of interest
        std::vector<std::string> source_shortcode_vec_;
        HFSAT::PCAPortPrice::CollectShortCodes(source_shortcode_vec_, std::string(tokens_[1]));
        for (auto i = 0u; i < source_shortcode_vec_.size(); i++) {
          if (!sec_name_indexer_.HasString(source_shortcode_vec_[i])) {
            const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(source_shortcode_vec_[i]);
            sec_name_indexer_.AddString(exchange_symbol_, source_shortcode_vec_[i]);
          }
        }

        for (auto i = 0u; i < sec_name_indexer_.NumSecurityId(); i++) {
          std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
          const char* _this_exchange_symbol_ = sec_name_indexer_.GetSecurityNameFromId(i);
          HFSAT::ExchSource_t _this_exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(_this_shortcode_);

          HFSAT::SecurityMarketView* p_smv_ = new HFSAT::SecurityMarketView(
              dbglogger_, watch_, sec_name_indexer_, _this_shortcode_, _this_exchange_symbol_, i, _this_exch_source_);
          shortcode_smv_map_.AddEntry(_this_shortcode_, p_smv_);  // add to shortcode_ to SMV* map
        }

        HFSAT::PCADeviationPairsPort::GetUniqueInstance(dbglogger_, watch_, r_tokens_);

      } else {
        exit(0);
      }
    }
  }
}
