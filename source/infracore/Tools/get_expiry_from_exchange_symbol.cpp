// =====================================================================================
//
//       Filename:  get_expiry_from_exchange_symbol.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/14/2014 12:29:14 PM
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

#include "dvccode/Utils/load_our_defined_products.hpp"
#include <string>

HFSAT::ExchSource_t list_exchange[] = {
    HFSAT::kExchSourceCME,      HFSAT::kExchSourceEUREX,   HFSAT::kExchSourceBMF,     HFSAT::kExchSourceBMFEQ,
    HFSAT::kExchSourceNTP,      HFSAT::kExchSourceTMX,     HFSAT::kExchSourceMEFF,    HFSAT::kExchSourceIDEM,
    HFSAT::kExchSourceHONGKONG, HFSAT::kExchSourceREUTERS, HFSAT::kExchSourceICE,     HFSAT::kExchSourceEBS,
    HFSAT::kExchSourceLIFFE,    HFSAT::kExchSourceRTS,     HFSAT::kExchSourceMICEX,   HFSAT::kExchSourceMICEX_EQ,
    HFSAT::kExchSourceMICEX_CR, HFSAT::kExchSourceLSE,     HFSAT::kExchSourceBATSCHI, HFSAT::kExchSourceHYB,
    HFSAT::kExchSourceJPY,      HFSAT::kExchSourceJPY_L1,  HFSAT::kExchSourceTSE,     HFSAT::kExchSourceQUINCY,
    HFSAT::kExchSourceEOBI,     HFSAT::kExchSourcePUMA,    HFSAT::kExchSourceCFE,     HFSAT::kExchSourceASX,
    HFSAT::kExchSourceSGX,      HFSAT::kExchSourceNSE};

std::set<HFSAT::ExchSource_t> list_exchange_(list_exchange, list_exchange + 29);

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage : <exec> <exch_symbol>\n";
    exit(0);
  }

  HFSAT::Utils::LoadOurDefinedProducts::GetUniqueInstance(list_exchange_);

  std::string exch_symbol_ = argv[1];

  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  int sec_id_ = sec_name_indexer_.GetIdFromSecname(exch_symbol_.c_str());

  if (sec_id_ == -1) {
    std::cout << "Expired\n";
  } else {
    std::cout << "Not Expired\n";
  }

  return 0;
}
