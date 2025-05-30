#include "basetrade/Tools/l1_data_generator.hpp"

namespace HFSAT {

L1DataGenerator::L1DataGenerator(int trading_date, const std::vector<std::string>& shortcodes) {
  common_smv_source_ = new CommonSMVSource(shortcodes, trading_date);
  common_smv_source_->SetNSEL1Mode(false);
  common_smv_source_->Initialize();
}

L1DataGenerator::~L1DataGenerator() {
  if (common_smv_source_ != nullptr) {
    delete common_smv_source_;
    common_smv_source_ = nullptr;
  }
}

void L1DataGenerator::Run() { common_smv_source_->Run(); }

void L1DataGenerator::AddListener(SecurityMarketViewChangeListener* listener) {
  if (common_smv_source_ != nullptr) {
    common_smv_source_->getSMV()->subscribe_L1_Only(listener);
  }
}

const Watch& L1DataGenerator::GetWatch() { return common_smv_source_->getWatch(); }
}
