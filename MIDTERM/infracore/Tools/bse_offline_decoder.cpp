/**
 \file Tools/sgx_offline_decoder.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 354, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551

 */

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/CDef/bse_security_definition.hpp"
#include "infracore/DataDecoders/BSE/bse_decoder.hpp"
#include "dvccode/Utils/bse_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

namespace {

#define ISTIMENOTGREATER(a, b) \
  (((a).tv_sec < (b).tv_sec) || (((a).tv_sec == (b).tv_sec) && ((a).tv_usec <= (b).tv_usec)))
#define ISTIMEGREATER(a, b) (!(ISTIMENOTGREATER(a, b)))

void GetLineFields(std::string& line, std::vector<std::string>& fields) {
  std::istringstream ss(line);
  fields.clear();

  std::string field;
  while (getline(ss, field, '|')) {
    fields.push_back(field);
  }
}

inline void FlushOrder(BSE_MDS::BSEOrder& order, int division_factor, HFSAT::BulkFileWriter& bulk_file_writer) {
  if (division_factor) {
    order.price_ /= division_factor;
  }

  bulk_file_writer.Write(&order, sizeof(BSE_MDS::BSEOrder));
  bulk_file_writer.CheckToFlushBuffer();
}

void FlushOrders(std::vector<BSE_MDS::BSEOrder>& add_orders, std::vector<BSE_MDS::BSEOrder>& modify_orders,
                 std::vector<BSE_MDS::BSEOrder>& trade_orders, std::vector<BSE_MDS::BSEOrder>& complex_trade_orders,
                 std::vector<BSE_MDS::BSEOrder>& system_record_orders, std::vector<BSE_MDS::BSEOrder>& delete_orders,
                 HFSAT::BulkFileWriter& bulk_file_writer, const std::string& orderfeed_dir, int& division_factor) {
  if (!bulk_file_writer.is_open()) {
    if (!add_orders.empty() || !modify_orders.empty() || !delete_orders.empty() || !trade_orders.empty()) {
      int instrumentcode = 0;
      std::string date;
      if (!add_orders.empty()) {
        instrumentcode = add_orders[0].instrument_code_;
        date = std::string(add_orders[0].date_);

      } else if (!modify_orders.empty()) {
        instrumentcode = modify_orders[0].instrument_code_;
        date = std::string(modify_orders[0].date_);

      } else if (!delete_orders.empty()) {
        instrumentcode = delete_orders[0].instrument_code_;
        date = std::string(delete_orders[0].date_);

      } else if (!trade_orders.empty()) {
        instrumentcode = trade_orders[0].instrument_code_;
        date = std::string(trade_orders[0].date_);
      } else if (!complex_trade_orders.empty()) {
        instrumentcode = complex_trade_orders[0].instrument_code_;
        date = std::string(complex_trade_orders[0].date_);
      } else if (!system_record_orders.empty()) {
        instrumentcode = system_record_orders[0].instrument_code_;
        date = std::string(system_record_orders[0].date_);
      }
      int intdate = std::atoi(date.substr(0, 4).c_str()) * 10000 + std::atoi(date.substr(5, 2).c_str()) * 100 +
                    std::atoi(date.substr(8, 2).c_str());
#if DEBUG
      std::cout << "Instrument Code :" << instrumentcode << "\n"
                << "Int Date :" << intdate << "\n";
#endif
      std::string orderfeed_filename = HFSAT::BSESecurityDefinitions::GetUniqueInstance(intdate)
                                           .GetDataSourceSymbolFromInstrumentCode(instrumentcode) +
                                       "_" + std::to_string(intdate);
      const std::string orderfeed_filepath = BSE_UTILS::GetFilePath(orderfeed_dir, orderfeed_filename);
#if DEBUG
      std::cout << "Orderfeed File Path : " << orderfeed_filepath << "\n";
#endif
      division_factor =
          HFSAT::BSESecurityDefinitions::GetUniqueInstance(intdate).GetPrecisionFromInstrumentCode(instrumentcode);
      division_factor = std::pow(10, division_factor);
#if DEBUG
      std::cout << "Division Factor : " << division_factor << "\n";
#endif
      bulk_file_writer.Open(orderfeed_filepath);
      if (!bulk_file_writer.is_open()) {
        return;
      }
    } else {
      return;
    }
  }
  for (auto order : add_orders) {
    FlushOrder(order, division_factor, bulk_file_writer);
  }
  add_orders.clear();
  for (auto order : modify_orders) {
    FlushOrder(order, division_factor, bulk_file_writer);
  }
  modify_orders.clear();
  // As per doc, we should consider Complex Trade orders after Normal Trade Orders. But we are intentionally changing
  // this order and bse order book logic is written keeping this ordering in mind.
  for (auto order : complex_trade_orders) {
    FlushOrder(order, division_factor, bulk_file_writer);
  }
  complex_trade_orders.clear();
  for (auto order : trade_orders) {
    FlushOrder(order, division_factor, bulk_file_writer);
  }
  trade_orders.clear();
  for (auto order : system_record_orders) {
    FlushOrder(order, division_factor, bulk_file_writer);
  }
  system_record_orders.clear();
  for (auto order : delete_orders) {
    FlushOrder(order, division_factor, bulk_file_writer);
  }
  delete_orders.clear();
}
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <order-input-file> <orderfeed-output-dir>" << std::endl;
    exit(-1);
  }

  HFSAT::BSE::BSEDecoder decoder = HFSAT::BSE::BSEDecoder::GetUniqueInstance();
  HFSAT::BulkFileWriter bulk_file_writer_;

  std::ifstream infile(argv[1], std::ifstream::in);
  BSE_MDS::BSEOrder bse_data;
  std::vector<BSE_MDS::BSEOrder> add_orders, modify_orders, delete_orders, trade_orders, system_record_orders,
      complex_trade_orders;
  timeval time;
  time.tv_sec = 0;
  time.tv_usec = 0;
  int division_factor = 0;

  if (infile.is_open()) {
    std::vector<std::string> data;
    std::string line;
    while (infile.good()) {
      getline(infile, line);
      if (line.empty()) {
        break;
      }

      GetLineFields(line, data);
      decoder.Decode(data);
      bse_data = decoder.GetBSEData();

      if (ISTIMEGREATER(bse_data.time_, time)) {
        FlushOrders(add_orders, modify_orders, trade_orders, complex_trade_orders, system_record_orders, delete_orders,
                    bulk_file_writer_, argv[2], division_factor);
        time = bse_data.time_;
      }
      switch (static_cast<BSE_MDS::BSEOrderType>(bse_data.type_)) {
        case BSE_MDS::BSEOrderType::kAdd:
          add_orders.push_back(bse_data);
          break;
        case BSE_MDS::BSEOrderType::kModify:
          modify_orders.push_back(bse_data);
          break;
        case BSE_MDS::BSEOrderType::kDelete:
          delete_orders.push_back(bse_data);
          break;
        case BSE_MDS::BSEOrderType::kTrade:
          trade_orders.push_back(bse_data);
          break;
        case BSE_MDS::BSEOrderType::kSystem:
          system_record_orders.push_back(bse_data);
          break;
        case BSE_MDS::BSEOrderType::kComplexInstTrade:
          complex_trade_orders.push_back(bse_data);
          break;
      }
    }
    FlushOrders(add_orders, modify_orders, trade_orders, complex_trade_orders, system_record_orders, delete_orders,
                bulk_file_writer_, argv[2], division_factor);
  }

  return 0;
}
