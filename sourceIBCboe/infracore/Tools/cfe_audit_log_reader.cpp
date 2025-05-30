#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "infracore/CFET/CFETemplates/ResponseHeader.hpp"
#include "infracore/CFET/CFETemplates/LogonResponse.hpp"
#include "infracore/CFET/CFETemplates/HeartbeatResponse.hpp"
#include "infracore/CFET/CFETemplates/LogoutResponse.hpp"
#include "infracore/CFET/CFETemplates/RateInformationResponse.hpp"
#include "infracore/CFET/CFETemplates/SequenceResetResponse.hpp"
#include "infracore/CFET/CFETemplates/SessionLevelRejectResponse.hpp"
#include "infracore/CFET/CFETemplates/BusinessLevelRejectResponse.hpp"
#include "infracore/CFET/CFETemplates/OrderResponse1.hpp"
#include "infracore/CFET/CFETemplates/OrderResponse1_3.hpp"
#include "infracore/CFET/CFETemplates/OrderFillResponse1_3.hpp"
#include "infracore/CFET/CFETemplates/OrderBustReport.hpp"
#include "infracore/CFET/cfe_container.hpp"
#include "infracore/CFET/CFEEngine.hpp"

void ParseAuditIn(std::string filename);

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage : <exec> <AUDIT_IN> <input-file>" << std::endl;
    exit(-1);
  }

  std::string option(argv[1]);
  std::string filename(argv[2]);

  if (option == "AUDIT_IN") {
    ParseAuditIn(filename);
  } else {
    std::cout << "Invalid Option" << std::endl;
  }

  return 0;
}

