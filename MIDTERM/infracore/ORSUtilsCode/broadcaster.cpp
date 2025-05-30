/**
 \file ORSUtils/broadcaster.cpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066, India
 +91 80 4060 0717
 */

#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"
#include "infracore/ORSUtils/broadcaster.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

namespace HFSAT {

Broadcaster::Broadcaster(DebugLogger& t_dbglogger_, std::string bcast_ip, int bcast_port, int32_t base_writer_id)
    : dbglogger_(t_dbglogger_),
      base_writer_id_(base_writer_id +
                      1),  // since we are allocating client id starting with +1 and incrementing by 1 then onwards
      multicast_sender_socket_(bcast_ip, bcast_port,
                               NetworkAccountInterfaceManager::instance().GetInterfaceForApp(k_ORS)),
      position_manager_(HFSAT::ORS::PositionManager::GetUniqueInstance()),
      simple_security_symbol_indexer_(HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance()),
      order_manager_(HFSAT::ORS::OrderManager::GetUniqueInstance()),
      mds_shm_interface_(HFSAT::Utils::ORSReplyShmInterface::GetUniqueInstance()),
      use_shm_for_ors_reply_(true),
      mutex_(0),
      clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
      using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()) {
  queue_ = new boost::lockfree::queue<GenericORSReplyStructLive>(100);

  mds_shm_interface_.Initialize();

  // There can be at max 255 clients assigned writer id from ORS, the same range is used in the shm reader
  per_saci_recovery_pool_queue_ = new std::deque<GenericORSReplyStructLive>[ORS_MAX_NUM_OF_CLIENTS - 1]();

  // Reset memory - sequence numbers, The base of this array ( 0th element is the 1st SACI the ORS will allocate )
  // So the array is actually an SACI range value storage for it's respective sequence numbers
  memset((void*)per_saci_unique_message_sequence_number_, 0, (ORS_MAX_NUM_OF_CLIENTS - 1) * sizeof(int32_t));
  memset((void*)per_saci_sams_shm_, 0, (ORS_MAX_NUM_OF_CLIENTS) * sizeof(int32_t));
  ors_reply_generic_struct_.mds_msg_exch_ = HFSAT::MDS_MSG::ORS_REPLY;
}

Broadcaster::~Broadcaster() {}

