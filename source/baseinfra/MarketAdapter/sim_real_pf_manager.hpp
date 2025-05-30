/**
   \file MarketAdapter/sim_real_pf_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once
#include <cstdlib>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

namespace HFSAT {

class SimRealPFManager : public PriceLevelGlobalListener,
                         public NTPPriceLevelGlobalListener,
                         public CFEPriceLevelGlobalListener,
                         public ControlMessageListener {
 public:
  virtual ~SimRealPFManager() {}

  SimRealPFManager(std::string file_name_);

  void OnPriceLevelNew(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_added_,
                       const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                       const bool t_is_intermediate_message_) {}

  void OnPriceLevelDelete(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_removed_,
                          const double t_price_, const bool t_is_intermediate_message_) {}

  void OnPriceLevelChange(const unsigned int t_security_id_, const TradeType_t t_buysell_, const int t_level_changed_,
                          const double t_price_, const int t_new_size_, const int t_new_ordercount_,
                          const bool t_is_intermediate_message_) {}

  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_min_level_deleted_, const bool t_is_intermediate_message_) {}

  void OnPriceLevelDeleteFrom(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const int t_max_level_removed_, const double t_price_,
                              const bool t_is_intermediate_message_) {}

  void OnPriceLevelDeleteThrough(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                                 const int t_max_level_deleted_, const bool t_is_intermediate_message_) {}

  void OnPriceLevelOverlay(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                           const int t_level_overlayed_, const double t_price_, const int t_new_size_,
                           const int t_new_ordercount_, const bool t_is_intermediate_message_) {}
  void OnPriceLevelDeleteThru(const unsigned int t_security_id_, const TradeType_t t_buysell_,
                              const bool t_is_intermediate_message_) {}

  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_) {}

  void OnMarketStatusUpdate(const unsigned int t_security_id_, const MktStatus_t t_this_mkt_status_) {}

  void OnOTCTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) {}

  void OnTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) {}

  virtual void OnSpreadTrade(const unsigned int t_security_id_, const double t_trade_price_, const int t_trade_size_) {}

  void OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                          HFSAT::MDS_MSG::MDSMessageExchType exch_type);
  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol_, const int trader_id) {}

  int GetMismatchCount() { return sim_mismatch_vec.size(); }

  timeval GetPacketTimeStamp(void* ptr_to_sim_real_data, HFSAT::MDS_MSG::MDSMessageExchType sim_exch_type);

 private:
  struct SimRealPacketinfo {
    void* ptr_;
    int size_of_struct_;
    HFSAT::MDS_MSG::MDSMessageExchType exch_type_;
  };
  void RemoveMatchedPackets();

  template <class T>
  void ReadAndCompareRealData(void* ptr_to_price_level_update, int length_of_bytes, BulkFileReader& bulk_file_reader_,
                              HFSAT::MDS_MSG::MDSMessageExchType real_exch_type,
                              HFSAT::MDS_MSG::MDSMessageExchType sim_exch_type) {
    size_t mds_available_len;
    T next_real_event;
    mds_available_len = bulk_file_reader_.read(&next_real_event, sizeof(T));
    if (mds_available_len < sizeof(T)) {
      std::cerr << "Unable to read the Real struct \n";
      exit(1);
    }

    timeval sim_packet_timestamp = GetPacketTimeStamp(ptr_to_price_level_update, sim_exch_type);
    timeval real_packet_timestamp = GetPacketTimeStamp(&next_real_event, real_exch_type);
    timeval default_timestamp{0, 0};

    // If a packet with new time stamp is found, previous packets with same time stamp will be removed from vector
    if ((packet_mismatch_time_.tv_sec != default_timestamp.tv_sec ||
         packet_mismatch_time_.tv_usec != default_timestamp.tv_usec) &&
        ((packet_mismatch_time_.tv_sec != sim_packet_timestamp.tv_sec ||
          packet_mismatch_time_.tv_usec != sim_packet_timestamp.tv_usec))) {
      // Sim & Real Packets with same time stamp will be compared & removed
      RemoveMatchedPackets();
      // All packets should be removed from the vector if the match is successful
      if (sim_mismatch_vec.size() > 0) {
        std::cerr << "Sim Real Packet Order mismatch detected \n";
        exit(1);
      }
      packet_mismatch_time_ = default_timestamp;
    }

    // There may be a case where there are consecutive packets in Sim & Real with same time stamp
    // In such cases, its not necessary that order in which packets are read in Sim is same as that of Real
    // To handle this, the packets are pushed in a mismatch vector until a different time stamp is encountered
    // Case 1:- Mismatch vector is empty , packet_mismatch_time_ is {0,0}
    // Case 2:- Mismatch vector is non-empty, packet_mismatch_time will have time stamp of the packet inserted in vector
    if (real_exch_type != sim_exch_type || memcmp(ptr_to_price_level_update, &next_real_event, length_of_bytes)) {
      if ((sim_packet_timestamp.tv_sec == real_packet_timestamp.tv_sec &&
           sim_packet_timestamp.tv_usec == real_packet_timestamp.tv_usec) &&
          (sim_packet_timestamp.tv_sec != default_timestamp.tv_sec ||
           sim_packet_timestamp.tv_usec != default_timestamp.tv_usec) &&
          ((packet_mismatch_time_.tv_sec == sim_packet_timestamp.tv_sec &&
            packet_mismatch_time_.tv_usec == sim_packet_timestamp.tv_usec) ||
           (packet_mismatch_time_.tv_sec == default_timestamp.tv_sec &&
            packet_mismatch_time_.tv_usec == default_timestamp.tv_usec))) {
        SimRealPacketinfo* sim_data_ptr = (SimRealPacketinfo*)malloc(sizeof(SimRealPacketinfo));
        SimRealPacketinfo* real_data_ptr = (SimRealPacketinfo*)malloc(sizeof(SimRealPacketinfo));

        sim_data_ptr->ptr_ = malloc(length_of_bytes);
        memcpy(sim_data_ptr->ptr_, ptr_to_price_level_update, length_of_bytes);
        sim_data_ptr->exch_type_ = sim_exch_type;
        sim_data_ptr->size_of_struct_ = length_of_bytes;
        sim_mismatch_vec.push_back(sim_data_ptr);

        real_data_ptr->ptr_ = malloc(sizeof(T));
        memcpy(real_data_ptr->ptr_, &next_real_event, sizeof(T));
        real_data_ptr->exch_type_ = real_exch_type;
        real_data_ptr->size_of_struct_ = sizeof(T);
        real_mismatch_vec.push_back(real_data_ptr);
        // packet_mismatch_time_ is by default {0,0}
        if (packet_mismatch_time_.tv_sec == default_timestamp.tv_sec &&
            packet_mismatch_time_.tv_usec == default_timestamp.tv_usec) {
          packet_mismatch_time_ = sim_packet_timestamp;
        }
      } else {
        std::cerr << "Sim Real Packet Order mismatch detected \n";
        exit(1);
      }
    }
  }

 private:
  BulkFileReader bulk_file_reader_;
  // Below two vectors will always have mismatched packets with same time stamp
  std::vector<SimRealPacketinfo*> sim_mismatch_vec;
  std::vector<SimRealPacketinfo*> real_mismatch_vec;
  // packet_mismatch_time_ indicates time at which mismatch b/w Sim & Real occurred but both packets had same time stamp
  // It will be set when Sim & Real packets have same time stamp but different contents
  // By default, packet_mismatch_time_ will be set to {0,0}
  timeval packet_mismatch_time_;
};
}
