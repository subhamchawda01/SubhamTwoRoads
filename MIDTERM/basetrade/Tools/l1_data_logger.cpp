#include <cstdlib>

#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "basetrade/Tools/l1_data_logger.hpp"

namespace HFSAT {

L1DataLogger::L1DataLogger(const std::string log_file_path, const Watch& watch) : watch_(watch) {
  bulk_file_writer_.Open(log_file_path);
  if (!bulk_file_writer_.is_open()) {
    std::cerr << "Could not create file at " << log_file_path << ". Exiting.\n";
    exit(1);
  }
}

L1DataLogger::~L1DataLogger() {
  bulk_file_writer_.DumpCurrentBuffer();
  bulk_file_writer_.Close();
}

void L1DataLogger::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  GenericL1DataStruct l1_data_;

  std::string exchange_symbol_ = _market_update_info_.secname_;

  strncpy(l1_data_.symbol, exchange_symbol_.c_str(),
          std::min(GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE - 1, (int)exchange_symbol_.size()));
  l1_data_.symbol[std::min(GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE - 1, (int)exchange_symbol_.size())] = '\0';
  l1_data_.time = watch_.tv();
  l1_data_.type = GenericL1DataType::L1_DELTA;
  l1_data_.delta.bestbid_price = _market_update_info_.bestbid_price_;
  l1_data_.delta.bestbid_size = _market_update_info_.bestbid_size_;
  l1_data_.delta.bestbid_ordercount = _market_update_info_.bestbid_ordercount_;
  l1_data_.delta.bestask_price = _market_update_info_.bestask_price_;
  l1_data_.delta.bestask_size = _market_update_info_.bestask_size_;
  l1_data_.delta.bestask_ordercount = _market_update_info_.bestask_ordercount_;

  bulk_file_writer_.Write(&l1_data_, sizeof(GenericL1DataStruct));
  bulk_file_writer_.CheckToFlushBuffer();
}

void L1DataLogger::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                const MarketUpdateInfo& _market_update_info_) {
  GenericL1DataStruct l1_data_;

  std::string exchange_symbol_ = _market_update_info_.secname_;

  strncpy(l1_data_.symbol, exchange_symbol_.c_str(),
          std::min(GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE - 1, (int)exchange_symbol_.size()));
  l1_data_.symbol[std::min(GENERIC_L1_DATA_EXCHANGE_SYMBOL_SIZE - 1, (int)exchange_symbol_.size())] = '\0';
  l1_data_.time = watch_.tv();
  l1_data_.type = GenericL1DataType::L1_TRADE;
  l1_data_.trade.side = _trade_print_info_.buysell_;
  l1_data_.trade.price = _trade_print_info_.trade_price_;
  l1_data_.trade.size = _trade_print_info_.size_traded_;

  bulk_file_writer_.Write(&l1_data_, sizeof(GenericL1DataStruct));

  if (_market_update_info_.exch_source_ == ExchSource_t::kExchSourceNSE) {
    // REQUIRED : We are not getting explicit non-aggress side delete/change message in NSE.
    l1_data_.type = GenericL1DataType::L1_DELTA;
    l1_data_.delta.bestbid_price = _market_update_info_.bestbid_price_;
    l1_data_.delta.bestbid_size = _market_update_info_.bestbid_size_;
    l1_data_.delta.bestbid_ordercount = _market_update_info_.bestbid_ordercount_;
    l1_data_.delta.bestask_price = _market_update_info_.bestask_price_;
    l1_data_.delta.bestask_size = _market_update_info_.bestask_size_;
    l1_data_.delta.bestask_ordercount = _market_update_info_.bestask_ordercount_;

    bulk_file_writer_.Write(&l1_data_, sizeof(GenericL1DataStruct));
  }

  bulk_file_writer_.CheckToFlushBuffer();
}
}
