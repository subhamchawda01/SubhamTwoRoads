/**
   \file BasicOrderRoutingServer/bcaster.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#ifndef BASE_BASICORDERROUTINGSERVER_BCASTER_H
#define BASE_BASICORDERROUTINGSERVER_BCASTER_H

#include <string.h>  // for memcpy

#include "dvccode/Utils/lock.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

#include "dvccode/CDef/ors_messages.hpp"

#include "infracore/BasicOrderRoutingServer/defines.hpp"
#include "infracore/BasicOrderRoutingServer/order.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

static HFSAT::DebugLogger _dbglogger__(10240, 1);

namespace HFSAT {
namespace ORS {

class BCaster {
 public:
  BCaster(DebugLogger& _dbglogger_, const std::string& bcast_ip_, const int& bcast_port_, std::string exchange_)
      : multicast_sender_socket_(bcast_ip_, bcast_port_,
                                 NetworkAccountInterfaceManager::instance().GetInterfaceForApp(k_ORS)),
        bcaster_lock_(),
#define STARTING_VALUE_SERVER_ASSIGNED_MESSAGE_SEQUENCE 1
        server_assigned_message_sequence_(STARTING_VALUE_SERVER_ASSIGNED_MESSAGE_SEQUENCE),
#undef STARTING_VALUE_SERVER_ASSIGNED_MESSAGE_SEQUENCE
        position_manager_(PositionManager::GetUniqueInstance()),
        dbglogger_(_dbglogger_),
        watch_(_dbglogger_, HFSAT::DateTime::GetCurrentIsoDateLocal()),
        this_exchange_(exchange_) {
    //	std::ifstream ors_shm_key_config_file_;
    //        ors_shm_key_config_file_.open ( ORS_SHM_KEY_CONFIG_FILE ) ;
    //        if ( ! ors_shm_key_config_file_.is_open () ) {
    //          std::cerr << "Fatal Error, Can't Read ORS SHM KEY FILE : " << ors_shm_key_config_file_ << " Continuing
    //          with default value\n";
    //        }
    //	else{
    //
    //          char line_buffer_[ 1024 ];
    //          std::string line_read_ ;
    //
    //          while ( ors_shm_key_config_file_.good() ) {
    //
    //            memset ( line_buffer_, 0, sizeof ( line_buffer_ ) ) ;
    //
    //            ors_shm_key_config_file_.getline ( line_buffer_, sizeof ( line_buffer_ ) ) ;
    //
    //            line_read_ = line_buffer_ ;
    //
    //            if ( line_read_.find ( "#" ) != std::string::npos ) {
    //
    //              continue ; //comments
    //
    //            }
    //
    //            HFSAT::PerishableStringTokenizer st_ ( line_buffer_, sizeof( line_buffer_ ) );
    //            const std::vector< const char * > & tokens_ = st_.GetTokens ( );
    //
    //            if ( tokens_.size () > 0 ) {
    //
    //              std::string exch = tokens_ [ 0 ] ;
    //
    //              if ( exch != this_exchange_ ) continue ;
    //
    //
    //            }
    //            ors_shm_key_config_file_.close () ;
    //
    //          }
    //        }

    //        if(key==-1)key=SHM_KEY_ORS_REPLY;
    //
    //	std::cout<<"Size of segment = "<<ORS_REPLY_SHM_QUEUE_SIZE*sizeof(GenericORSReplyStructLive)<<" bytes key =
    //"<<key<<
    //" SHM KEY : " << key<< "\n" ;
    //
    //	if ((shmid = shmget(key,(size_t) ( ORS_REPLY_SHM_QUEUE_SIZE*(sizeof(GenericORSReplyStructLive)) + sizeof ( int )
    //),
    // IPC_CREAT | 0666)) < 0) {
    //
    //
    //	  printf("Failed to shmget error = %s\n",strerror(errno));
    //	  if(errno == EINVAL)
    //	    printf("Invalid segment size specified\n");
    //	  else if(errno == EEXIST)
    //	    printf("Segment exists, cannot create it\n");
    //	  else if(errno == EIDRM)
    //	    printf("Segment is marked for deletion or was removed\n");
    //	  else if(errno == ENOENT)
    //	    printf("Segment does not exist\n");
    //	  else if(errno == EACCES)
    //	    printf("Permission denied\n");
    //	  else if(errno == ENOMEM)
    //	    printf("Not enough memory to create segment\n");
    //
    //	  exit(1);
    //
    //	}
    //
    //	if (( ors_shm_queue_ = (volatile GenericORSReplyStructLive *)shmat(shmid, NULL, 0)) == (volatile
    // GenericORSReplyStructLive *) -1) {
    //
    //	  perror("shmat");
    //	  exit(1);
    //
    //	}
    //
    //	if(shmctl(shmid, IPC_STAT, &shm_ds) == -1){
    //
    //	  perror("shmctl");
    //	  exit(1);
    //
    //	}
    //
    //	memset( (void*)ors_shm_queue_ , 0, ( ORS_REPLY_SHM_QUEUE_SIZE*( sizeof( GenericORSReplyStructLive ) ) + sizeof(
    // int
    //) ) ) ;
    //	ors_shm_queue_pointer_ = ors_shm_queue_ ;
    //
    //	shm_queue_pointer_ = (volatile int *)( ors_shm_queue_ + ORS_REPLY_SHM_QUEUE_SIZE ) ;
  }

  ~BCaster() {}

  inline void WriteEvent(GenericORSReplyStructLive* generic_ors_reply_struct_) {
    return;
    ////	/*while (__sync_add_and_fetch( &thread_lock_, 1 ) > 0 ){
    ////	  __sync_add_and_fetch(&thread_lock_,-1);
    ////	  }*/
    ////	while (!__sync_bool_compare_and_swap(&thread_lock_, 0, 1));
    ////
    ////	memcpy( (void *)ors_shm_queue_pointer_, (void *)generic_ors_reply_struct_, sizeof(
    /// GenericORSReplyStructLive
    ///)
    ///)
    ///;
    ////
    ////	last_write_seq_ = *shm_queue_pointer_;
    ////
    ////	last_write_seq_ = ( last_write_seq_ + 1 ) & (ORS_REPLY_SHM_QUEUE_SIZE -1) ;
    ////
    ////	count++;
    ////	*shm_queue_pointer_ = last_write_seq_ ;
    ////
    ////	if(last_write_seq_==0){
    ////
    ////	  //count=0;
    ////
    ////	  ors_shm_queue_pointer_ = ors_shm_queue_ ;
    ////
    ////	  //shm_queue_pointer_ = (volatile int*) (ors_shm_queue_ + ORS_REPLY_SHM_QUEUE_SIZE) ;
    ////
    ////	}
    ////
    ////	else{
    ////
    ////	  ors_shm_queue_pointer_ ++ ;
    ////
    ////	}
    //
    //	watch_.OnTimeReceived ( generic_ors_reply_struct_->time_set_by_server_ );
    //        if(USE_SHM_LOOPBACK_LOGGING){
    //	  switch ( generic_ors_reply_struct_->orr_type_ )
    //	    {
    //	    case HFSAT::kORRType_None:
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_None    SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //	      }
    //	      break;
    //	    case HFSAT::kORRType_Seqd:
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_Seqd    SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //	      }
    //
    //	      break;
    //	    case HFSAT::kORRType_Conf:
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_Conf    SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " client_position_ : "<< generic_ors_reply_struct_->client_position_
    //		  << " global_position_ : "<< generic_ors_reply_struct_->global_position_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //		std::ostringstream t_temp_oss_ ;
    //		t_temp_oss_ << generic_ors_reply_struct_->symbol_ << "-" <<
    // generic_ors_reply_struct_->server_assigned_order_sequence_ ;
    //
    //		//exch_sym_saos_to_conf_map_ [ t_temp_oss_.str() ] = 1 ;
    //
    //	      }
    //
    //
    //	      break;
    //	    case HFSAT::kORRType_ORSConf:
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_ORSConf SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //	      }
    //
    //	      break;
    //	    case HFSAT::kORRType_CxlRejc:
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_CxlRejc SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " rejR: " << generic_ors_reply_struct_->size_executed_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //
    //		/* std::ostringstream t_temp_oss_ ;
    //
    //		   t_temp_oss_ << generic_ors_reply_struct_->symbol_ << "-" <<
    // generic_ors_reply_struct_->server_assigned_order_sequence_ ;
    //
    //		   if ( exch_sym_saos_to_conf_map_.find ( t_temp_oss_.str() ) != exch_sym_saos_to_conf_map_.end() ) {
    //
    //		   if ( exch_sym_saos_to_cxl_reject_map_.find ( t_temp_oss_.str() ) !=
    // exch_sym_saos_to_cxl_reject_map_.end()
    //) {
    //
    //		   std::ostringstream alert_msg_str_ ;
    //
    //		   alert_msg_str_ << " Multiple CxlRejects Received For : " << generic_ors_reply_struct_->symbol_ << "
    // Possibly Dead Order : " << generic_ors_reply_struct_->server_assigned_order_sequence_ << " Total Cxl Rejects : "
    //<< exch_sym_saos_to_cxl_reject_map_ [ t_temp_oss_.str() ] ;
    //		   exch_sym_saos_to_cxl_reject_map_ [ t_temp_oss_.str() ] ++ ;
    //
    //		   }else {
    //
    //		   exch_sym_saos_to_cxl_reject_map_ [ t_temp_oss_.str() ] = 1 ;
    //
    //		   }
    //
    //		   }*/
    //
    //	      }
    //
    //	      break;
    //	    case HFSAT::kORRType_Cxld:
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_Cxld    SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " client_position_ : "<< generic_ors_reply_struct_->client_position_
    //		  << " global_position_ : "<< generic_ors_reply_struct_->global_position_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //	      }
    //
    //	      break;
    //	    case HFSAT::kORRType_Exec:
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_Exec    SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " executed: " << generic_ors_reply_struct_->size_executed_
    //		  << " remaining: " << generic_ors_reply_struct_->size_remaining_
    //		  << " client_position_ : "<< generic_ors_reply_struct_->client_position_
    //		  << " global_position_ : "<< generic_ors_reply_struct_->global_position_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //	      }
    //	      break;
    //	    case HFSAT::kORRType_Rejc :
    //	      {
    //		DBGLOG_TIME_CLASS_FUNC
    //		  << "kORRType_Rejc    SAOS : " << generic_ors_reply_struct_-> server_assigned_order_sequence_
    //		  << " SYM:  "<< generic_ors_reply_struct_->symbol_
    //		  << " CAOS: " << generic_ors_reply_struct_->client_assigned_order_sequence_
    //		  << " bs: " << (generic_ors_reply_struct_->buysell_==HFSAT::kTradeTypeBuy?"BUY":"SELL")
    //		  << " px: " << generic_ors_reply_struct_->price_
    //		  << " intpx: " << generic_ors_reply_struct_->int_price_
    //		  << " executed: " << generic_ors_reply_struct_->size_executed_
    //		  << " RejReason: " << HFSAT::ORSRejectionReasonStr ( (HFSAT::ORSRejectionReason_t )
    // generic_ors_reply_struct_->size_executed_)
    //		  << " remaining: " << generic_ors_reply_struct_->size_remaining_
    //		  << " count: " << count
    //		  << DBGLOG_ENDL_FLUSH ;
    //
    //	      }
    //	      break;
    //
    //	    default:
    //	      break;
    //	    }
    //	}
    //	//__sync_add_and_fetch( &thread_lock_, -1 );
    //	__sync_bool_compare_and_swap(&thread_lock_, 1, 0);
    //
  }

  /// @param _this_client_request_ most details of reply struct are got from this
  /// @param server_assigned_client_id_ needed to fill in replystruct
  inline void BroadcastOrderNotFoundNotification(const GenericORSRequestStruct& _this_client_request_,
                                                 const int server_assigned_client_id_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    //	generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay ( ); // could take some time, should be done
    // outside mutex, does not matter if not thread safe
    generic_ors_reply_struct_.time_set_by_server_ = _this_client_request_.client_request_time_;
    generic_ors_reply_struct_.client_request_time_ =
        _this_client_request_.client_request_time_;  // DO NOT USE THIS AS SOURCE TIME - @ravi

    memcpy(generic_ors_reply_struct_.symbol_, _this_client_request_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = 0;  // not used by BaseOrderManager in client
    generic_ors_reply_struct_.orr_type_ = kORRType_None;
    generic_ors_reply_struct_.server_assigned_client_id_ = server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = 0;  // not used by BaseOrderManager in client
    generic_ors_reply_struct_.buysell_ = _this_client_request_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = _this_client_request_.server_assigned_order_sequence_;
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_client_request_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = 0;    // not used by BaseOrderManager in client
    generic_ors_reply_struct_.client_position_ = 0;  // not used by BaseOrderManager in client
    generic_ors_reply_struct_.global_position_ = 0;  // invalid not used by BaseOrderManager in client
    generic_ors_reply_struct_.int_price_ =
        _this_client_request_.int_price_;  // used by BaseOrderManager in client to track down order
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;

    //	bcaster_lock_.LockMutex ( );
    //	{
    //	generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //	}
    //	bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastORSRejection(const GenericORSRequestStruct& _this_client_request_,
                                    const int server_assigned_client_id_,
                                    const ORSRejectionReason_t rejection_reason_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;

    // THIS WILL REDUCE THE TIMEFRAME OF THE RACE WINDOW, JUST PULLED THE BOUNDARY, This guy is only a reader of the
    // below variables, hence wrap up copy at first
    // could take some time, should be done outside mutex, does not matter if not thread safe
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    generic_ors_reply_struct_.time_set_by_server_ = _this_client_request_.client_request_time_;  // GetTimeOfDay ( );
    generic_ors_reply_struct_.client_request_time_ = _this_client_request_.client_request_time_;

    // we need a lock because the struct generic_ors_reply_struct_ could be simultaneously operated upon by more than
    // one thread,
    // also the field server_assigned_message_sequence_ needs to be unique
    memcpy(generic_ors_reply_struct_.symbol_, _this_client_request_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_client_request_.price_;
    generic_ors_reply_struct_.orr_type_ = kORRType_Rejc;
    generic_ors_reply_struct_.server_assigned_client_id_ = server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = _this_client_request_.size_requested_;
    generic_ors_reply_struct_.buysell_ = _this_client_request_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = 0;  // invalid field
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_client_request_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = rejection_reason_;             // used to convey rejection_reason_
    generic_ors_reply_struct_.client_position_ = 0;                           // invalid field
    generic_ors_reply_struct_.global_position_ = 0;                           // invalid field
    generic_ors_reply_struct_.int_price_ = _this_client_request_.int_price_;  // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;

    //	bcaster_lock_.LockMutex ( );
    //	{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //	  }
    //	bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastExchRejection(const Order& _this_order_, const ORSRejectionReason_t rejection_reason_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay();
    generic_ors_reply_struct_.client_request_time_ =
        _this_order_.ors_timestamp_;  // This should now be the data source time, since client will provide the mds time

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;

    generic_ors_reply_struct_.orr_type_ = kORRType_Rejc;
    // skip server_assigned_message_sequence_ since it needs to be thread safe
    generic_ors_reply_struct_.server_assigned_client_id_ = _this_order_.server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = _this_order_.server_assigned_order_sequence_;
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_order_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = rejection_reason_;    // used to convey rejection_reason_
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ );
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //	}
    // bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastExchRejectionDueToFunds(const Order& _this_order_, const bool _should_get_flat_ = true) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay();
    generic_ors_reply_struct_.client_request_time_ =
        _this_order_.ors_timestamp_;  // This should now be the data source time, since client will provide the mds time
    dbglogger_ << "BroadcastExchRejectionDueToFunds called with " << _should_get_flat_ << "\n";

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;
    if (_should_get_flat_)
      generic_ors_reply_struct_.orr_type_ = kORRType_Rejc_Funds;
    else
      generic_ors_reply_struct_.orr_type_ = kORRType_Wake_Funds;
    // skip server_assigned_message_sequence_ since it needs to be thread safe
    generic_ors_reply_struct_.server_assigned_client_id_ = _this_order_.server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = _this_order_.server_assigned_order_sequence_;
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_order_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = -1;                   // used to convey rejection_reason_
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ );
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //	}
    // bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastCancelRejection(const Order& _this_order_,
                                       const CxlRejectReason_t exch_level_rejection_reason_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;

    // THIS WILL REDUCE THE TIMEFRAME OF THE RACE WINDOW, JUST PULLED THE BOUNDARY, This guy is only a reader of the
    // below variables, hence wrap up copy at first
    // This is more often called by the thread which is actually the main source of modification for the below
    // variables, client thread will call this only on Throttle/NotFoundCases
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay();
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;

    generic_ors_reply_struct_.orr_type_ = kORRType_CxlRejc;
    // skip server_assigned_message_sequence_ since it needs to be thread safe
    generic_ors_reply_struct_.server_assigned_client_id_ = _this_order_.server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = _this_order_.server_assigned_order_sequence_;
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_order_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = exch_level_rejection_reason_;  // used to convey rejection_reason_
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;           // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ );
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );
    generic_ors_reply_struct_.pad_ = 0;
    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

  // THIS IS AN EXCLUSIVE CALL FROM THE CLIENT THREAD
  inline void BroadcastSequenced(const Order& _this_order_, const GenericORSRequestStruct& _this_client_request_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;

    // THIS WILL REDUCE THE TIMEFRAME OF THE RACE WINDOW, JUST PULLED THE BOUNDARY, This guy is only a reader of the
    // below variables, hence wrap up copy at first
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    // could take some time, should be done outside mutex, does not matter if not thread safe
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.

    generic_ors_reply_struct_.time_set_by_server_ = _this_client_request_.client_request_time_;  // GetTimeOfDay ( );
    generic_ors_reply_struct_.client_request_time_ =
        _this_client_request_.client_request_time_;  // So in any case I need to copy somthing, do memset or

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);

    generic_ors_reply_struct_.price_ = _this_order_.price_;

    generic_ors_reply_struct_.orr_type_ = kORRType_Seqd;
    // skip server_assigned_message_sequence_ since it needs to be thread safe
    generic_ors_reply_struct_.server_assigned_client_id_ = _this_order_.server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = _this_order_.server_assigned_order_sequence_;
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_order_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = _this_order_.size_executed_;
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client

    // TODO_OPT : Locking here does make sure that the last packet that goes on to the wire
    // ( mcast udp top clients ) has the most correct server_assigned_order_sequence_
    // and position information.
    // which means that packets are always SENT with chronologically increasing sequence numbers and
    // most recent position information.
    // But since mcast-udp does not ensure that packets will be received in order or packets
    // will not be missed, I am not sure what good locking does here.
    // bcaster_lock_.LockMutex ( );
    //{
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    //	  generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    generic_ors_reply_struct_.pad_ = 0;

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

  // THIS IS AN EXCLUSIVE CALL FROM THE CLIENT THREAD
  inline void BroadcastCxlSequenced(const Order& _this_order_, const HFSAT::ttime_t& client_request_time_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;

    // THIS WILL REDUCE THE TIMEFRAME OF THE RACE WINDOW, JUST PULLED THE BOUNDARY, This guy is only a reader of the
    // below variables, hence wrap up copy at first
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    // could take some time, should be done outside mutex, does not matter if not thread safe
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.


    generic_ors_reply_struct_.time_set_by_server_ = client_request_time_;  // GetTimeOfDay ( );
    generic_ors_reply_struct_.client_request_time_ =
        _this_order_.ors_timestamp_;  // So in any case I need to copy somthing, do memset or

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);

    generic_ors_reply_struct_.price_ = _this_order_.price_;

    generic_ors_reply_struct_.orr_type_ = kORRType_CxlSeqd;  // CxlSequenced
    // skip server_assigned_message_sequence_ since it needs to be thread safe
    generic_ors_reply_struct_.server_assigned_client_id_ = _this_order_.server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = _this_order_.server_assigned_order_sequence_;
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_order_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = _this_order_.size_executed_;
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client

    // TODO_OPT : Locking here does make sure that the last packet that goes on to the wire
    // ( mcast udp top clients ) has the most correct server_assigned_order_sequence_
    // and position information.
    // which means that packets are always SENT with chronologically increasing sequence numbers and
    // most recent position information.
    // But since mcast-udp does not ensure that packets will be received in order or packets
    // will not be missed, I am not sure what good locking does here.
    // bcaster_lock_.LockMutex ( );
    //{
    //@ramkris: Atomic increment

    // Here Not Incrementing Sequence
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = __sync_add_and_fetch (
    // &server_assigned_message_sequence_, 1 );
    //        generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    generic_ors_reply_struct_.exch_assigned_sequence_ = _this_order_.exch_assigned_seq_;
    generic_ors_reply_struct_.pad_ = 0;

    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);

    //}
    //      multicast_sender_socket_.WriteN ( sizeof ( GenericORSReplyStructLive ), &generic_ors_reply_struct_ ) ;
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

  // THIS IS AN EXCLUSIVE CALL FROM THE CLIENT THREAD
  inline void BroadcastORSSequenced(const Order& _this_order_, const GenericORSRequestStruct& _this_client_request_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;

    // THIS WILL REDUCE THE TIMEFRAME OF THE RACE WINDOW, JUST PULLED THE BOUNDARY, This guy is only a reader of the
    // below variables, hence wrap up copy at first
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    // could take some time, should be done outside mutex, does not matter if not thread safe
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.


    //	generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay ( );
    generic_ors_reply_struct_.time_set_by_server_ = _this_client_request_.client_request_time_;
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);


    generic_ors_reply_struct_.price_ = _this_order_.price_;

    generic_ors_reply_struct_.orr_type_ = kORRType_Seqd;
    // skip server_assigned_message_sequence_ since it needs to be thread safe
    generic_ors_reply_struct_.server_assigned_client_id_ = _this_order_.server_assigned_client_id_;
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;
    generic_ors_reply_struct_.server_assigned_order_sequence_ = _this_order_.server_assigned_order_sequence_;
    generic_ors_reply_struct_.client_assigned_order_sequence_ = _this_order_.client_assigned_order_sequence_;
    generic_ors_reply_struct_.size_executed_ = _this_order_.size_executed_;
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client

    // TODO_OPT : Locking here does make sure that the last packet that goes on to the wire
    // ( mcast udp top clients ) has the most correct server_assigned_order_sequence_
    // and position information.
    // which means that packets are always SENT with chronologically increasing sequence numbers and
    // most recent position information.
    // But since mcast-udp does not ensure that packets will be received in order or packets
    // will not be missed, I am not sure what good locking does here.
    // bcaster_lock_.LockMutex ( );
    //{
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    //	  generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    generic_ors_reply_struct_.pad_ = 0;

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastConfirm(const Order& _this_order_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    generic_ors_reply_struct_.time_set_by_server_ =
        GetTimeOfDay();  // could take some time, should be done outside mutex, does not matter if not thread safe
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;
    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;
    generic_ors_reply_struct_.orr_type_ = kORRType_Conf;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_client_id_ =
        _this_order_.server_assigned_client_id_;                               // used by BaseOrderManager in client
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;                // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_order_sequence_ =
        _this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.client_assigned_order_sequence_ =
        _this_order_.client_assigned_order_sequence_;  // used by BaseOrderManager in client ( if missed kORRType_Seqd
                                                       // sent earlier )
    generic_ors_reply_struct_.size_executed_ = _this_order_.size_executed_;
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);
    generic_ors_reply_struct_.exch_assigned_sequence_ = _this_order_.exch_assigned_seq_;
    generic_ors_reply_struct_.pad_ = 0;

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ ); // used by BaseOrderManager in client
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );
    // // used by BaseOrderManager in client
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    //}
    //	bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastORSConfirm(const Order& _this_order_, const GenericORSRequestStruct& _this_client_request_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    //	generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay ( ); // could take some time, should be done
    // outside mutex, does not matter if not thread safe
    generic_ors_reply_struct_.time_set_by_server_ = _this_client_request_.client_request_time_;
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;
    generic_ors_reply_struct_.orr_type_ = kORRType_ORSConf;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_client_id_ =
        _this_order_.server_assigned_client_id_;                               // used by BaseOrderManager in client
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;                // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_order_sequence_ =
        _this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.client_assigned_order_sequence_ =
        _this_order_.client_assigned_order_sequence_;  // used by BaseOrderManager in client ( if missed kORRType_Seqd
                                                       // sent earlier )
    generic_ors_reply_struct_.size_executed_ = _this_order_.size_executed_;
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    generic_ors_reply_struct_.client_position_ = 0;  // not used in this message type
    generic_ors_reply_struct_.global_position_ = 0;  // not used in this message type

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ ); // used by BaseOrderManager in client
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );
    // // used by BaseOrderManager in client
    generic_ors_reply_struct_.pad_ = 0;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    //}
    //	bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastConfirmCxlReplace(const Order& _this_order_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    generic_ors_reply_struct_.time_set_by_server_ =
        GetTimeOfDay();  // could take some time, should be done outside mutex, does not matter if not thread safe
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;
    generic_ors_reply_struct_.orr_type_ = kORRType_CxRe;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_client_id_ =
        _this_order_.server_assigned_client_id_;                               // used by BaseOrderManager in client
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;                // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_order_sequence_ =
        _this_order_
            .server_assigned_order_sequence_;  // new SAOS .. the client has the SAOS of the old order that was canceled
    generic_ors_reply_struct_.client_assigned_order_sequence_ =
        _this_order_.client_assigned_order_sequence_;  // used by BaseOrderManager in client ( if missed kORRType_Seqd
                                                       // sent earlier )
    generic_ors_reply_struct_.size_executed_ = _this_order_.size_executed_;
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    //	bcaster_lock_.LockMutex ( );
    //	{
    //	  generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    //	  generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    //_this_order_.server_assigned_client_id_ ); // used by BaseOrderManager in client
    //	  generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_
    //); // used by BaseOrderManager in client
    generic_ors_reply_struct_.pad_ = 0;

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //	}
    //	bcaster_lock_.UnlockMutex ( );
  }

  // THIS IS CALLED FROM CLIENT THREAD ONLY WHEN THERE IS AN FOK ORDER PARTIAL FILL
  inline void BroadcastCancelNotification(const Order& _this_order_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay();
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.orr_type_ = kORRType_Cxld;     // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_client_id_ =
        _this_order_.server_assigned_client_id_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_order_sequence_ =
        _this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.client_assigned_order_sequence_ =
        _this_order_.client_assigned_order_sequence_;                // used by BaseOrderManager in client
    generic_ors_reply_struct_.size_executed_ = 0;                    // ignored in client
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    generic_ors_reply_struct_.exch_assigned_sequence_ = _this_order_.exch_assigned_seq_;
    generic_ors_reply_struct_.pad_ = 0;

    // Mainly called from engine thread hence delaying the write to race vaiables until write is due
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ ); // used by BaseOrderManager in client
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );
    // // used by BaseOrderManager in client
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

  inline void BroadcastExecNotification(const Order& _this_order_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay();
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.orr_type_ = kORRType_Exec;     // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_client_id_ =
        _this_order_.server_assigned_client_id_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;
    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_order_sequence_ =
        _this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.client_assigned_order_sequence_ =
        _this_order_.client_assigned_order_sequence_;                        // used by BaseOrderManager in client
    generic_ors_reply_struct_.size_executed_ = _this_order_.size_executed_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;          // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);
    generic_ors_reply_struct_.client_position_ =
        position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
    generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);
    generic_ors_reply_struct_.exch_assigned_sequence_ = _this_order_.exch_assigned_seq_;

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ ); // used by BaseOrderManager in client
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );
    // // used by BaseOrderManager in client
    generic_ors_reply_struct_.pad_ = 0;

    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

  // THIS IS AN EXCLUSIVE CALL FROM THE CLIENT THREAD
  inline void BroadcastORSExecNotification(const Order& _this_order_, int current_size_executed_,
                                           const GenericORSRequestStruct& _this_client_request_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    generic_ors_reply_struct_.time_set_by_server_ = _this_client_request_.client_request_time_;  // GetTimeOfDay ( );
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;  // used by BaseOrderManager in client
    //	generic_ors_reply_struct_.orr_type_ = kORRType_Exec; // used by BaseOrderManager in client
    generic_ors_reply_struct_.orr_type_ = kORRType_IntExec;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_client_id_ =
        _this_order_.server_assigned_client_id_;  // used by BaseOrderManager in client

    // Don't assume full execution at once for this notification
    generic_ors_reply_struct_.size_remaining_ = _this_order_.size_remaining_;

    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_order_sequence_ =
        _this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.client_assigned_order_sequence_ =
        _this_order_.client_assigned_order_sequence_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.exch_assigned_sequence_ = _this_order_.exch_assigned_seq_;

    // Don't assume full execution at once for this notification
    generic_ors_reply_struct_.size_executed_ = current_size_executed_;  // used by BaseOrderManager in client

    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);

    // FIXME : Adjust Position, Only Update For bcast not with the position manager

    generic_ors_reply_struct_.pad_ = 0;

    // BEING A WRITER OF THE CLIENT POSITION, DELAY THE WRITE TO THE RACE VARIABLE UNTIL THE WRITE IS DUE
    if (generic_ors_reply_struct_.buysell_ == HFSAT::kTradeTypeBuy) {
      // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
      // _this_order_.server_assigned_client_id_ ) + _this_order_.size_remaining_ ;
      // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ )
      // + _this_order_.size_remaining_ ;
      position_manager_.AddInternalBuyTrade(_this_order_.server_assigned_client_id_, current_size_executed_);
      generic_ors_reply_struct_.client_position_ =
          position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
      generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);

    } else {
      // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
      // _this_order_.server_assigned_client_id_ ) - _this_order_.size_remaining_ ;
      // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ )
      // - _this_order_.size_remaining_ ;
      position_manager_.AddInternalSellTrade(_this_order_.server_assigned_client_id_, current_size_executed_);
      generic_ors_reply_struct_.client_position_ =
          position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_);
      generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition(_this_order_.security_id_);
    }

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ ); // used by BaseOrderManager in client
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );
    // // used by BaseOrderManager in client

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

  // This Execution would cancel out the previous exec sent to client
  inline void BroadcastORSTradeBustNotification(const Order& _this_order_) {
    GenericORSReplyStructLive generic_ors_reply_struct_;
    // TODO_OPT : figure out ways of making this faster by avoiding calling GetTimeOfDay so many times
    // for instance suppose we were to keep a timemanager, a separate thread that is woken up every 100 microseconds and
    // calls GetTimeOfDay ()
    // and bcaster and other threads just read that value ?
    // it would reduce the number of calls made to GetTimeOfDay ( ) in a busy time and increase the frequency of
    // GetTimeOfDay () calls in a quiet time.
    generic_ors_reply_struct_.time_set_by_server_ = GetTimeOfDay();
    generic_ors_reply_struct_.client_request_time_ = _this_order_.ors_timestamp_;

    memcpy(generic_ors_reply_struct_.symbol_, _this_order_.symbol_, kSecNameLen);
    generic_ors_reply_struct_.price_ = _this_order_.price_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.orr_type_ = kORRType_Exec;     // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_client_id_ =
        _this_order_.server_assigned_client_id_;  // used by BaseOrderManager in client

    // Always assume full execution at once for this notification
    generic_ors_reply_struct_.size_remaining_ = 0;

    generic_ors_reply_struct_.buysell_ = _this_order_.buysell_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.server_assigned_order_sequence_ =
        _this_order_.server_assigned_order_sequence_;  // used by BaseOrderManager in client
    generic_ors_reply_struct_.client_assigned_order_sequence_ =
        _this_order_.client_assigned_order_sequence_;  // used by BaseOrderManager in client

    // Always assume full execution at once for this notification
    // negative size to cancel out previous exec notice
    generic_ors_reply_struct_.size_executed_ = -(_this_order_.size_executed_);  // used by BaseOrderManager in client

    generic_ors_reply_struct_.int_price_ = _this_order_.int_price_;  // used by BaseOrderManager in client
    //@ramkris: Atomic increment
    generic_ors_reply_struct_.server_assigned_message_sequence_ =
        __sync_add_and_fetch(&server_assigned_message_sequence_, 1);

    // FIXME : Adjust Position, Only Update For bcast not with the position manager

    if (generic_ors_reply_struct_.buysell_ == HFSAT::kTradeTypeBuy) {
      generic_ors_reply_struct_.client_position_ =
          position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_) - _this_order_.size_remaining_;
      generic_ors_reply_struct_.global_position_ =
          position_manager_.GetGlobalPosition(_this_order_.security_id_) - _this_order_.size_remaining_;

    } else {
      generic_ors_reply_struct_.client_position_ =
          position_manager_.GetClientPosition(_this_order_.server_assigned_client_id_) + _this_order_.size_remaining_;
      generic_ors_reply_struct_.global_position_ =
          position_manager_.GetGlobalPosition(_this_order_.security_id_) + _this_order_.size_remaining_;
    }

    //	bcaster_lock_.LockMutex ( );
    //{
    // generic_ors_reply_struct_.server_assigned_message_sequence_ = server_assigned_message_sequence_++;
    // generic_ors_reply_struct_.client_position_ = position_manager_.GetClientPosition (
    // _this_order_.server_assigned_client_id_ ); // used by BaseOrderManager in client
    // generic_ors_reply_struct_.global_position_ = position_manager_.GetGlobalPosition ( _this_order_.security_id_ );
    // // used by BaseOrderManager in client
    generic_ors_reply_struct_.pad_ = 0;

    //        WriteEvent ( & generic_ors_reply_struct_ ) ;
    multicast_sender_socket_.WriteN(sizeof(GenericORSReplyStructLive), &generic_ors_reply_struct_);
    //}
    // bcaster_lock_.UnlockMutex ( );
  }

 protected:
 private:
  HFSAT::MulticastSenderSocket multicast_sender_socket_;
  /// to make usage of generic_ors_reply_struct_, and server_assigned_message_sequence_ thread safe
  Lock bcaster_lock_;

  int server_assigned_message_sequence_;
  PositionManager& position_manager_;

  // to keep track of the reversal of the queue
  DebugLogger& dbglogger_;
  std::ostringstream t_temp_oss_;
  HFSAT::Watch watch_;
  std::string this_exchange_;
};
}
}
#endif  //  BASE_BASICORDERROUTINGSERVER_BCASTER_H
