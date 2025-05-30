#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "infracore/DataDecoders/SGX/sgx_pf_listener.hpp"
#include "infracore/DataDecoders/SGX/sgx_itch_order_book.hpp"
#include "infracore/DataDecoders/SGX/sgx_order_book.hpp"
#include "infracore/SGXOuch/sgx_common_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include <cstdlib>
#include <string>
#include <vector>

// Forward Declaration
namespace HFSAT {
namespace SGXPFCONVERTER {
void MakeSGXOrderBookForPF(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                           std::string outfile_price_feed);
void MakeSGXItchOrderBookForPFNew(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                                  std::string outfile_price_feed);
}
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << " Usage < exec > < order_lvl_data > < price-feed file > <date>\n";
    exit(-1);
  }
  // Shortcode isn't required now. But would be required in future. So Adding a dummy for it.
  char dummy[3];
  dummy[2] = '\0';
  HFSAT::SGXPFCONVERTER::MakeSGXItchOrderBookForPFNew(argv[3], dummy, argv[1], argv[2]);

  return 0;
}

namespace HFSAT {
namespace SGXPFCONVERTER {

class PriceFeedHandler : public SGXPFListener {
 public:
  BulkFileWriter bulk_file_writer_;

  PriceFeedHandler(std::string file_name_) {
    bulk_file_writer_.Open(file_name_);

    if (!bulk_file_writer_.is_open()) {
      std::cerr << " Could not open PriceFeed Output File : " << file_name_ << "\n";
      exit(-1);
    }
  }

  void ProcessMarketUpdate(SGX_MDS::SGXPFCommonStruct* sgx_mds_) {
    bulk_file_writer_.Write(sgx_mds_, sizeof(SGX_MDS::SGXPFCommonStruct));
    bulk_file_writer_.CheckToFlushBuffer();
  }
};

void MakeSGXOrderBookForPF(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                           std::string outfile_price_feed) {
  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(infile_order_feed);

  if (!bulk_file_reader_.is_open()) {
    std::cerr << " Could not open OrderLevelData File : " << infile_order_feed << "\n";
    exit(-1);
  }

  size_t MDS_SIZE_ = sizeof(SGX_MDS::SGXOrder);
  SGX_MDS::SGXOrder event, next_event;
  size_t available_len_ = bulk_file_reader_.read(&event, MDS_SIZE_);

  HFSAT::SecurityDefinitions& security_definitions_ =
      HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

  HFSAT::ShortcodeContractSpecificationMap contract_spec_map = security_definitions_.contract_specification_map_;

  std::string this_exch_symbol_(event.series);
  double min_px_increment_ = 1.0;
  min_px_increment_ = contract_spec_map["SGX_" + this_exch_symbol_.substr(0, 2) + "_0"].min_price_increment_;

  HFSAT::SGXOrderBookForPF sgx_order_book_(this_exch_symbol_, min_px_increment_, event.date, event.series);
  HFSAT::SGXPFCONVERTER::PriceFeedHandler price_feed_handler_(outfile_price_feed);
  sgx_order_book_.AddPriceFeedListener(&price_feed_handler_);

  while (true) {
    if (available_len_ < MDS_SIZE_) break;
    available_len_ = bulk_file_reader_.read(&next_event, MDS_SIZE_);

    sgx_order_book_.ProcessOrder(event);
    event = next_event;
  }

  bulk_file_reader_.close();
}

double GetMinPxInc(std::string exch_symbol_, std::string trading_date) {
  std::fstream file(HFSAT::SGX_COMMON_UTILS::GetCompleteRefFilePath(trading_date), std::ofstream::in);
  if (!file || !file.is_open()) {
    fprintf(stderr, "Could not open file %s in SGXEngine::ParseReferenceFile",
            HFSAT::SGX_COMMON_UTILS::GetCompleteRefFilePath(trading_date).c_str());
    exit(-1);
  }

  while (file.good()) {
    char line[1024];
    file.getline(line, sizeof(line));

    HFSAT::PerishableStringTokenizer st(line, 1024);
    const std::vector<const char*>& tokens = st.GetTokens();

    if (tokens.size() < 1) continue;

    if (std::string(tokens[0]).find("#") != std::string::npos) continue;

    if (tokens.size() >= 5) {
      std::string symbol = tokens[0];

      if (symbol == exch_symbol_) {
        int16_t num_decimals = atoi(tokens[2]);
        double min_px_inc = atoi(tokens[4]) / std::pow(10, num_decimals);
        return min_px_inc;
      }

    } else {
      std::cerr << "Malformatted line in SGX Parese Ref File NumTokens: " << tokens.size() << " Expected: 3\n";
    }
  }
  file.close();
  return 0;
}

void MakeSGXItchOrderBookForPFNew(std::string trading_date, std::string shortcode, std::string infile_order_feed,
                                  std::string outfile_price_feed) {
  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(infile_order_feed);

  if (!bulk_file_reader_.is_open()) {
    std::cerr << " Could not open OrderLevelData File : " << infile_order_feed << "\n";
    exit(-1);
  }

  size_t MDS_SIZE_ = sizeof(SGX_ITCH_MDS::SGXItchOrder);
  SGX_ITCH_MDS::SGXItchOrder event, next_event;
  size_t available_len_ = bulk_file_reader_.read(&event, MDS_SIZE_);

  std::string this_exch_symbol_(event.series);

  double min_px_increment_ = GetMinPxInc(this_exch_symbol_, trading_date);
  std::cout << "Min Px Inc : " << min_px_increment_ << "\n";

  HFSAT::SGXItchOrderBookForPF sgx_order_book_(this_exch_symbol_, min_px_increment_);
  HFSAT::SGXPFCONVERTER::PriceFeedHandler price_feed_handler_(outfile_price_feed);
  sgx_order_book_.AddPriceFeedListener(&price_feed_handler_);

  while (true) {
    if (available_len_ < MDS_SIZE_) break;
    available_len_ = bulk_file_reader_.read(&next_event, MDS_SIZE_);

    sgx_order_book_.ProcessOrder(event);
    event = next_event;
  }

  bulk_file_reader_.close();
}
}
}