void ParseAuditIn(std::string filename) {
  HFSAT::BulkFileReader reader1, reader2;
  reader1.open(filename);
  reader2.open(filename);
  typedef struct {
    uint8_t marker;
    uint8_t version;
    uint8_t reserved;
    uint8_t flags;
    uint32_t len;
    uint32_t seqno;
    uint16_t reserved2;
    uint16_t message_type;
  } HeaderPrefix;

  HeaderPrefix header;

  size_t MDS_SIZE_ = sizeof(HeaderPrefix);
  size_t available_len_1;
  size_t msg_len;

  while (true) {
    available_len_1 = reader1.read(&header, MDS_SIZE_);
    msg_len = EXCLUDE_TILL_MESSAGE_LENGTH_FIELD + (unsigned long int)(ntoh32(header.len));
    if (available_len_1 < MDS_SIZE_) break;
    char buff[10000];
    reader1.read(buff, msg_len - MDS_SIZE_);
    char msg[10000];
    reader2.read(msg, msg_len);
    HFSAT::CFET::ProcessedResponseHeader* processed_response_header_;
    HFSAT::CFET::HeaderResponse response_header_;
    processed_response_header_ = response_header_.ProcessResponseHeader(msg);
    switch (processed_response_header_->MessageType) {
      case MESSAGE_TYPE_LOGON_RESPONSE: {
        HFSAT::CFET::ProcessedLogonResponse* processed_logon_response_;
        HFSAT::CFET::LogonResponse logon_response_;
        processed_logon_response_ = logon_response_.ProcessLogonResponse(msg);
        std::cout << processed_logon_response_->ToString() << std::endl;
      } break;

      case MESSAGE_TYPE_HEARTBEAT: {
        HFSAT::CFET::ProcessedHeartbeatResponse* processed_heartbeat_response_;
        HFSAT::CFET::HeartbeatResponse heartbeat_response_;
        processed_heartbeat_response_ = heartbeat_response_.ProcessHeartbeatResponse(msg);
        std::cout << processed_heartbeat_response_->ToString() << std::endl;
      } break;

      case MESSAGE_TYPE_LOGOUT_REQUEST_OR_RESPONSE: {
        HFSAT::CFET::ProcessedLogoutResponse* processed_logout_response_;
        HFSAT::CFET::LogoutResponse logout_response_;
        processed_logout_response_ = logout_response_.ProcessLogoutResponse(msg);
        std::cout << processed_logout_response_->ToString() << std::endl;

      } break;

      case MESSAGE_TYPE_SESSION_LEVEL_REJECT: {
        HFSAT::CFET::SessionLevelRejectResponse sesion_level_reject_response_;
        HFSAT::CFET::ProcessedSessionLevelReject* processed_session_level_reject_response_;
        processed_session_level_reject_response_ = sesion_level_reject_response_.ProcessSessionLevelRejectResponse(msg);
        std::cout << processed_session_level_reject_response_->ToString() << std::endl;
      } break;

      case MESSAGE_TYPE_SEQUENCE_RESET_NOTIFICATION: {
        HFSAT::CFET::SequenceResetResponse sequence_reset_response_;
        HFSAT::CFET::ProcessedSequenceResetNotification* processed_sequence_reset_response_;
        processed_sequence_reset_response_ = sequence_reset_response_.ProcessSequenceResetResponse(msg);
        std::cout << processed_sequence_reset_response_->ToString() << std::endl;
      } break;

      case MESSAGE_TYPE_RATE_INFORMATION: {
        HFSAT::CFET::RateInformationResponse rate_information_response_;
        HFSAT::CFET::ProcessedRateInformation* processed_rate_information_response_;
        processed_rate_information_response_ = rate_information_response_.ProcessRateInformationResponse(msg);
        std::cout << processed_rate_information_response_->ToString() << std::endl;

      } break;

      case MESSAGE_TYPE_ORDER_FORMAT_1_RESPONSE: {
        HFSAT::CFET::ProcessedOptimizedOrderFormat_1_Response* processed_optimized_order_format_1_response_;
        HFSAT::CFET::OrderFormat1Response order_format_1_response_;
        processed_optimized_order_format_1_response_ =
            order_format_1_response_.ProcessOptimizedOrderFormat1Response(msg);
        std::cout << processed_optimized_order_format_1_response_->ToString() << std::endl;

      } break;

      // THIS IS NOT USED CURRENTLY
      case MESSAGE_TYPE_ORDER_FORMAT_1_3_RESPONSE: {
        HFSAT::CFET::ProcessedOrderFormat_1_3_Response* processed_order_format_1_3_response_;
        HFSAT::CFET::OrderFormat1_3Response order_format_1_3_response_;
        processed_order_format_1_3_response_ = order_format_1_3_response_.ProcessOrderFormat1_3Response(msg);
        std::cout << processed_order_format_1_3_response_->ToString() << std::endl;

      } break;

      case MESSAGE_TYPE_ORDER_FORMAT_1_3_FILL_RESPONSE: {
        HFSAT::CFET::ProcessedOptimizedOrderFormat_1_3_FillResponse*
            processed_optimized_order_format_1_3_fill_response_;
        HFSAT::CFET::OrderFormat1_3FillResponse order_format_1_3_fill_response_;
        processed_optimized_order_format_1_3_fill_response_ =
            order_format_1_3_fill_response_.ProcessOptimizedOrderFormat1_3FillResponse(msg);
        std::cout << processed_optimized_order_format_1_3_fill_response_->ToString() << std::endl;

      } break;

      case MESSAGE_TYPE_BUSINESS_LEVEL_REJECT: {
        HFSAT::CFET::BusinessLevelRejectResponse business_level_reject_response_;
        HFSAT::CFET::ProcessedBusinessLevelReject* processed_business_level_reject_response_;
        processed_business_level_reject_response_ =
            business_level_reject_response_.ProcessBusinessLevelRejectResponse(msg);
        std::cout << processed_business_level_reject_response_->ToString() << std::endl;

      } break;

      case MESSAGE_TYPE_ORDER_BUST_REPORT: {
        HFSAT::CFET::ProcessedOrderBustReport* processed_order_bust_report_;
        HFSAT::CFET::OrderBustReport order_bust_report_;
        processed_order_bust_report_ = order_bust_report_.ProcessOrderBustReport(msg);
        std::cout << "ORDER BUST -> " << processed_order_bust_report_->ToString() << std::endl;

      } break;

      default: { } break; }
  }
}
