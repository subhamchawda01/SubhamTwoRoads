#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "string.h"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/Utils/async_writer.hpp"
#include "dvccode/Utils/query_t2t.hpp"
#include "dvccode/Utils/sem_utils.hpp"
#include "infracore/SGXOuch/sgx_common_utils.hpp"
#include "infracore/SGXOuch/sgx_omnet_api.hpp"

#include "infracore/SGXOuch/SGX_OMEX/omni.hpp"
#include "infracore/SGXOuch/SGX_OMEX/omex.hpp"
#include "infracore/SGXOuch/SGX_OMEX/omnifact.hpp"
#include "infracore/SGXOuch/SGX_OMEX/omniapi.hpp"
#include "infracore/SGXOuch/SGXEngine.hpp"

inline uint8_t GetUint8(const char *bytes) { return bytes[0]; }
uint16_t GetUint16(const char *bytes) { return hton16((*(uint16_t *)bytes)); }
inline uint32_t GetUint32(const char *bytes) { return (*(uint32_t *)bytes); }
inline uint64_t GetUint64(const char *bytes) { return (*(uint64_t *)bytes); }
inline uint16_t GetInt16(const char *bytes) { return (*(int16_t *)bytes); }
inline uint32_t GetInt32(const char *bytes) { return (*(int32_t *)bytes); }
inline uint64_t GetInt64(const char *bytes) { return (*(int64_t *)bytes); }
void ParseAuditOut(std::string filename);
void ParseAuditIn(std::string filename);

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: <exec> <AUDIT_IN/AUDIT_OUT> <input-file>" << std::endl;
    exit(-1);
  }

  std::string option(argv[1]);
  std::string filename(argv[2]);

  if (option == "AUDIT_OUT") {
    ParseAuditOut(filename);
  } else if (option == "AUDIT_IN") {
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

  size_t available_len;

  while (true) {
    char msg_ptr[SGX_OUCH_MAX_MSG_LENGTH];
    available_len = reader1.read(msg_ptr, 3);
    uint16_t msg_len = GetUint16(msg_ptr) + 2;
    char msg_type = msg_ptr[2];

    if (available_len < 3) break;

    reader1.read(msg_ptr, msg_len - 3);
    reader2.read(msg_ptr, msg_len);

    switch (msg_type) {
      case LOGIN_ACCEPTED: {
        char session[10];
        char seq_num[20];

        memcpy(session, msg_ptr + 3, sizeof(session));
        memcpy(seq_num, msg_ptr + 13, sizeof(seq_num));

        std::cout << "LOGIN_ACCEPTED: Session: " << session << " SeqNum: " << seq_num << std::endl;
      } break;

      case LOGIN_REJECTED: {
        char reason = msg_ptr[3];
        std::cout << "LOGIN_REJECTED : Reason: " << reason << std::endl;
        ;
        if (reason == 'A') {
          std::cout << "Logon Reject Reason : " << reason << " : Not Authorized, Invalid Username/Password"
                    << std::endl;
          ;
        } else if (reason == 'S') {
          std::cout << "Logon Reject Reason : " << reason
                    << " Session not available. Requested session not valid or not available " << std::endl;
          ;
        }
      } break;

      case SERVER_HEARTBEAT: {
        std::cout << "SERVER_HEARTBEAT\n";
      } break;

      case END_OF_SESSION: {
        std::cout << "END_OF_SESSION\n";
      } break;

      case SEQUENCED_DATA_PACKED: {
        char type = msg_ptr[3];
        switch (type) {
          case ORDER_ACCEPTED: {
            char *msg = msg_ptr;
            msg += 3;
            char saos_t[15] = {'\0'};
            memcpy(saos_t, msg + HFSAT::ORS::SGXOUCH::OrderAcceptedOffsets::kOrderToken, sizeof(saos_t) - 1);
            int64_t saos = atol(saos_t);

            uint32_t size = GetUint64(msg + HFSAT::ORS::SGXOUCH::OrderAcceptedOffsets::kQuantity);

            uint64_t exch_order_num = GetUint64(msg + HFSAT::ORS::SGXOUCH::OrderAcceptedOffsets::kOrderID);

            uint32_t book_id = GetUint32(msg + HFSAT::ORS::SGXOUCH::OrderAcceptedOffsets::kOrderBookID);
            double price = GetInt32(msg + HFSAT::ORS::SGXOUCH::OrderAcceptedOffsets::kPrice);

            std::cout << "ORDER_ACCEPTED: SAOS: " << saos << " Size: " << size << " ExchOrderNum: " << exch_order_num
                      << " book_id: " << book_id << " Price: " << price << std::endl;

          } break;

          case ORDER_CANCELLED: {
            char *msg = msg_ptr;
            msg += 3;
            char saos_t[15] = {'\0'};
            memcpy(saos_t, msg + HFSAT::ORS::SGXOUCH::OrderCancelledOffsets::kOrderToken, sizeof(saos_t) - 1);
            int64_t saos = atol(saos_t);

            uint8_t reason = GetUint8(msg + HFSAT::ORS::SGXOUCH::OrderCancelledOffsets::kReason);

            std::cout << "ORDER_CANCELLED: SAOS: " << saos << " Reason: " << (int)reason << std::endl;

          } break;

          case ORDER_REPLACED: {
            char *msg = msg_ptr;
            msg += 3;
            char saos_t[15] = {'\0'};
            char new_saos_t[15] = {'\0'};
            memcpy(saos_t, msg + HFSAT::ORS::SGXOUCH::OrderReplacedOffsets::kPreviousOrderToken, sizeof(saos_t) - 1);
            memcpy(new_saos_t, msg + HFSAT::ORS::SGXOUCH::OrderReplacedOffsets::kReplacementOrderToken,
                   sizeof(new_saos_t) - 1);

            int64_t saos = atol(saos_t);

            int64_t new_saos = atol(new_saos_t);

            uint32_t size = GetUint64(msg + HFSAT::ORS::SGXOUCH::OrderReplacedOffsets::kQuantity);
            uint64_t exch_order_num = GetUint64(msg + HFSAT::ORS::SGXOUCH::OrderReplacedOffsets::kOrderID);
            uint32_t book_id = GetUint32(msg + HFSAT::ORS::SGXOUCH::OrderReplacedOffsets::kOrderBookID);
            double price = GetInt32(msg + HFSAT::ORS::SGXOUCH::OrderReplacedOffsets::kPrice);
            std::cout << "ORDER_REPLACED: New SAOS: " << new_saos << " SAOS: " << saos << " NewSAOS: " << new_saos
                      << " Size: " << size << " OrderID:  " << exch_order_num << " book_id: " << book_id
                      << " Price: " << price << std::endl;

          } break;

          case ORDER_REJECTED: {
            char *msg = msg_ptr;
            msg += 3;
            char saos_t[15] = {'\0'};
            memcpy(saos_t, msg + HFSAT::ORS::SGXOUCH::OrderRejectedOffsets::kOrderToken, sizeof(saos_t) - 1);
            int64_t saos = atol(saos_t);
            int32_t reject_code = GetInt32(msg + HFSAT::ORS::SGXOUCH::OrderRejectedOffsets::kRejectCode);

            std::cout << "ORDER_REJECTED: SAOS: " << saos << " RejectCode: " << reject_code << std::endl;
          } break;

          case ORDER_EXECUTED: {
            char *msg = msg_ptr;
            msg += 3;
            char saos_t[15] = {'\0'};
            memcpy(saos_t, msg + HFSAT::ORS::SGXOUCH::OrderExecutedOffsets::kOrderToken, sizeof(saos_t) - 1);
            int64_t saos = atol(saos_t);

            uint32_t size = GetUint64(msg + HFSAT::ORS::SGXOUCH::OrderExecutedOffsets::kQuantity);

            int32_t book_id = GetUint32(msg + HFSAT::ORS::SGXOUCH::OrderExecutedOffsets::kOrderBookID);
            double price = GetInt32(msg + HFSAT::ORS::SGXOUCH::OrderExecutedOffsets::kPrice);

            std::cout << "ORDER_EXECUTED: SAOS: " << saos << " Size: " << size << " book_id: " << book_id
                      << " Price: " << price << std::endl;

          } break;
        }
      } break;
    }
  }
}

