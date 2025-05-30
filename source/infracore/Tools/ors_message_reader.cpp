#include "infracore/Tools/ors_message_reader.hpp"

namespace HFSAT {

ORSMessageReader::ORSMessageReader(ORSMessagesListener* listener, const std::string filename, int security_id)
    : listener_(listener), security_id_(security_id), filename_(filename) {
  bulk_file_reader_.open(filename.c_str());
}

void ORSMessageReader::PlayAll() {
  if (bulk_file_reader_.is_open()) {
    while (true) {
      HFSAT::GenericORSReplyStruct ors_reply;

      size_t read_length_ =
          bulk_file_reader_.read(reinterpret_cast<char*>(&ors_reply), sizeof(HFSAT::GenericORSReplyStruct));

      if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

      DoPlayORSReply(ors_reply);
    }
  } else {
    std::cout << "Cannot Open File " << filename_ << std::endl;
  }
}

void ORSMessageReader::DoPlayORSReply(const GenericORSReplyStruct& ors_reply) {
  listener_->ORSMessageBegin(security_id_, ors_reply);
  switch (ors_reply.orr_type_) {
    case kORRType_Seqd:
      listener_->OrderSequencedAtTime(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                                      ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                                      ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
                                      ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
                                      ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_Conf:
      listener_->OrderConfirmedAtTime(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                                      ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                                      ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
                                      ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
                                      ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_ORSConf:  // Not getting used
      listener_->OrderConfirmedAtTime(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                                      ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                                      ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
                                      ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
                                      ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_CxlSeqd:
      listener_->OrderCxlSequencedAtTime(
          ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
          ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_, ors_reply.buysell_,
          ors_reply.size_remaining_, ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
          ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_CxlRejc:
      listener_->OrderCancelRejected(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                                     ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                                     ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
                                     ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
                                     ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_Cxld:
      listener_->OrderCanceledAtTime(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                                     ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                                     ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.client_position_,
                                     ors_reply.global_position_, ors_reply.int_price_,
                                     ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_Exec:
      listener_->OrderExecuted(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                               ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                               ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
                               ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
                               ors_reply.server_assigned_message_sequence_, ors_reply.exch_assigned_sequence_,
                               ors_reply.time_set_by_server_);
      break;
    case kORRType_IntExec:
      listener_->OrderInternallyMatched(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                                        ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                                        ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
                                        ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
                                        ors_reply.server_assigned_message_sequence_, ors_reply.exch_assigned_sequence_,
                                        ors_reply.time_set_by_server_);
      break;
    case kORRType_CxRe:
      listener_->OrderConfCxlReplaced(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                                      ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_,
                                      ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
                                      ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
                                      ors_reply.server_assigned_message_sequence_, ors_reply.exch_assigned_sequence_,
                                      ors_reply.time_set_by_server_);
      break;
    case kORRType_CxReRejc:
      listener_->OrderConfCxlReplaceRejected(
          ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
          ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.price_, ors_reply.buysell_,
          ors_reply.size_remaining_, ors_reply.client_position_, ors_reply.global_position_, ors_reply.int_price_,
          ors_reply.size_executed_, ors_reply.server_assigned_message_sequence_, ors_reply.exch_assigned_sequence_,
          ors_reply.time_set_by_server_);
      break;
    case kORRType_Rejc:
      listener_->OrderRejected(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                               security_id_, ors_reply.price_, ors_reply.buysell_, ors_reply.size_remaining_,
                               ors_reply.size_executed_, ors_reply.int_price_, ors_reply.exch_assigned_sequence_,
                               ors_reply.time_set_by_server_);
      break;
    case kORRType_Rejc_Funds:
      listener_->OrderRejectedDueToFunds(
          ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_, security_id_,
          ors_reply.price_, ors_reply.buysell_, ors_reply.size_remaining_, ors_reply.size_executed_,
          ors_reply.int_price_, ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_Wake_Funds:
      listener_->WakeUpifRejectedDueToFunds();
      break;
    case kORRType_None:
      listener_->OrderNotFound(ors_reply.server_assigned_client_id_, ors_reply.client_assigned_order_sequence_,
                               ors_reply.server_assigned_order_sequence_, security_id_, ors_reply.buysell_,
                               ors_reply.int_price_, ors_reply.server_assigned_message_sequence_,
                               ors_reply.exch_assigned_sequence_, ors_reply.time_set_by_server_);
      break;
    case kORRType_CxReSeqd:
      break;
    default:
      std::cerr << "Unknown ORS Message Type : " << ors_reply.orr_type_ << std::endl;
      break;
  }
  listener_->ORSMessageEnd(security_id_, ors_reply);
}
}
