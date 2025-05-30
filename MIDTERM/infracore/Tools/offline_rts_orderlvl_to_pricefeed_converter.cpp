// =====================================================================================
//
//       Filename:  offline_ice_orderlvl_to_pricefeed_convertor.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/26/2013 11:30:32 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <stdlib.h>
#include <map>
#include <vector>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "infracore/lwfixfast/indexed_rts_order_book.hpp"
#include "infracore/lwfixfast/rts_converted_pf_listener.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#define DEBUG 0

namespace HFSAT {

class PriceFeedHandler : public RTSConvertedPFListener {
 public:
  BulkFileWriter bulk_file_writer_;

  PriceFeedHandler(std::string file_name_) {
    bulk_file_writer_.Open(file_name_);

    if (!bulk_file_writer_.is_open()) {
      std::cerr << " Could not open PriceFeed Output File : " << file_name_ << "\n";
      exit(-1);
    }
  }

  void ProcessMarketUpdate(RTS_MDS::RTSCommonStruct* rts_mds) {
    bulk_file_writer_.Write(rts_mds, sizeof(RTS_MDS::RTSCommonStruct));
    bulk_file_writer_.CheckToFlushBuffer();
  }
};
}

HFSAT::CpucycleProfiler& cpucycle_profiler_ = HFSAT::CpucycleProfiler::SetUniqueInstance(10);

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << " Usage < exec > < order_lvl_data > < price-feed file > <trading-date> <shortcode>\n";
    exit(-1);
  }

  int trading_date_ = atoi(argv[3]);

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  HFSAT::SecurityDefinitions& security_definitions_ = HFSAT::SecurityDefinitions::GetUniqueInstance(trading_date_);
  HFSAT::ShortcodeContractSpecificationMap this_contract_specification_map_ =
      security_definitions_.contract_specification_map_;
  HFSAT::ShortcodeContractSpecificationMapCIter_t itr_ = this_contract_specification_map_.begin();

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(trading_date_);
  std::string shc_(argv[4]);

  std::string this_exch_symbol_;
  double min_px_increment_;
  bool found = false;

  for (itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end(); itr_++) {
    std::string shortcode_ = (itr_->first);

    if (shortcode_ == shc_) {
      HFSAT::ContractSpecification contract_spec_ = (itr_->second);

      this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
      min_px_increment_ = contract_spec_.min_price_increment_;

      sec_name_indexer_.AddString(this_exch_symbol_.c_str(), shortcode_);

      found = true;

      break;
    }
  }

  if (!found) {
    std::cerr << " Shortcode " << shc_ << " not found \n";
    exit(-1);
  }

  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(argv[1]);

  if (!bulk_file_reader_.is_open()) {
    std::cerr << " Could not open OrderLevelData File : " << argv[1] << "\n";
    exit(-1);
  }

  cpucycle_profiler_.SetTag(1, "Data Conversion Time");

  HFSAT::RTSOrderBookForPF rts_order_book_(this_exch_symbol_, min_px_increment_, trading_date_, true);

  HFSAT::PriceFeedHandler price_feed_handler_(argv[2]);

  rts_order_book_.AddPriceFeedListener(&price_feed_handler_);

  RTS_MDS::RTSOFCommonStruct next_event_;
  size_t MDS_SIZE_ = sizeof(RTS_MDS::RTSOFCommonStruct);

  while (true) {
    size_t available_len_ = bulk_file_reader_.read(&next_event_, MDS_SIZE_);

    if (available_len_ < MDS_SIZE_) break;

    rts_order_book_.ProcessOrder(&next_event_);
  }

  bulk_file_reader_.close();

  std::cout << HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString() << "\n";

  return 0;
}
