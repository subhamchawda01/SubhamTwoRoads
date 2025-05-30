#ifndef _CASH_CORPORATE_ACTION_HPP_
#define _CASH_CORPORATE_ACTION_HPP_

#include <iostream>
#include <string>
#include <inttypes.h>
#include <unordered_map>
#include <stdlib.h>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

class CashCorporateAction {
 public:
  static CashCorporateAction &GetUniqueInstance();
  ~CashCorporateAction() {}

  double CheckCorporateAction(std::string date_, std::string product_);

 private:
  CashCorporateAction();
  std::unordered_map<std::string, double> productDate_adjustmentFactor;
};

}  // namespace HFSAT

#endif
