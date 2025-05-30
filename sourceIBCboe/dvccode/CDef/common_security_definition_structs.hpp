/**
 *    \file dvccode/CDef/common_security_definition_structs.hpp
 *
 *    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 *     Address:
 *     Suite No 353, Evoma, #14, Bhattarhalli,
 *     Old Madras Road, Near Garden City College,
 *     KR Puram, Bangalore 560049, India
 *    +91 80 4190 3551
 **/

#pragma once
#include "dvccode/CDef/defines.hpp"
#include <map>

namespace HFSAT {
struct ContractSpecification {
  double min_price_increment_;
  double numbers_to_dollars_;
  ExchSource_t exch_source_;
  int min_order_size_;

  ContractSpecification()
      : min_price_increment_(1), numbers_to_dollars_(1), exch_source_(kExchSourceInvalid), min_order_size_(1) {}

  ContractSpecification(double _min_price_increment_, double _numbers_to_dollars_, ExchSource_t _exch_source_,
                        int t_min_ord_size_)
      : min_price_increment_(_min_price_increment_),
        numbers_to_dollars_(_numbers_to_dollars_),
        exch_source_(_exch_source_),
        min_order_size_(t_min_ord_size_) {}
};

typedef std::map<std::string, ContractSpecification> ShortcodeContractSpecificationMap;
typedef std::map<std::string, ContractSpecification>::const_iterator ShortcodeContractSpecificationMapCIter_t;
}
