#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "infracore/DataDecoders/BSE/bse_pf_listener.hpp"
#include "infracore/DataDecoders/BSE/bse_order_book.hpp"
#include "dvccode/Utils/bse_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Forward Declaration
namespace HFSAT {
namespace BSEPFCONVERTER {
void MakeBSEOrderBookForPF(int trading_date, int instrument_code, std::string in_order_feed_dir,
                           std::string out_price_feed_dir);
}
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << " Usage < exec > <date> <instrument-code> < order_feed_dir > < price_feed_dir >\n";
    exit(-1);
  }

  if (!std::strcmp(argv[3], argv[4])) {
    std::cerr << "Orderfeed Directory and Pricefeed Directory should be different. Exiting.\n ";
    exit(1);
  }
  std::string date(argv[1]);
  if (date.size() != 8) {
    std::cerr << "Date should be in the format YYYYMMDD . Exiting.\n ";
    exit(1);
  }

  HFSAT::BSEPFCONVERTER::MakeBSEOrderBookForPF(std::atoi(date.c_str()), std::atoi(argv[2]), argv[3], argv[4]);

  return 0;
}

namespace HFSAT {
namespace BSEPFCONVERTER {

class PriceFeedHandler : public BSEPFListener {
 public:
  BulkFileWriter bulk_file_writer_;

  PriceFeedHandler(std::string file_name_) {
    bulk_file_writer_.Open(file_name_);

    if (!bulk_file_writer_.is_open()) {
      std::cerr << typeid(*this).name() << ':' << __func__ << " "
                << " Could not open PriceFeed Output File : " << file_name_ << "\n";
      exit(-1);
    }
  }

  void ProcessMarketUpdate(BSE_MDS::BSEPFCommonStruct* bse_mds_) {
    bulk_file_writer_.Write(bse_mds_, sizeof(BSE_MDS::BSEPFCommonStruct));
    bulk_file_writer_.CheckToFlushBuffer();
  }
};

void GetExchangeSymbolAndMinPriceIncrement(int trading_date, int instrument_code, std::string& exchange_symbol,
                                           double& min_px_increment) {
  HFSAT::SecurityDefinitions& security_definitions_ = HFSAT::SecurityDefinitions::GetUniqueInstance(trading_date);
  security_definitions_.LoadBSESecurityDefinitions();
  HFSAT::ShortcodeContractSpecificationMap& contract_spec_map = security_definitions_.contract_specification_map_;

  exchange_symbol = (HFSAT::BSESecurityDefinitions::GetUniqueInstance(trading_date)
                         .GetExchangeSymbolFromInstumentCode(instrument_code));  // /For testing purposes

  min_px_increment = contract_spec_map[HFSAT::BSESecurityDefinitions::GetUniqueInstance(trading_date)
                                           .GetShortcodeFromInstrumentCode(instrument_code)]
                         .min_price_increment_;
}

void MakeBSEOrderBookForPF(int trading_date, int instrument_code, std::string in_order_feed_dir,
                           std::string out_price_feed_dir) {
  if (HFSAT::BSESecurityDefinitions::GetUniqueInstance(trading_date).IsExpiryValidForInstrument(instrument_code)) {
    std::string filename = HFSAT::BSESecurityDefinitions::GetUniqueInstance(trading_date)
                               .GetDataSourceSymbolFromInstrumentCode(instrument_code) +
                           "_" + std::to_string(trading_date);
    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(BSE_UTILS::GetFilePath(in_order_feed_dir, filename));

    if (!bulk_file_reader_.is_open()) {
      std::cerr << __func__ << " "
                << " Could not open OrderLevelData File : " << BSE_UTILS::GetFilePath(in_order_feed_dir, filename)
                << "\n";
      exit(-1);
    }

    size_t MDS_SIZE_ = sizeof(BSE_MDS::BSEOrder);
    BSE_MDS::BSEOrder event, next_event;
    size_t available_len_ = bulk_file_reader_.read(&event, MDS_SIZE_);
    std::string date(event.date_);
    int intdate = std::atoi(date.substr(0, 4).c_str()) * 10000 + std::atoi(date.substr(5, 2).c_str()) * 100 +
                  std::atoi(date.substr(8, 2).c_str());

    // Sanity Checks
    if (instrument_code != event.instrument_code_) {
      std::cerr << __func__ << " "
                << "Supplied Instrument Code " << instrument_code << " does not match with Instrument Code "
                << event.instrument_code_ << " in data.\n";
      exit(1);
    }
    if (intdate != trading_date) {
      std::cerr << __func__ << " "
                << "Supplied Date " << trading_date << " does not match with Actual Date " << intdate << " in data.\n";
      exit(1);
    }
    std::string exch_symbol;
    double min_px_increment = 1;

    GetExchangeSymbolAndMinPriceIncrement(trading_date, instrument_code, exch_symbol, min_px_increment);
    HFSAT::BSEOrderBookForPF bse_order_book_(exch_symbol, min_px_increment, event.date_ /*, event.series*/);
    HFSAT::BSEPFCONVERTER::PriceFeedHandler price_feed_handler_(BSE_UTILS::GetFilePath(out_price_feed_dir, filename));
#if DEBUG
    std::cout << "Reading Orderfeed File : " << BSE_UTILS::GetFilePath(in_order_feed_dir, filename) << "\n";
    std::cout << "Writing to Pricefeed File : " << BSE_UTILS::GetFilePath(out_price_feed_dir, filename) << "\n";
    std::cout << "Exchange symbol : " << exch_symbol << "\n";
    std::cout << "Min Price Inc : " << min_px_increment << "\n";
#endif
    bse_order_book_.AddPriceFeedListener(&price_feed_handler_);

    while (true) {
      if (available_len_ < MDS_SIZE_) break;
      available_len_ = bulk_file_reader_.read(&next_event, MDS_SIZE_);

      bse_order_book_.ProcessOrder(event);
      event = next_event;
    }
#if DEBUG
    bse_order_book_.ShowOpenOrders();
#endif

    bulk_file_reader_.close();
  } else {
    std::cerr << __func__ << " "
              << "Invalid Expiry for Instrument : " << instrument_code << "\n";
  }
}
}
}
