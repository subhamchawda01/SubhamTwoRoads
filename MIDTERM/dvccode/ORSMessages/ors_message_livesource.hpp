/**
   \file dvccode/ORSMessages/ors_message_livesource.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_ORSMESSAGES_ORS_MESSAGE_LIVESOURCE_H
#define BASE_ORSMESSAGES_ORS_MESSAGE_LIVESOURCE_H
#define USE_SHM_LOOPBACK_LOGGING false

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"

#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"

#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/ORSMessages/ors_message_listener.hpp"

namespace HFSAT {

/** @brief class to listen to live UDP from OrderRoutingServer, when it is a separate Process
 */
class ORSMessageLiveSource : public SimpleExternalDataLiveListener {
 protected:
  DebugLogger &dbglogger_;
  SecurityNameIndexer &sec_name_indexer_;

  std::vector<std::vector<OrderNotFoundListener *> > order_not_found_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_None
  std::vector<std::vector<OrderSequencedListener *> > order_sequenced_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages
  /// of type kORRType_Seqd
  std::vector<std::vector<OrderConfirmedListener *> > order_confirmed_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages
  /// of type kORRType_Conf

  std::vector<std::vector<OrderConfCxlReplaceRejectListener *> > order_conf_cxlreplace_reject_listener_vec_map_;

  std::vector<std::vector<OrderConfCxlReplacedListener *> > order_conf_cxlreplaced_listener_vec_map_;  ///< map (vector)
  /// from
  /// security_id to
  /// vector of
  /// listeners to
  /// ORS messages
  /// of type
  /// kORRType_CxRe
  std::vector<std::vector<OrderCxlSeqdListener *> > order_cancel_sequenced_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Cxld
  std::vector<std::vector<OrderCanceledListener *> > order_canceled_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Cxld
  std::vector<std::vector<OrderExecutedListener *> > order_executed_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Exec
  std::vector<std::vector<OrderRejectedListener *> > order_rejected_listener_vec_map_;  ///< map (vector) from
  /// security_id to vector of
  /// listeners to ORS messages of
  /// type kORRType_Rejc
  std::vector<OrderRejectedDueToFundsListener *>
      order_rejected_due_to_funds_listener_vec_;  ///< vector of listeners to ORS messages of type kORRType_Rejc_Funds
  std::vector<std::vector<OrderInternallyMatchedListener *> >
      order_internally_matched_listener_vec_map_;  ///< map (vector) from security_id to vector of listeners to ORS
  /// messages of type kORRType_IntExec

  ExternalTimeListener *p_time_keeper_;  ///< only meant for Watch
  int shmid;
  volatile GenericORSReplyStructLive *ORSReply_queue;
  volatile int *qIndexPtr;
  struct shmid_ds shm_ds;

  int index_;
  int count;
  key_t key;
  std::string _this_exchange_;

  GenericORSReplyStructLive next_event_;

  MulticastReceiverSocket multicast_receiver_socket_;

  IPCMode ipc_mode_;

  int ors_struct_pkt_size_;

 public:
  /**
   * @param _dbglogger_ for logging errors
   * @param _sec_name_indexer_ to detect if the security is of interest and not to process if not
   * @param md_udp_ip the braodcast ip
   * @param md_udp_port the braodcast port
   */

