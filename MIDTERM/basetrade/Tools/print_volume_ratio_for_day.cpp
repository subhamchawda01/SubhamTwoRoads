#include "dvctrade/Indicators/volume_ratio_calculator.hpp"
#include "dvctrade/Indicators/volume_ratio_calculator_port.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

class TestVolume : public HFSAT::VolumeRatioListener {
  HFSAT::SecurityMarketView* this_smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  int duration_;
  HFSAT::VolumeRatioCalculator& volume_ratio_calculator_;

 public:
  TestVolume(HFSAT::SecurityMarketView* _this_smv_, HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
             int _duration_)
      : this_smv_(_this_smv_),
        dbglogger_(_dbglogger_),
        watch_(_watch_),
        duration_(_duration_),
        volume_ratio_calculator_(
            *(HFSAT::VolumeRatioCalculator::GetUniqueInstance(dbglogger_, watch_, *this_smv_, duration_))) {
    volume_ratio_calculator_.AddVolumeRatioListener(1, this);
  }
  void OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_) {
    if (r_security_id_ == 1) std::cout << watch_.tv() << " " << r_new_volume_ratio_ << std::endl;
  }
};
class TestVolumePort : public HFSAT::VolumeRatioListener {
  std::string this_port_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  int duration_;
  HFSAT::VolumeRatioCalculatorPort& volume_ratio_calculator_port_;

 public:
  TestVolumePort(std::string _this_port_, HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, int _duration_)
      : this_port_(_this_port_),
        dbglogger_(_dbglogger_),
        watch_(_watch_),
        duration_(_duration_),
        volume_ratio_calculator_port_(
            *(HFSAT::VolumeRatioCalculatorPort::GetUniqueInstance(dbglogger_, watch_, this_port_, duration_))) {
    volume_ratio_calculator_port_.AddVolumeRatioListener(1, this);
  }
  void OnVolumeRatioUpdate(const unsigned int r_security_id_, const double& r_new_volume_ratio_) {
    if (r_security_id_ == 1) std::cout << watch_.tv() << " " << r_new_volume_ratio_ << std::endl;
  }
};

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: exec <shortcode/port> <SHORTCODE/PORT> <duration> <date> <NTP/NTP_ORD/BMF_EQ>\n";
    exit(1);
  }
  if (strcmp(argv[2], "SHORTCODE") != 0 && strcmp(argv[2], "PORT") != 0) {
    std::cerr << "Usage: exec <shortcode/port> <SHORTCODE/PORT> <duration> <date> <NTP/NTP_ORD/BMF_EQ>\n";
    exit(1);
  }
  std::string _this_shortcode_ = argv[1];
  std::string _this_port_ = argv[1];
  int _this_duration_ = atoi(argv[3]);
  int tradingdate_ = atoi(argv[4]);
  bool isNTP = true;
  bool isNTPORD = (argc >= 5 && strcmp(argv[argc - 1], "NTP_ORD") == 0);
  bool isBMFEq = (argc >= 5 && strcmp(argv[argc - 1], "BMF_EQ") == 0);
  if (isNTPORD || isBMFEq) isNTP = false;

  std::vector<std::string> shc_list;

  if (strcmp(argv[2], "SHORTCODE") == 0) {
    shc_list.push_back(_this_shortcode_);
  } else {
    HFSAT::PcaWeightsManager::SetUniqueInstance(tradingdate_);
    HFSAT::PcaWeightsManager& pca_weight_manager_ = HFSAT::PcaWeightsManager::GetUniqueInstance();
    pca_weight_manager_.GetPortfolioShortCodeVec(_this_port_, shc_list);
    if (shc_list.size() == 0) {
      std::cerr << "Shortcode vec for port " << _this_port_ << " is of size zero\n";
      exit(1);
    }
  }

  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonSMVSource* common_smv_source = new CommonSMVSource(shc_list, tradingdate_);

  // Set the book type for NTP
  common_smv_source->SetNTPBookType(isNTP, isNTPORD, isBMFEq);

  // Get the dbglogger and watch after creating the source
  HFSAT::Watch& watch_ = common_smv_source->getWatch();
  HFSAT::DebugLogger& dbglogger_ = common_smv_source->getLogger();

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  // Get the security id to smv ptr from common source
  HFSAT::SecurityMarketViewPtrVec sid_to_smv_ptr_map = common_smv_source->getSMVMap();

  if (strcmp(argv[2], "SHORTCODE") == 0)
    TestVolume test_volume_(sid_to_smv_ptr_map[0], dbglogger_, watch_, _this_duration_);
  else
    TestVolumePort test_volume_(_this_port_, dbglogger_, watch_, _this_duration_);

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();
}
