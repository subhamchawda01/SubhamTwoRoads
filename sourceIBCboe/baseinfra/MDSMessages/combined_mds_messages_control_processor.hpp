// =====================================================================================
//
//       Filename:  combined_mds_messages_control_processor.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/03/2014 09:29:53 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"

#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvccode/CDef/mds_shm_interface_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"

namespace HFSAT {

class CombinedMDSMessagesControlProcessor {
 private:
  std::string this_hostname_;

 protected:
  DebugLogger& dbglogger_;

  std::map<std::string, std::vector<ControlMessageListener*> >
      string2_control_message_listener_vec_map_; /**< map (vector) from shortcode to vector of listeners to
                                                    ControlMessage */
  std::map<int, std::vector<ControlMessageListener*> > int2_control_message_listener_vec_map_;

  ExternalTimeListener* p_time_keeper_;

 public:
  /**
   * @param _dbglogger_ for logging errors
   * @param md_udp_ip the braodcast ip
   * @param md_udp_port the braodcast port
   */
  CombinedMDSMessagesControlProcessor(DebugLogger& _dbglogger_)
      : this_hostname_(""),
        dbglogger_(_dbglogger_),
        string2_control_message_listener_vec_map_(),
        int2_control_message_listener_vec_map_(),
        p_time_keeper_(NULL) {
    char hostname[64];
    hostname[63] = '\0';
    gethostname(hostname, 63);

    this_hostname_ = hostname;

    // Assume - we have all hostnames within 14 chars,
    // remove domains
    if (std::string::npos != this_hostname_.find("."))
      this_hostname_ = this_hostname_.substr(0, this_hostname_.find("."));
  }

  /// should be deprecated
  inline void AddControlMessageListener(const std::string& _shortcode_, ControlMessageListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(string2_control_message_listener_vec_map_[_shortcode_], _new_listener_);
  }
  inline void RemoveControlMessageListener(const std::string& _shortcode_, ControlMessageListener* _new_listener_) {
    VectorUtils::UniqueVectorRemove(string2_control_message_listener_vec_map_[_shortcode_], _new_listener_);
  }

  /// listener functions for ints
  inline void AddControlMessageListener(const int trader_id_, ControlMessageListener* _new_listener_) {
    if (int2_control_message_listener_vec_map_.find(trader_id_) == int2_control_message_listener_vec_map_.end()) {
      int2_control_message_listener_vec_map_[trader_id_] = std::vector<ControlMessageListener*>();
    }

    // std::cerr << " Listener Added For Control Source \n";
    VectorUtils::UniqueVectorAdd(int2_control_message_listener_vec_map_[trader_id_], _new_listener_);
  }

  inline void RemoveControlMessageListener(const int trader_id_, ControlMessageListener* _new_listener_) {
    if (int2_control_message_listener_vec_map_.find(trader_id_) == int2_control_message_listener_vec_map_.end()) {
      return;
    }
    VectorUtils::UniqueVectorRemove(int2_control_message_listener_vec_map_[trader_id_], _new_listener_);
  }

  inline void SetExternalTimeListener(ExternalTimeListener* _new_listener_) { p_time_keeper_ = _new_listener_; }

  inline void ProcessControlEvent(HFSAT::GenericControlRequestStruct* control_request_) {
    // We have received a getflat on host across all queries
    if (HOSTWIDE_GETFLAT_TRADER_ID == control_request_->trader_id_) {
      std::cout << " RECEIVED GETFLAT FOR HOST : " << HOSTWIDE_GETFLAT_TRADER_ID << this_hostname_ << std::endl;

      // Let's double check the message type
      // as well as host to ensure we are passing getflat to correct clients since we'll be
      // receiving control message across locations/servers
      if (HFSAT::kControlMessageCodeGetFlatOnThisHost == (control_request_->control_message_).message_code_ &&
          std::string(control_request_->symbol_) == this_hostname_) {
        p_time_keeper_->OnTimeReceived(control_request_->time_set_by_frontend_);

        // Now that we have verified the message is intended for us, let's getflat
        for (auto& itr : int2_control_message_listener_vec_map_) {
          // For each TraderId There can be multiple listeners
          for (auto& vec_itr : itr.second) {
            std::cout << "ASKING LISTENER TO GETFLAT : " << control_request_->symbol_ << " " << itr.first << std::endl;
            vec_itr->OnControlUpdate(control_request_->control_message_, control_request_->symbol_, itr.first);
          }
        }
      }

    } else if (control_request_->symbol_ &&
               string2_control_message_listener_vec_map_.find(control_request_->symbol_) !=
                   string2_control_message_listener_vec_map_.end()) {
      /// Semantics - control message should be associated with either a symbol or a trader id.
      p_time_keeper_->OnTimeReceived(control_request_->time_set_by_frontend_);

      std::vector<ControlMessageListener*>& listener_vec =
          string2_control_message_listener_vec_map_[control_request_->symbol_];
      for (size_t i = 0; i < listener_vec.size(); i++)
        listener_vec[i]->OnControlUpdate(control_request_->control_message_, control_request_->symbol_,
                                         control_request_->trader_id_);
    } else if (control_request_->trader_id_ > 0 &&
               int2_control_message_listener_vec_map_.find(control_request_->trader_id_) !=
                   int2_control_message_listener_vec_map_.end()) {
      /// Semantics - control message should be associated with either a symbol or a trader id.
      p_time_keeper_->OnTimeReceived(control_request_->time_set_by_frontend_);

      std::vector<ControlMessageListener*>& listener_vec =
          int2_control_message_listener_vec_map_[control_request_->trader_id_];
      for (size_t i = 0; i < listener_vec.size(); i++)
        listener_vec[i]->OnControlUpdate(control_request_->control_message_, control_request_->symbol_,
                                         control_request_->trader_id_);
    }
  }
};
}