void ParseAuditOut(std::string filename) {
  HFSAT::BulkFileReader reader1, reader2;
  reader1.open(filename);
  reader2.open(filename);

  size_t available_len;

  while (true) {
    char buf[SGX_OUCH_MAX_MSG_LENGTH];
    available_len = reader1.read(buf, 3);
    uint16_t msg_len = GetUint16(buf) + 2;
    char msg_type = buf[2];

    if (available_len < 3) break;

    reader1.read(buf, msg_len - 3);

    switch (msg_type) {
      case CLIENT_HEARTBEAT: {
        HFSAT::ORS::SGXOUCH::ClientHeartbeat heartbeat;
        reader2.read(&heartbeat, msg_len);
        std::cout << "CLIENT_HEARTBEAT\n";
      } break;
      case LOGIN_REQUEST: {
        HFSAT::ORS::SGXOUCH::LoginRequest logon;
        reader2.read(&logon, msg_len);

        std::cout << "LOGIN_REQUEST: User: " << logon.user_name << " requested_session: " << logon.requested_session
                  << " requested_seq_num: " << logon.requested_seq_number << std::endl;
      } break;
      case LOGOUT_REQUEST: {
        reader2.read(buf, msg_len);
        std::cout << "LOGOUT_REQUEST\n";
      } break;
      case UNSEQUENCED_DATA_PACKED: {
        char type = buf[0];
        switch (type) {
          case ENTER_ORDER: {
            HFSAT::ORS::SGXOUCH::EnterOrder new_order_;
            reader2.read(&new_order_, msg_len);
            // Exchange prohibits us to end the token(saos) char buffer by a null character
            // Hence random trash characters would be printed while parsing after saos value
            std::cout << "ENTER_ORDER: Side: " << new_order_.side << " Price: " << new_order_.price
                      << " Size: " << new_order_.quantity << " Saos: " << std::string(new_order_.order_token)
                      << " time_in_force: " << (int)new_order_.time_in_force
                      << " order_book_id: " << new_order_.order_book_id
                      << " customer_info: " << new_order_.customer_info << std::endl;
          } break;
          case ORDER_REPLACE: {
            HFSAT::ORS::SGXOUCH::ReplaceOrder modify_order_;
            reader2.read(&modify_order_, msg_len);
            // Exchange prohibits us to end the token(saos) char buffer by a null character
            // Hence random trash characters would be printed while parsing after saos value
            std::cout << "ORDER_REPLACE: "
                      << "Existing saos: " << modify_order_.existing_order_token
                      << " New Saos: " << modify_order_.replacement_order_token
                      << " Quantity: " << modify_order_.quantity << " Price: " << modify_order_.price << std::endl;
          } break;
          case CANCEL_ORDER: {
            // Exchange prohibits us to end the token(saos) char buffer by a null character
            // Hence random trash characters would be printed while parsing after saos value
            HFSAT::ORS::SGXOUCH::DeleteOrder cancel_order_;
            reader2.read(&cancel_order_, msg_len);
            std::cout << "CANCEL_ORDER: Saos: " << (cancel_order_.order_token) << std::endl;
          } break;
        }
      } break;
      default: { reader2.read(buf, msg_len); } break;
    }
  }
}
