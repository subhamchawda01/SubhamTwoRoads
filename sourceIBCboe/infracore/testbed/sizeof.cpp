#include <iostream>
#include <stdint.h>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

int main() {
  LIFFE_MDS::LIFFECommonStruct t_cstr_;

  memset(&t_cstr_, 0, sizeof(t_cstr_));

  t_cstr_.data_.liffe_trds_.trd_px_ = 55.55;

  std::cout << t_cstr_.data_.liffe_trds_.trd_px_ << " " << t_cstr_.data_.liffe_dels_.price_ << std::endl;

  memset(&t_cstr_, 0, sizeof(t_cstr_));

  t_cstr_.data_.liffe_dels_.price_ = 99.99;

  std::cout << t_cstr_.data_.liffe_trds_.trd_px_ << " " << t_cstr_.data_.liffe_dels_.price_ << std::endl;

  return 0;
}
