#include "dvccode/CDef/generic_l1_data_struct.hpp"

namespace HFSAT {

const std::string GenericL1DataTrade::ToString() {
  std::ostringstream temp_oss;

  switch (side) {
    case kTradeTypeBuy:
      temp_oss << "Side :\t\t\tB\n";
      break;
    case kTradeTypeSell:
      temp_oss << "Side :\t\t\tS\n";
      break;
    case kTradeTypeNoInfo:
      temp_oss << "Side :\t\t\tUnknown\n";
      break;
    default:
      temp_oss << "Side :\t\t\tInvalid\n";
      break;
  }

  temp_oss << "Last Trade Price :\t" << price << "\n";
  temp_oss << "Last Trade Size :\t" << size << "\n";
  return temp_oss.str();
}

const std::string GenericL1DataDelta::ToString() {
  std::ostringstream temp_oss;

  temp_oss << "Ask Order Count :\t" << bestask_ordercount << "\n";
  temp_oss << "Ask Size :\t\t" << bestask_size << "\n";
  temp_oss << "Ask Price :\t\t" << bestask_price << "\n\n";
  temp_oss << "Bid Price :\t\t" << bestbid_price << "\n";
  temp_oss << "Bid Size :\t\t" << bestbid_size << "\n";
  temp_oss << "Bid Order Count :\t" << bestbid_ordercount << "\n";

  return temp_oss.str();
}

const std::string GenericL1DataStruct::ToString() {
  std::ostringstream temp_oss;

  temp_oss << "\n=============== GENERIC L1 DATA ===================\n\n";

  temp_oss << "Security :\t\t" << symbol << "\n";
  temp_oss << "Time :\t\t\t" << time.tv_sec << "." << std::setw(6) << std::setfill('0') << time.tv_usec << "\n\n";

  switch (type) {
    case L1_DELTA:
      temp_oss << "Type :\t\t\tDELTA\n";
      temp_oss << delta.ToString();
      break;
    case L1_TRADE:
      temp_oss << "Type :\t\t\tTRADE\n";
      temp_oss << trade.ToString();
      break;
    default:
      temp_oss << "Type :\t\tINVALID\n";
      break;
  }

  temp_oss << "====================================================\n";
  return temp_oss.str();
}
}