void Broadcaster::thread_main() {
  // We want the replies to reach as fast as it can to the awaiting clients,
  // moving ahead this thread will evolve into something which will be doing some more tasks than just sending messages
  // to the clients
  setName("BroadcastManager");
  AllocateCPUOrExit();

  while (1) {
    while (!queue_->empty()) {
      GenericORSReplyStructLive reply_struct;
      queue_->pop(reply_struct);

      // Anything other than replay request, Just process normally
      if (0 == reply_struct.server_assigned_message_sequence_) {
        if (kORRType_CxlSeqd != reply_struct.orr_type_ && kORRType_Rejc != reply_struct.orr_type_ &&
            kORRType_CxlRejc != reply_struct.orr_type_ && kORRType_Rejc_Funds != reply_struct.orr_type_ &&
            kORRType_Wake_Funds != reply_struct.orr_type_ && kORRType_CxReRejc != reply_struct.orr_type_ &&
            kORRType_CxReSeqd != reply_struct.orr_type_) {
          // Start sequence with 1
          reply_struct.server_assigned_message_sequence_ =
              ++per_saci_unique_message_sequence_number_[reply_struct.server_assigned_client_id_ - base_writer_id_];

          // It's a new reply from ORS and also not a drop recovery packet, hence store it
          per_saci_recovery_pool_queue_[reply_struct.server_assigned_client_id_ - base_writer_id_].push_back(
              reply_struct);

          // Make sure we are not storing more than MAX_ORS_REPLY_DROP_RECOVERY_STORAGE otherwise this would become an
          // always growing in memory design
          if (MAX_ORS_REPLY_DROP_RECOVERY_STORAGE ==
              per_saci_recovery_pool_queue_[reply_struct.server_assigned_client_id_ - base_writer_id_].size()) {
            per_saci_recovery_pool_queue_[reply_struct.server_assigned_client_id_ - base_writer_id_].pop_front();
          }

        } else {
          // Don't want to store rejects including cancel as well as exchange ones
          reply_struct.server_assigned_message_sequence_ = 0;
        }

        multicast_sender_socket_.WriteN(sizeof(reply_struct), &reply_struct);

      } else {  // Some query has asked for recovery now

        DBGLOG_CLASS_FUNC_LINE_ERROR << " REBROADCAST REQUEST RECEIVED FOR : "
                                     << reply_struct.server_assigned_client_id_
                                     << " SEQ : " << reply_struct.server_assigned_message_sequence_
                                     << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

        // Well we do have the missing packet in the pocket, up for rebroadcast now
        if ((reply_struct.server_assigned_message_sequence_ >=
             (per_saci_recovery_pool_queue_[reply_struct.server_assigned_client_id_ - base_writer_id_].front())
                 .server_assigned_message_sequence_) &&
            (reply_struct.server_assigned_message_sequence_ <
             (per_saci_recovery_pool_queue_[reply_struct.server_assigned_client_id_ - base_writer_id_].back())
                 .server_assigned_message_sequence_)) {
          GenericORSReplyStructLive& recovery_ors_reply_packet =
              per_saci_recovery_pool_queue_[reply_struct.server_assigned_client_id_ - base_writer_id_]
                                           [reply_struct.server_assigned_message_sequence_ -
                                            (per_saci_recovery_pool_queue_[reply_struct.server_assigned_client_id_ -
                                                                           base_writer_id_].front())
                                                .server_assigned_message_sequence_];

          // Needs an order manager
          HFSAT::ORS::Order* this_order =
              order_manager_.GetOrderByOrderSequence(reply_struct.server_assigned_order_sequence_);

          if (NULL == this_order) {
            recovery_ors_reply_packet.size_remaining_ = 0;

          } else {
            recovery_ors_reply_packet.size_remaining_ = this_order->size_remaining_;
          }

          // So Clients Don't react to the positional change with cp / gp / prom
          recovery_ors_reply_packet.client_position_ =
              position_manager_.GetClientPosition(recovery_ors_reply_packet.server_assigned_client_id_);
          recovery_ors_reply_packet.global_position_ = position_manager_.GetGlobalPosition(
              simple_security_symbol_indexer_.GetIdFromSecname(recovery_ors_reply_packet.symbol_));

          multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &recovery_ors_reply_packet);

        } else {  // Can't recover, what was query doing all this time, if it's asking for stale packet

          char hostname[128];
          hostname[127] = '\0';
          gethostname(hostname, 127);

          std::ostringstream mail_body_str;
          mail_body_str << "RECOVERY REQUEST BY : " << reply_struct.server_assigned_client_id_
                        << " FOR PACKET : " << reply_struct.server_assigned_message_sequence_ << " CAN'T BE SERVED";

          std::string alert_message = mail_body_str.str();
          HFSAT::SendAlert::sendAlert(alert_message);

          HFSAT::Email e;

          e.setSubject(std::string(hostname) + " -- ORS Packet Drop/Recovery Request Failed");
          e.addRecepient("nseall@tworoads.co.in");
          e.addSender("nseall@tworoads.co.in");
          e.content_stream << mail_body_str.str() << "<br/>";
          e.sendMail();

          DBGLOG_CLASS_FUNC_LINE_ERROR << "CAN't RECOVER DROPPED PACKET FOR : "
                                       << reply_struct.server_assigned_client_id_
                                       << " SEQ : " << reply_struct.server_assigned_message_sequence_
                                       << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }
      }
    }
  }
}

void Broadcaster::Push(GenericORSReplyStructLive& t_ors_reply_) {
  if (use_shm_for_ors_reply_) {
    // Locks are required for serializing the per saci sequence numbers ( SAMS ). If we decide to ignore packet recovery
    // ( as shm shouldn't drop packets ), then we can remove the locks from here.
    HFSAT::SemUtils::spin_lock(&mutex_);
    memcpy((void*)&ors_reply_generic_struct_.generic_data_.ors_reply_data_, (void*)&t_ors_reply_,
           sizeof(GenericORSReplyStructLive));

    if (kORRType_CxlSeqd != t_ors_reply_.orr_type_ && kORRType_Rejc != t_ors_reply_.orr_type_ &&
        kORRType_CxlRejc != t_ors_reply_.orr_type_ && kORRType_Rejc_Funds != t_ors_reply_.orr_type_ &&
        kORRType_Wake_Funds != t_ors_reply_.orr_type_ && kORRType_CxReRejc != t_ors_reply_.orr_type_ &&
        kORRType_CxReSeqd != t_ors_reply_.orr_type_) {
      // Start sequence with 1
      ors_reply_generic_struct_.generic_data_.ors_reply_data_.server_assigned_message_sequence_ =
          ++per_saci_sams_shm_[t_ors_reply_.server_assigned_client_id_ - base_writer_id_];
    }

    // Timestamping
    if (true == using_simulated_clocksource_) {
      ors_reply_generic_struct_.time_ = clock_source_.GetTimeOfDay();
    } else {
      gettimeofday(&(ors_reply_generic_struct_.time_), NULL);
    }
    ors_reply_generic_struct_.generic_data_.ors_reply_data_.time_set_by_server_.val =
        HFSAT::GetCpucycleCountForTimeTick();
    t_ors_reply_.time_set_by_server_.val =
        ors_reply_generic_struct_.generic_data_.ors_reply_data_.time_set_by_server_.val;
    std::cout << "ORS MSSG REPLY: " << ors_reply_generic_struct_.ToString() << std::endl;
    mds_shm_interface_.WriteGenericStruct(&ors_reply_generic_struct_);
    HFSAT::SemUtils::spin_unlock(&mutex_);
  }

  queue_->push(t_ors_reply_);
}
}
