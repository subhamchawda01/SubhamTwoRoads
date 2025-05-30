#include <unordered_map>
#include <cstring>
#include <math.h>
#include <time.h>
#include "baseinfra/FPGA/BMF_FPGA/mbochip/SumdfEventApi.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/Utils/shm_writer.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define BMF_FPGA_DEBUG_MODE 0

namespace BMF_FPGA {

struct BMFFpgaRefStruct {
  uint64_t secid;
  char secname[30];
  uint64_t group_id;
  uint32_t trading_status;
  uint32_t channel_id;
  bool is_following_group_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "SecID: " << secid << "\n";
    t_temp_oss_ << "Secname: " << secname << "\n";
    t_temp_oss_ << "GroupID: " << group_id << "\n";
    t_temp_oss_ << "TradingStatus: " << trading_status << "\n";
    t_temp_oss_ << "ChannelID:  " << channel_id << "\n";
    t_temp_oss_ << "IsFollowingGroup: " << is_following_group_ << "\n";
    return t_temp_oss_.str();
  }
};

class BMFFpgaDecoder {
 public:
  BMFFpgaDecoder();
  void Initialize(HFSAT::FastMdConsumerMode_t mode);
  void ProcessReferenceData(SiliconUmdf::EventAPI::Events_t& events_);
  void ProcessBook(SiliconUmdf::EventAPI::Events_t& events_);
  void ProcessInstrumentStatus(SiliconUmdf::EventAPI::Events_t& events_);
  void ProcessGroupStatus(SiliconUmdf::EventAPI::Events_t& events_);
  void ProcessMDEntries(SiliconUmdf::EventAPI::Events_t& events_);
  void PrintReferenceData();
  void AggregateTrades(SiliconUmdf::EventAPI::Events_t& events_, unsigned int index, uint64_t sec_id);
  void FlushFpgaData();
  void FlushAggregatedTrade(uint64_t sec_id);
  void SetOpenFlag(uint64_t sec_id);
  void SetTime();

 private:
  std::vector<double> powers;
  std::vector<double> neg_powers;
  std::unordered_map<uint64_t, BMFFpgaRefStruct*> secid_refdata_map_;
  FPGA_MDS::BMFFPGACommonStruct* fpga_cstr;
  MDSLogger<FPGA_MDS::BMFFPGACommonStruct> mdsLogger;
  HFSAT::FastMdConsumerMode_t mode_;
  SHM::ShmWriter<FPGA_MDS::BMFFPGACommonStruct>* shm_writer_;
  bool is_cached_trade_valid_;
  HFSAT::ClockSource& clock_source_;
  bool using_simulated_clocksource_;
};
}
