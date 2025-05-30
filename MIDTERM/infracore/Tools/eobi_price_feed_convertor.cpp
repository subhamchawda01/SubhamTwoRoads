/**
   \file eobi_price_feed_convertor.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " shortcode yyyymmdd" << std::endl;
    exit(-1);
  }

  std::string shortcode_(argv[1]);
  int date_ = atoi(argv[2]);

  // Add delay in microseconds
  int delay_ = 0;

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(date_);
  const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::ExchSource_t exch_source_ = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, date_);
  HFSAT::TradingLocation_t trading_location_ = HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_source_);

  std::string filename_ = HFSAT::EOBILoggedMessageFileNamer::GetName(exchange_symbol_, date_, trading_location_);

  EOBI_MDS::EOBICommonStruct next_event_;
  EUREX_MDS::EUREXCommonStruct price_event_;
  EUREX_MDS::EUREXLSCommonStruct price_ls_event_;

  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(filename_);

  while (true) {
    size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(EOBI_MDS::EOBICommonStruct));
    if (available_len_ >= sizeof(EOBI_MDS::EOBICommonStruct)) {
    } else {
      // data not found in file
      break;
    }
  }

  return 0;
}
