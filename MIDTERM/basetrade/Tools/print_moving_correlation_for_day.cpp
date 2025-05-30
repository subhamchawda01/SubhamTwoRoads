#include "dvctrade/Indicators/moving_correlation_cutoff.hpp"
#include "baseinfra/Tools/common_smv_source.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 20901225

class TestMovingCorrelation : public HFSAT::IndicatorListener {
  HFSAT::SecurityMarketView* this_smv_;
  HFSAT::SecurityMarketView* indep_smv_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  int duration_;
  HFSAT::MovingCorrelationCutOff& ind_;

 public:
  TestMovingCorrelation(HFSAT::SecurityMarketView* _this_smv_, HFSAT::SecurityMarketView* _indep_smv_,
                        HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_, int _duration_, double _ind_thresh_,
                        int indicator_return_type)
      : this_smv_(_this_smv_),
        indep_smv_(_indep_smv_),
        dbglogger_(_dbglogger_),
        watch_(_watch_),
        duration_(_duration_),
        ind_(*(HFSAT::MovingCorrelationCutOff::GetUniqueInstance(dbglogger_, watch_, *this_smv_, *indep_smv_, duration_,
                                                                 HFSAT::kPriceTypeOfflineMixMMS, _ind_thresh_,
                                                                 indicator_return_type))) {
    ind_.add_unweighted_indicator_listener(1u, this);
  }

  ~TestMovingCorrelation() {}
  void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& _new_value_) {
    if (indicator_index_ == 1u && _new_value_ <= 1 && -1 <= _new_value_)
      std::cout << watch_.tv() << " " << _new_value_ << std::endl;
    return;
  }
  void OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_decrease_,
                         const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }
};

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << "Usage: exec <shortcode/port> <SHORTCODE/PORT> <shortcode/port> <SHORTCODE/PORT> <duration> "
                 "<threshold>  <date> <NTP/NTP_ORD/BMF_EQ>\n";
    exit(1);
  }
  if (strcmp(argv[2], "SHORTCODE") != 0 && strcmp(argv[2], "PORT") != 0) {
    std::cerr << "Usage: exec <shortcode/port> <SHORTCODE/PORT> <shortcode/port> <SHORTCODE/PORT> <duration> "
                 "<threshold> <date> <NTP/NTP_ORD/BMF_EQ>\n";
    exit(1);
  }
  std::string _this_shortcode_ = argv[1];
  std::string _this_port_ = argv[1];
  std::string _this_dependant_ = argv[3];
  int _this_duration_ = atoi(argv[5]);
  double threshold = atof(argv[6]);
  int tradingdate_ = atoi(argv[7]);
  bool isNTP = true;
  bool isNTPORD = (argc >= 8 && strcmp(argv[argc - 1], "NTP_ORD") == 0);
  bool isBMFEq = (argc >= 8 && strcmp(argv[argc - 1], "BMF_EQ") == 0);
  if (isNTPORD || isBMFEq) isNTP = false;

  std::vector<std::string> shc_list = {_this_shortcode_, _this_dependant_};

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

  TestMovingCorrelation test_volume_(sid_to_smv_ptr_map[0], sid_to_smv_ptr_map[1], dbglogger_, watch_, _this_duration_,
                                     threshold, -1);

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();
}