  // default mode of operation is SHAREDMEM in basetrade
  ORSMessageLiveSource(DebugLogger &_dbglogger_, SecurityNameIndexer &_sec_name_indexer_, const std::string &md_udp_ip,
                       const int md_udp_port, IPCMode ipcm = MULTICAST, std::string exchange = "CME")
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        order_not_found_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderNotFoundListener *>()),
        order_sequenced_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderSequencedListener *>()),
        order_confirmed_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderConfirmedListener *>()),
        order_conf_cxlreplace_reject_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                       std::vector<OrderConfCxlReplaceRejectListener *>()),
        order_conf_cxlreplaced_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                 std::vector<OrderConfCxlReplacedListener *>()),
        order_cancel_sequenced_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                 std::vector<OrderCxlSeqdListener *>()),
        order_canceled_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderCanceledListener *>()),
        order_executed_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderExecutedListener *>()),
        order_rejected_listener_vec_map_(_sec_name_indexer_.NumSecurityId(), std::vector<OrderRejectedListener *>()),
        order_rejected_due_to_funds_listener_vec_(),
        order_internally_matched_listener_vec_map_(_sec_name_indexer_.NumSecurityId(),
                                                   std::vector<OrderInternallyMatchedListener *>()),
        p_time_keeper_(NULL),
        shmid(-1),
        ORSReply_queue(NULL),
        index_(-1),
        count(0),
        key(-1),
        _this_exchange_(exchange),
        next_event_(),
        multicast_receiver_socket_(md_udp_ip, md_udp_port,
                                   NetworkAccountInterfaceManager::instance().GetInterfaceForApp(k_ORS_LS)),
        ipc_mode_(ipcm),
        ors_struct_pkt_size_(sizeof(GenericORSReplyStructLive))

  {
    if (ipc_mode_ == MULTICAST) {
      multicast_receiver_socket_.SetNonBlocking();

    } else {
      std::ifstream ors_shm_key_config_file_;
      ors_shm_key_config_file_.open(ORS_SHM_KEY_CONFIG_FILE);

      if (!ors_shm_key_config_file_.is_open()) {
        std::cerr << "Fatal Error, Can't Read ORS SHM KEY FILE : " << ORS_SHM_KEY_CONFIG_FILE
                  << " Continuing with default value\n";

      } else {
        char line_buffer_[1024];
        std::string line_read_;

        while (ors_shm_key_config_file_.good()) {
          memset(line_buffer_, 0, sizeof(line_buffer_));

          ors_shm_key_config_file_.getline(line_buffer_, sizeof(line_buffer_));

          line_read_ = line_buffer_;

          if (line_read_.find("#") != std::string::npos) {
            continue;  // comments
          }

          HFSAT::PerishableStringTokenizer st_(line_buffer_, sizeof(line_buffer_));
          const std::vector<const char *> &tokens_ = st_.GetTokens();

          if (tokens_.size() > 0) {
            std::string exch = tokens_[0];

            if (exch != _this_exchange_)
              continue;
            else {
              key = atoi(tokens_[5]);
              break;
            }
          }

          ors_shm_key_config_file_.close();
        }
      }

      if (key == -1) key = SHM_KEY_ORS_REPLY;

      std::cerr << "Key Taken : " << key << "\n";

      if ((shmid = shmget(key, (size_t)(ORS_REPLY_SHM_QUEUE_SIZE * (sizeof(GenericORSReplyStructLive)) + sizeof(int)),
                          IPC_CREAT | 0666)) < 0) {
        std::cout << "Size of segment = " << ORS_REPLY_SHM_QUEUE_SIZE * sizeof(GenericORSReplyStructLive)
                  << " bytes key = " << key << std::endl;

        printf("Failed to shmget error = %s\n", strerror(errno));
        if (errno == EINVAL)
          printf("Invalid segment size specified\n");
        else if (errno == EEXIST)
          printf("Segment exists, cannot create it\n");
        else if (errno == EIDRM)
          printf("Segment is marked for deletion or was removed\n");
        else if (errno == ENOENT)
          printf("Segment does not exist\n");
        else if (errno == EACCES)
          printf("Permission denied\n");
        else if (errno == ENOMEM)
          printf("Not enough memory to create segment\n");

        exit(1);
      }

      if ((ORSReply_queue = (volatile GenericORSReplyStructLive *)shmat(shmid, NULL, 0)) ==
          (volatile GenericORSReplyStructLive *)-1) {
        perror("shmat failed");
        exit(0);
      }

      if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
        perror("shmctl");
        exit(1);
      }

      if (shm_ds.shm_nattch == 1) {
        memset((void *)ORSReply_queue, 0,
               (ORS_REPLY_SHM_QUEUE_SIZE * (sizeof(GenericORSReplyStructLive)) + sizeof(int)));
      }
      qIndexPtr = (volatile int *)(ORSReply_queue + ORS_REPLY_SHM_QUEUE_SIZE);
      index_ = *qIndexPtr;
    }
  }

  ~ORSMessageLiveSource() {
    if (ipc_mode_ == SHAREDMEM) {
      shmdt((void *)ORSReply_queue);

      if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
        perror("shmctl");
        exit(1);
      }

      if (shm_ds.shm_nattch == 0) shmctl(shmid, IPC_RMID, 0);  // remove shm segment if no users are attached
    }

    exit(0);
  }

  inline void AddOrderNotFoundListener(const unsigned int _security_id_, OrderNotFoundListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_not_found_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderNotFoundListener(const unsigned int _security_id_, OrderNotFoundListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_not_found_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderSequencedListener(const unsigned int _security_id_, OrderSequencedListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemOrderSequencedListener(const unsigned int _security_id_, OrderSequencedListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderConfirmedListener(const unsigned int _security_id_, OrderConfirmedListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_confirmed_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderConfirmedListener(const unsigned int _security_id_, OrderConfirmedListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_confirmed_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderConfCxlReplacedListener(const unsigned int _security_id_,
                                              OrderConfCxlReplacedListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplaced_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderConfCxlReplacedListener(const unsigned int _security_id_,
                                                 OrderConfCxlReplacedListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_conf_cxlreplaced_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderConfCxlReplaceRejectListener(const unsigned int _security_id_,
                                                   OrderConfCxlReplaceRejectListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_conf_cxlreplace_reject_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderConfCxlReplaceRejectListener(const unsigned int _security_id_,
                                                      OrderConfCxlReplaceRejectListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_conf_cxlreplace_reject_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderCanceledListener(const unsigned int _security_id_, OrderCanceledListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_canceled_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderCanceledListener(const unsigned int _security_id_, OrderCanceledListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_canceled_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderCancelSequencedListener(const unsigned int _security_id_, OrderCxlSeqdListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_cancel_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderCancelSequencedListener(const unsigned int _security_id_,
                                                 OrderCxlSeqdListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_cancel_sequenced_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderExecutedListener(const unsigned int _security_id_, OrderExecutedListener *_new_listener_) {
    if (_security_id_ >= order_executed_listener_vec_map_
                             .size()) {  // Due to sec-name-indexer changing sizes ( PBSAT ) , this check is needed.
      order_executed_listener_vec_map_.resize(_security_id_ + 1);
    }

    VectorUtils::UniqueVectorAdd(order_executed_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void AddOrderInternallyMatchedListener(const unsigned int _security_id_,
                                                OrderInternallyMatchedListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_internally_matched_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void RemoveOrderInternallyMatchedListener(const unsigned int _security_id_,
                                                   OrderInternallyMatchedListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_internally_matched_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void RemoveOrderExecutedListener(const unsigned int _security_id_, OrderExecutedListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_executed_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderRejectedListener(const unsigned int _security_id_, OrderRejectedListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_rejected_listener_vec_map_[_security_id_], _new_listener_);
  }
  inline void RemoveOrderRejectedListener(const unsigned int _security_id_, OrderRejectedListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_rejected_listener_vec_map_[_security_id_], _new_listener_);
  }

  inline void AddOrderRejectedDueToFundsListener(const unsigned int _security_id_,
                                                 OrderRejectedDueToFundsListener *_new_listener_) {
    VectorUtils::UniqueVectorAdd(order_rejected_due_to_funds_listener_vec_, _new_listener_);
  }
  inline void RemoveOrderRejectedDueToFundsListener(const unsigned int _security_id_,
                                                    OrderRejectedDueToFundsListener *_new_listener_) {
    VectorUtils::UniqueVectorRemove(order_rejected_due_to_funds_listener_vec_, _new_listener_);
  }
  inline void SetExternalTimeListener(ExternalTimeListener *_new_listener_) { p_time_keeper_ = _new_listener_; }

  inline int socket_file_descriptor() const { return multicast_receiver_socket_.socket_file_descriptor(); }

  void ProcessORSReply(GenericORSReplyStructLive *_this_event_) {
    // Logging
    if (USE_SHM_LOOPBACK_LOGGING) {
      switch (next_event_.orr_type_) {
        case HFSAT::kORRType_None: {
          DBGLOG_FUNC << "kORRType_None    SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_ << " count: " << count
                      << DBGLOG_ENDL_FLUSH;
        } break;
        case HFSAT::kORRType_Seqd: {
          DBGLOG_FUNC << "kORRType_Seqd    SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_ << " count: " << count
                      << DBGLOG_ENDL_FLUSH;
        }

        break;
        case HFSAT::kORRType_Conf: {
          DBGLOG_FUNC << "kORRType_Conf    SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_
                      << " client_position_ : " << next_event_.client_position_
                      << " global_position_ : " << next_event_.global_position_ << " count: " << count
                      << DBGLOG_ENDL_FLUSH;

          // exch_sym_saos_to_conf_map_ [ t_temp_oss_.str() ] = 1 ;
          //
        } break;
        case HFSAT::kORRType_ORSConf: {
          DBGLOG_FUNC << "kORRType_ORSConf SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_ << " count: " << count
                      << DBGLOG_ENDL_FLUSH;
        }

        break;
        case HFSAT::kORRType_CxlRejc: {
          DBGLOG_FUNC << "kORRType_CxlRejc SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_
                      << " rejR: " << next_event_.size_executed_ << " count: " << count << DBGLOG_ENDL_FLUSH;
        }

        break;
        case HFSAT::kORRType_Cxld: {
          DBGLOG_FUNC << "kORRType_Cxld    SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_
                      << " client_position_ : " << next_event_.client_position_
                      << " global_position_ : " << next_event_.global_position_ << " count: " << count
                      << DBGLOG_ENDL_FLUSH;
        }

        break;
        case HFSAT::kORRType_Exec: {
          DBGLOG_FUNC << "kORRType_Exec    SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_
                      << " executed: " << next_event_.size_executed_ << " remaining: " << next_event_.size_remaining_
                      << " client_position_ : " << next_event_.client_position_
                      << " global_position_ : " << next_event_.global_position_ << " count: " << count
                      << DBGLOG_ENDL_FLUSH;

        } break;
        case HFSAT::kORRType_IntExec: {
          DBGLOG_FUNC << "kORRType_IntExec    SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_
                      << " executed: " << next_event_.size_executed_ << " remaining: " << next_event_.size_remaining_
                      << " client_position_ : " << next_event_.client_position_
                      << " global_position_ : " << next_event_.global_position_ << " count: " << count
                      << DBGLOG_ENDL_FLUSH;
        } break;
        case HFSAT::kORRType_Rejc: {
          DBGLOG_FUNC << "kORRType_Rejc    SAOS : " << next_event_.server_assigned_order_sequence_
                      << " SYM:  " << next_event_.symbol_ << " CAOS: " << next_event_.client_assigned_order_sequence_
                      << " bs: " << (next_event_.buysell_ == HFSAT::kTradeTypeBuy ? "BUY" : "SELL")
                      << " px: " << next_event_.price_ << " intpx: " << next_event_.int_price_
                      << " executed: " << next_event_.size_executed_ << " RejReason: "
                      << HFSAT::ORSRejectionReasonStr((HFSAT::ORSRejectionReason_t)next_event_.size_executed_)
                      << " remaining: " << next_event_.size_remaining_ << " count: " << count << DBGLOG_ENDL_FLUSH;
        } break;

        default:
          break;
      }

      // DBGLOG_FUNC<<count<<" read\n";
      DBGLOG_DUMP;
    }

    // Process Message reply
    register const int security_id_ = sec_name_indexer_.GetIdFromChar16(_this_event_->symbol_);

    if (security_id_ >= 0) {
      p_time_keeper_->OnTimeReceived(_this_event_->time_set_by_server_);

      switch (_this_event_->orr_type_) {
        case kORRType_None: {
          std::vector<OrderNotFoundListener *> &order_not_found_listener_vec_ =
              order_not_found_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_not_found_listener_vec_.size(); i++) {
            order_not_found_listener_vec_[i]->OrderNotFound(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->buysell_,
                _this_event_->int_price_, _this_event_->server_assigned_message_sequence_,
                _this_event_->exch_assigned_sequence_, _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_Seqd: {
          std::vector<OrderSequencedListener *> &order_sequenced_listener_vec_ =
              order_sequenced_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_sequenced_listener_vec_.size(); i++) {
            order_sequenced_listener_vec_[i]->OrderSequenced(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->size_executed_,
                _this_event_->client_position_, _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_Conf: {
          std::vector<OrderConfirmedListener *> &order_confirmed_listener_vec_ =
              order_confirmed_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_confirmed_listener_vec_.size(); i++) {
            order_confirmed_listener_vec_[i]->OrderConfirmed(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->size_executed_,
                _this_event_->client_position_, _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_ORSConf: {
          std::vector<OrderConfirmedListener *> &order_confirmed_listener_vec_ =
              order_confirmed_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_confirmed_listener_vec_.size(); i++) {
            order_confirmed_listener_vec_[i]->OrderORSConfirmed(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->size_executed_,
                _this_event_->int_price_, _this_event_->server_assigned_message_sequence_,
                _this_event_->exch_assigned_sequence_, _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_Cxld: {
          std::vector<OrderCanceledListener *> &order_canceled_listener_vec_ =
              order_canceled_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_canceled_listener_vec_.size(); i++) {
            order_canceled_listener_vec_[i]->OrderCanceled(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->client_position_,
                _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_CxRe: {
          std::vector<OrderConfCxlReplacedListener *> &order_conf_cxlreplaced_listener_vec_ =
              order_conf_cxlreplaced_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_conf_cxlreplaced_listener_vec_.size(); i++) {
            order_conf_cxlreplaced_listener_vec_[i]->OrderConfCxlReplaced(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->size_executed_,
                _this_event_->client_position_, _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_Exec: {
          std::vector<OrderExecutedListener *> &order_executed_listener_vec_ =
              order_executed_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_executed_listener_vec_.size(); i++) {
            order_executed_listener_vec_[i]->OrderExecuted(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->size_executed_,
                _this_event_->client_position_, _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_IntExec: {
          std::vector<OrderInternallyMatchedListener *> &order_internally_matched_listener_vec_ =
              order_internally_matched_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_internally_matched_listener_vec_.size(); i++) {
            order_internally_matched_listener_vec_[i]->OrderInternallyMatched(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->size_executed_,
                _this_event_->client_position_, _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_CxlRejc: {
          std::vector<OrderCanceledListener *> &order_canceled_listener_vec_ =
              order_canceled_listener_vec_map_[security_id_];
          const int t_rejection_reason_ = _this_event_->size_executed_;
          for (size_t i = 0; i < order_canceled_listener_vec_.size(); i++) {
            order_canceled_listener_vec_[i]->OrderCancelRejected(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, t_rejection_reason_,
                _this_event_->client_position_, _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->exch_assigned_sequence_, _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_Rejc: {
          const int t_rejection_reason_ = _this_event_->size_executed_;
          std::vector<OrderRejectedListener *> &order_rejected_listener_vec_ =
              order_rejected_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_rejected_listener_vec_.size(); i++) {
            order_rejected_listener_vec_[i]->OrderRejected(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_, security_id_,
                _this_event_->price_, _this_event_->buysell_, _this_event_->size_remaining_, t_rejection_reason_,
                _this_event_->int_price_, _this_event_->exch_assigned_sequence_, _this_event_->time_set_by_server_);
          }
        } break;

        case kORRType_CxReRejc: {
          std::vector<OrderConfCxlReplaceRejectListener *> &order_modify_reject_listener_vec =
              order_conf_cxlreplace_reject_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_modify_reject_listener_vec.size(); i++) {
            order_modify_reject_listener_vec[i]->OrderConfCxlReplaceRejected(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->client_position_,
                _this_event_->global_position_, _this_event_->int_price_, _this_event_->size_executed_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        case kORRType_Rejc_Funds: {
          const int t_rejection_reason_ = _this_event_->size_executed_;
          dbglogger_ << "Funds rejection received for security " << security_id_ << "\n";
          for (size_t i = 0; i < order_rejected_due_to_funds_listener_vec_.size(); i++) {
            order_rejected_due_to_funds_listener_vec_[i]->OrderRejectedDueToFunds(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_, security_id_,
                _this_event_->price_, _this_event_->buysell_, _this_event_->size_remaining_, t_rejection_reason_,
                _this_event_->int_price_, _this_event_->exch_assigned_sequence_, _this_event_->time_set_by_server_);
            dbglogger_ << "OrderRejectedDueToFunds called \n";
          }
          dbglogger_.DumpCurrentBuffer();
        } break;
        case kORRType_Wake_Funds: {
          for (size_t i = 0; i < order_rejected_due_to_funds_listener_vec_.size(); i++) {
            order_rejected_due_to_funds_listener_vec_[i]->WakeUpifRejectedDueToFunds();
          }
        } break;
        case kORRType_CxlSeqd: {
          std::vector<OrderCxlSeqdListener *> &order_cancel_sequenced_listener_vec_ =
              order_cancel_sequenced_listener_vec_map_[security_id_];
          for (size_t i = 0; i < order_cancel_sequenced_listener_vec_.size(); i++) {
            order_cancel_sequenced_listener_vec_[i]->OrderCxlSequenced(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_,
                _this_event_->server_assigned_order_sequence_, security_id_, _this_event_->price_,
                _this_event_->buysell_, _this_event_->size_remaining_, _this_event_->client_position_,
                _this_event_->global_position_, _this_event_->int_price_,
                _this_event_->server_assigned_message_sequence_, _this_event_->exch_assigned_sequence_,
                _this_event_->time_set_by_server_);
          }
        } break;
        default:
          fprintf(stderr, "Weird msgtype in ORSMessagesLiveSource::ProcessAllEvents %d \n", _this_event_->orr_type_);
          break;
      }
    } else {
      switch (_this_event_->orr_type_) {
        case kORRType_Rejc_Funds: {
          const int t_rejection_reason_ = _this_event_->size_executed_;
          dbglogger_ << "Funds rejection received for security " << security_id_ << "\n";
          for (size_t i = 0; i < order_rejected_due_to_funds_listener_vec_.size(); i++) {
            order_rejected_due_to_funds_listener_vec_[i]->OrderRejectedDueToFunds(
                _this_event_->server_assigned_client_id_, _this_event_->client_assigned_order_sequence_, security_id_,
                _this_event_->price_, _this_event_->buysell_, _this_event_->size_remaining_, t_rejection_reason_,
                _this_event_->int_price_, _this_event_->exch_assigned_sequence_, _this_event_->time_set_by_server_);
            dbglogger_ << "OrderRejectedDueToFunds called \n";
          }
          dbglogger_.DumpCurrentBuffer();
        } break;
        case kORRType_Wake_Funds: {
          for (size_t i = 0; i < order_rejected_due_to_funds_listener_vec_.size(); i++) {
            order_rejected_due_to_funds_listener_vec_[i]->WakeUpifRejectedDueToFunds();
          }
        } break;
        default:
          break;
      }
    }
  }

  inline void ProcessAllEvents(int this_socket_fd_) {
    if (MULTICAST == ipc_mode_) {
      while (true) {
        int num_bytes = multicast_receiver_socket_.ReadN(ors_struct_pkt_size_, (void *)(&next_event_));
        // only exit from loop
        if (num_bytes < ors_struct_pkt_size_) return;

        ProcessORSReply(&next_event_);
      }

    } else {
      while (true) {
        // only exit from loop
        if (index_ == *qIndexPtr) return;

        memcpy(&next_event_, (GenericORSReplyStructLive *)(ORSReply_queue + index_), sizeof(GenericORSReplyStructLive));
        index_ = (index_ + 1) & (ORS_REPLY_SHM_QUEUE_SIZE - 1);
        count++;

        ProcessORSReply(&next_event_);
      }
    }
  }

 private:
};
}
#endif
