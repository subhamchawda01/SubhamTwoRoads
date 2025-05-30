#include <cmath>
#include <limits>

#include "dvccode/CDef/common_security_definition_structs.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/live_trade_price_manager.hpp"

namespace HFSAT {
namespace Utils {

#define MAX_TRADES_WITHOUT_SLEEP 100

LiveTradePriceManager::LiveTradePriceManager(HFSAT::ExchSource_t exchange,
                                             std::vector<LastTradePriceInfo> &sec_id_to_last_traded_price,
                                             DebugLogger &dbglogger)
    : generic_mds_shm_key_(GENERIC_MDS_SHM_KEY),
      generic_mds_shmid_(-1),
      generic_mds_shm_struct_(NULL),
      index_(-1),
      exchange_(exchange),
      simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),
      sec_id_to_last_traded_price_(sec_id_to_last_traded_price),
      sec_id_to_last_bid_ask_price_(DEF_MAX_SEC_ID),
      sec_id_to_allowed_price_range_(DEF_MAX_SEC_ID),
      allowed_num_of_ticks_(20),
      dbglogger_(dbglogger),
      num_trades_(0) {
  for (int i = 0; i < DEF_MAX_SEC_ID; i++) {
    sec_id_to_allowed_price_range_[i] = 0.0;
    sec_id_to_last_bid_ask_price_[i].first = std::numeric_limits<double>::max();
    sec_id_to_last_bid_ask_price_[i].second = std::numeric_limits<double>::max();
  }
  // SHM Segment Join Related Work
  if ((generic_mds_shmid_ =
           shmget(generic_mds_shm_key_,
                  (size_t)(GENERIC_MDS_QUEUE_SIZE * (sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) + sizeof(int)),
                  IPC_CREAT | 0666)) < 0) {
    if (errno == EINVAL)
      std::cerr << "Invalid segment size specified \n";

    else if (errno == EEXIST)
      std::cerr << "Segment already exists \n";

    else if (errno == EIDRM)
      std::cerr << " Segment is marked for deletion \n";

    else if (errno == ENOENT)
      std::cerr << " Segment Doesn't Exist \n";

    else if (errno == EACCES)
      std::cerr << " Permission Denied \n";

    else if (errno == ENOMEM)
      std::cerr << " Not Enough Memory To Create Shm Segment \n";

    exit(1);
  }

  if ((generic_mds_shm_struct_ = (volatile HFSAT::MDS_MSG::GenericMDSMessage *)shmat(generic_mds_shmid_, NULL, 0)) ==
      (volatile HFSAT::MDS_MSG::GenericMDSMessage *)-1) {
    perror("shmat failed");
    exit(0);
  }

  if (shmctl(generic_mds_shmid_, IPC_STAT, &generic_shmid_ds_) == -1) {
    perror("shmctl");
    exit(1);
  }

  if (generic_shmid_ds_.shm_nattch == 1) {
    memset((void *)((HFSAT::MDS_MSG::GenericMDSMessage *)generic_mds_shm_struct_), 0,
           (GENERIC_MDS_QUEUE_SIZE * sizeof(HFSAT::MDS_MSG::GenericMDSMessage) + sizeof(int)));
  }

  simple_security_symbol_indexer_.AddSSSIListener(this);
}

void LiveTradePriceManager::thread_main() {
  AffinInitCores();  // Make Sure The Thread is Assigned to init cores, since it's not a latency sensitive task, The
                     // Time is also set by the writer

  HFSAT::MDS_MSG::GenericMDSMessage cstr_;

  memset((void *)&cstr_, 0, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

  dbglogger_ << "Starting LiveTradePriceManager Thread\n";
  dbglogger_.DumpCurrentBuffer();

  while (true) {  // Sit Tight For Signal, Until Then keep polling shm segment for events

    // has to be volatile, waiting on shared memory segment queue position
    volatile int queue_position_ = *((int *)(generic_mds_shm_struct_ + GENERIC_MDS_QUEUE_SIZE));

    // events are available only if the queue position at source is higher by 1, circular queue, will lag behind by 1
    // packet
    if (index_ == -1) {
      index_ = queue_position_;
    }

    if (index_ == queue_position_) {
      continue;
    }

    index_ = (index_ + 1) & (GENERIC_MDS_QUEUE_SIZE - 1);

    // memcpy is only done as safegaurd from writer writing the same segment, At this point it's important to know
    // which struct this is, to modify timestamp, convert Low bandwidth to compatible forms etc,
    memcpy((void *)&cstr_, (void *)((HFSAT::MDS_MSG::GenericMDSMessage *)(generic_mds_shm_struct_ + index_)),
           sizeof(HFSAT::MDS_MSG::GenericMDSMessage));

    if (cstr_.mds_msg_exch_ == MDS_MSG::SGX) {
      const char *secname = cstr_.generic_data_.sgx_data_.getContract();
      if (secname == NULL || secname == nullptr) {
        continue;
      }
      int security_id = simple_security_symbol_indexer_.GetIdFromSecname(secname);
      if (security_id < 0 || security_id >= (int)sec_id_to_last_traded_price_.size()) {
        continue;
      }
      if (cstr_.generic_data_.sgx_data_.msg_ == SGX_MDS::SGX_PF_TRADE) {
        sec_id_to_last_traded_price_[security_id].last_trade_price = cstr_.generic_data_.sgx_data_.data_.trade_.price_;
        double allowed_price_range =
            std::max(std::abs(0.04 * sec_id_to_last_traded_price_[security_id].last_trade_price),
                     sec_id_to_allowed_price_range_[security_id]);
        sec_id_to_last_traded_price_[security_id].min_price =
            sec_id_to_last_traded_price_[security_id].last_trade_price - allowed_price_range;
        sec_id_to_last_traded_price_[security_id].max_price =
            sec_id_to_last_traded_price_[security_id].last_trade_price + allowed_price_range;
        sec_id_to_last_traded_price_[security_id].is_trade_received_ = true;

        num_trades_++;
      } else if (cstr_.generic_data_.sgx_data_.msg_ == SGX_MDS::SGX_PF_DELTA &&
                 !sec_id_to_last_traded_price_[security_id].is_trade_received_) {
        if (cstr_.generic_data_.sgx_data_.data_.delta_.side_ == 1) {
          if ((sec_id_to_last_bid_ask_price_[security_id].first == std::numeric_limits<double>::max()) ||
              (cstr_.generic_data_.sgx_data_.data_.delta_.price_ > sec_id_to_last_bid_ask_price_[security_id].first)) {
            sec_id_to_last_bid_ask_price_[security_id].first = cstr_.generic_data_.sgx_data_.data_.delta_.price_;
          }

        } else if (cstr_.generic_data_.sgx_data_.data_.delta_.side_ == 2) {
          if (sec_id_to_last_bid_ask_price_[security_id].second == std::numeric_limits<double>::max() ||
              (cstr_.generic_data_.sgx_data_.data_.delta_.price_ < sec_id_to_last_bid_ask_price_[security_id].second)) {
            sec_id_to_last_bid_ask_price_[security_id].second = cstr_.generic_data_.sgx_data_.data_.delta_.price_;
          }
        }

        if (sec_id_to_last_bid_ask_price_[security_id].first != std::numeric_limits<double>::max() &&
            sec_id_to_last_bid_ask_price_[security_id].second != std::numeric_limits<double>::max()) {
          sec_id_to_last_traded_price_[security_id].last_trade_price =
              (sec_id_to_last_bid_ask_price_[security_id].first + sec_id_to_last_bid_ask_price_[security_id].second) /
              2;
          /*std::cout << "DEBUG: " << cstr_.generic_data_.sgx_data_.getContract() << " "
                    << sec_id_to_last_bid_ask_price_[security_id].first << " "
                    << sec_id_to_last_bid_ask_price_[security_id].second << " "
                    << sec_id_to_last_traded_price_[security_id].last_trade_price << std::endl;*/
          double allowed_price_range =
              std::max(std::abs(0.04 * sec_id_to_last_traded_price_[security_id].last_trade_price),
                       sec_id_to_allowed_price_range_[security_id]);
          sec_id_to_last_traded_price_[security_id].min_price =
              sec_id_to_last_traded_price_[security_id].last_trade_price - allowed_price_range;
          sec_id_to_last_traded_price_[security_id].max_price =
              sec_id_to_last_traded_price_[security_id].last_trade_price + allowed_price_range;
        }
      }
    } else if (cstr_.mds_msg_exch_ == MDS_MSG::HKOMDPF) {
      const char *secname = cstr_.generic_data_.hkomd_pf_data_.getContract();
      if (secname == NULL || secname == nullptr) {
        continue;
      }
      int security_id = simple_security_symbol_indexer_.GetIdFromSecname(secname);

      if (security_id < 0 || security_id >= (int)sec_id_to_last_traded_price_.size()) {
        continue;
      }

      if (cstr_.generic_data_.hkomd_pf_data_.msg_ == HKOMD_MDS::HKOMD_PF_TRADE) {
        sec_id_to_last_traded_price_[security_id].last_trade_price =
            cstr_.generic_data_.hkomd_pf_data_.data_.trade_.price_;
        double allowed_price_range =
            std::max(std::abs(0.04 * sec_id_to_last_traded_price_[security_id].last_trade_price),
                     sec_id_to_allowed_price_range_[security_id]);
        sec_id_to_last_traded_price_[security_id].min_price =
            sec_id_to_last_traded_price_[security_id].last_trade_price - allowed_price_range;
        sec_id_to_last_traded_price_[security_id].max_price =
            sec_id_to_last_traded_price_[security_id].last_trade_price + allowed_price_range;
        sec_id_to_last_traded_price_[security_id].is_trade_received_ = true;
        /*std::cout << "Last Traded Price for " << cstr_.generic_data_.hkomd_pf_data_.data_.trade_.contract_ << " ("
                  << security_id << ") " << sec_id_to_last_traded_price_[security_id].min_price << " "
                  << sec_id_to_last_traded_price_[security_id].last_trade_price << " "
                  << sec_id_to_last_traded_price_[security_id].max_price << std::endl;*/
        num_trades_++;
      } else if (cstr_.generic_data_.hkomd_pf_data_.msg_ == HKOMD_MDS::HKOMD_PF_DELTA &&
                 !sec_id_to_last_traded_price_[security_id].is_trade_received_) {
        if (cstr_.generic_data_.hkomd_pf_data_.data_.delta_.side_ == 0) {
          if ((sec_id_to_last_bid_ask_price_[security_id].first == std::numeric_limits<double>::max()) ||
              (cstr_.generic_data_.hkomd_pf_data_.data_.delta_.price_ >
               sec_id_to_last_bid_ask_price_[security_id].first)) {
            sec_id_to_last_bid_ask_price_[security_id].first = cstr_.generic_data_.hkomd_pf_data_.data_.delta_.price_;
          }

        } else if (cstr_.generic_data_.hkomd_pf_data_.data_.delta_.side_ == 1) {
          if ((sec_id_to_last_bid_ask_price_[security_id].first == std::numeric_limits<double>::max()) ||
              (cstr_.generic_data_.hkomd_pf_data_.data_.delta_.price_ <
               sec_id_to_last_bid_ask_price_[security_id].first)) {
            sec_id_to_last_bid_ask_price_[security_id].second = cstr_.generic_data_.hkomd_pf_data_.data_.delta_.price_;
          }
        }

        if (sec_id_to_last_bid_ask_price_[security_id].first != std::numeric_limits<double>::max() &&
            sec_id_to_last_bid_ask_price_[security_id].second != std::numeric_limits<double>::max()) {
          sec_id_to_last_traded_price_[security_id].last_trade_price =
              (sec_id_to_last_bid_ask_price_[security_id].first + sec_id_to_last_bid_ask_price_[security_id].second) /
              2;
          /*std::cout << "DEBUG: " << cstr_.generic_data_.hkomd_pf_data_.getContract() << " "
                    << sec_id_to_last_bid_ask_price_[security_id].first << " "
                    << sec_id_to_last_bid_ask_price_[security_id].second << " "
                    << sec_id_to_last_traded_price_[security_id].last_trade_price << std::endl;*/
          double allowed_price_range =
              std::max(std::abs(0.04 * sec_id_to_last_traded_price_[security_id].last_trade_price),
                       sec_id_to_allowed_price_range_[security_id]);
          sec_id_to_last_traded_price_[security_id].min_price =
              sec_id_to_last_traded_price_[security_id].last_trade_price - allowed_price_range;
          sec_id_to_last_traded_price_[security_id].max_price =
              sec_id_to_last_traded_price_[security_id].last_trade_price + allowed_price_range;
        }
      }
    }
    if (num_trades_ == MAX_TRADES_WITHOUT_SLEEP) {
      num_trades_ = 0;
      usleep(4999);
    }
  }
}

void LiveTradePriceManager::OnAddString(unsigned int t_num_security_id_) {
  unsigned int sec_id = t_num_security_id_ - 1;
  std::string product(simple_security_symbol_indexer_.GetSecuritySymbolFromId(sec_id));

  HFSAT::ShortcodeContractSpecificationMap &contract_spec_map =
      SecurityDefinitions::GetUniqueInstance().contract_specification_map_;

  for (auto iter = contract_spec_map.begin(); iter != contract_spec_map.end(); iter++) {
    if ((iter->second).exch_source_ == exchange_) {
      const char *exch_symbol = ExchangeSymbolManager::GetExchSymbol(iter->first);
      if (product == exch_symbol) {
        sec_id_to_allowed_price_range_[sec_id] = iter->second.min_price_increment_ * allowed_num_of_ticks_;
        std::cout << "AllowedPriceRange for " << product << ": " << sec_id_to_allowed_price_range_[sec_id] << std::endl;
        return;
      }
    }
  }
}

void LiveTradePriceManager::ShowLastTradedPrices() {
  unsigned int num_security = simple_security_symbol_indexer_.NumSecurityId();

  for (auto i = 0u; i < num_security; i++) {
    dbglogger_ << "LastTradePrice " << simple_security_symbol_indexer_.GetSecuritySymbolFromId(i) << ": ";
    if (sec_id_to_last_traded_price_[i].last_trade_price == std::numeric_limits<double>::max()) {
      dbglogger_ << "Not Available\n";
    } else {
      dbglogger_ << sec_id_to_last_traded_price_[i].last_trade_price
                 << " MinLimit: " << sec_id_to_last_traded_price_[i].min_price
                 << " MaxLimit:" << sec_id_to_last_traded_price_[i].max_price
                 << " Traded: " << sec_id_to_last_traded_price_[i].is_trade_received_ << "\n";
    }
  }
  dbglogger_.DumpCurrentBuffer();
}

void LiveTradePriceManager::SetAllowedPriceTicks(int num_of_ticks) {
  for (int i = 0; i < DEF_MAX_SEC_ID; i++) {
    sec_id_to_allowed_price_range_[i] = (sec_id_to_allowed_price_range_[i] / allowed_num_of_ticks_) * num_of_ticks;
  }
  allowed_num_of_ticks_ = num_of_ticks;
}
}
}
