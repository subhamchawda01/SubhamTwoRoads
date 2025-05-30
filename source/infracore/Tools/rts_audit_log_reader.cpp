#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/defines.hpp"
#include "infracore/TWIME/RTS/RTSTWEngine.hpp"
#include "infracore/TWIME/RTSTemplates/TWIMEDefines.hpp"
#include "dvccode/Utils/async_writer.hpp"
#include "dvccode/Utils/query_t2t.hpp"
inline uint8_t GetUint8(const char *bytes) { return bytes[0]; }
inline uint16_t GetUint16(const char *bytes) { return (*(uint16_t *)bytes); }
inline uint32_t GetUint32(const char *bytes) { return (*(uint32_t *)bytes); }
inline uint64_t GetUint64(uint64_t bytes) { return (*(uint64_t *)bytes); }
inline uint16_t GetInt16(const char *bytes) { return (*(int16_t *)bytes); }
inline uint32_t GetInt32(const char *bytes) { return (*(int32_t *)bytes); }
inline uint64_t GetInt64(const char *bytes) { return (*(int64_t *)bytes); }
void ParseAuditOut(std::string filename);
void ParseAuditIn(std::string filename);

typedef struct {
  uint16_t msg_len;
  uint16_t template_id;
  uint16_t schema_id;
  uint16_t schema_version;
} HeaderPrefix;
std::string print_time(uint64_t time) {
  std::string tm = "", res = "";
  while (time) {
    tm += char('0' + (time % 10));
    time /= 10;
  }
  reverse(tm.begin(), tm.end());
  int itr;
  for (itr = 0; itr < 10; itr++) {
    res += tm[itr];
  }

  res += ".";

  if (tm.size() >= 16) {
    for (; itr < 16; itr++) {
      res += tm[itr];
    }
  } else {
    int zero = 16 - tm.size();
    while (zero--) {
      res += "0";
    }
    for (; itr < (int)tm.size(); itr++) {
      res += tm[itr];
    }
  }
  return res;
}

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

  size_t MDS_SIZE_ = sizeof(HeaderPrefix);
  size_t available_len;

  while (true) {
    char buf[RTS_TWIME_MAX_MSG_LENGTH];
    HeaderPrefix header;
    available_len = reader1.read(&header, MDS_SIZE_);
    if (available_len < MDS_SIZE_) break;
    reader1.read(buf, header.msg_len);

    switch (header.template_id) {
      case LOGON_ACK_ID: {
        HFSAT::ORS::TWIME::EstablishmentAck logon_ack;
        reader2.read(&logon_ack, header.msg_len + MDS_SIZE_);

        std::cout << "LOGON_ACK:"
                  << " Time: " << print_time(logon_ack.request_time_stamp)
                  << " KeepAliveInt: " << logon_ack.keep_alive_interval << " NextSeqNo.: " << logon_ack.next_seq_no
                  << std::endl;
      } break;

      case LOGOUT_ID: {
        HFSAT::ORS::TWIME::Terminate logout;
        reader2.read(&logout, header.msg_len + MDS_SIZE_);

        std::cout << "LOGOUT:"
                  << " TerminationCode: " << (int8_t)logout.termination_code << std::endl;
      } break;

      case HEARTBEAT_ID: {
        HFSAT::ORS::TWIME::Sequence heartbeat;
        reader2.read(&heartbeat, header.msg_len + MDS_SIZE_);

        std::cout << "HEARTBEAT RESPONSE:"
                  << " NextSeqNo: " << heartbeat.next_seq_no << std::endl;
      } break;

      case NEW_ORDER_SINGLE_CONF_ID: {
        HFSAT::ORS::TWIME::NewOrderSingleResponse conf;
        reader2.read(&conf, header.msg_len + MDS_SIZE_);

        std::cout << "NEW_ORDER_SINGLE_CONF:"
                  << " ClOrdID: " << conf.cl_ord_id << " TimeStamp: " << print_time(conf.time_stamp)
                  << " ExpireDate: " << conf.expire_date << " OrderID: " << conf.order_id << " Flags: " << conf.flags
                  << " Price: " << conf.price << " SecurityID: " << conf.security_id << " OrderQty: " << conf.order_qty
                  << " TradingSessionID: " << conf.trading_session_id << " ClOrdLinkID: " << conf.cl_ord_link_id
                  << " Side: " << (int)conf.side << std::endl;
      } break;

      case NEW_ORDER_REJECT_ID: {
        HFSAT::ORS::TWIME::NewOrderReject reject;
        reader2.read(&reject, header.msg_len + MDS_SIZE_);

        std::cout << "NEW_ORDER_REJECT: SAOS: " << reject.cl_ord_id << " Reason: " << reject.ord_rej_reason
                  << " ReasonStr: " << HFSAT::ORS::TWIME::error_map[reject.ord_rej_reason] << std::endl;
      } break;

      case CANCEL_ORDER_CONF_ID: {
        HFSAT::ORS::TWIME::OrderCancelResponse conf;
        reader2.read(&conf, header.msg_len + MDS_SIZE_);

        std::cout << "CANCEL_ORDER_CONF:"
                  << " ClOrdID: " << conf.cl_ord_id << " TimeStamp: " << print_time(conf.time_stamp)
                  << " OrderID: " << conf.order_id << " Flags: " << conf.flags << " OrderQty: " << conf.order_qty
                  << " TradingSessionID: " << conf.trading_session_id << " ClOrdLinkID: " << conf.cl_ord_link_id
                  << std::endl;
      } break;

      case CANCEL_ORDER_REJECT_ID: {
        HFSAT::ORS::TWIME::OrderCancelReject reject;
        reader2.read(&reject, header.msg_len + MDS_SIZE_);

        std::cout << "CANCEL_ORDER_REJECT:  SAOS: " << reject.cl_ord_id << " Reason: " << reject.ord_rej_reason
                  << " ReasonStr: " << HFSAT::ORS::TWIME::error_map[reject.ord_rej_reason] << std::endl;
      } break;

      case MODIFY_ORDER_CONF_ID: {
        HFSAT::ORS::TWIME::OrderReplaceResponse conf;
        reader2.read(&conf, header.msg_len + MDS_SIZE_);

        std::cout << "MODIFY_ORDER_CONF:"
                  << " ClOrdID: " << conf.cl_ord_id << " TimeStamp: " << print_time(conf.time_stamp)
                  << " OrderID: " << conf.order_id << " PrevOrderID: " << conf.prev_order_id << " Flags: " << conf.flags
                  << " Price: " << conf.price << " OrderQty: " << conf.order_qty
                  << " TradingSessionID: " << conf.trading_session_id << " ClOrdLinkID: " << conf.cl_ord_link_id
                  << std::endl;
      } break;

      case MODIFY_ORDER_REJECT_ID: {
        HFSAT::ORS::TWIME::OrderReplaceReject reject;
        reader2.read(&reject, header.msg_len + MDS_SIZE_);

        std::cout << "MODIFY_ORDER_REJECT : SAOS: " << reject.cl_ord_id << " Reason: " << reject.ord_rej_reason
                  << " ReasonStr: " << HFSAT::ORS::TWIME::error_map[reject.ord_rej_reason] << std::endl;
      } break;

      case EXECUTION_REPORT: {
        HFSAT::ORS::TWIME::ExecutionSingleReport report;
        reader2.read(&report, header.msg_len + MDS_SIZE_);

        HFSAT::TradeType_t buy_sell =
            ((int)report.side == (int)HFSAT::ORS::TWIME::SideEnum::Buy) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

        std::cout << "EXECUTION REPORT : SAOS: " << report.cl_ord_id << " Price: " << report.last_px / PRICE_MULTIPLIER
                  << " SizeExec: " << report.last_qty << " SizeRemaining: " << report.order_qty << " B/S: " << buy_sell
                  << " SecID: " << report.security_id << std::endl;
      } break;

      case 5005: {
        HFSAT::ORS::TWIME::Retransmission r;
        reader2.read(&r, header.msg_len + MDS_SIZE_);

        std::cout << "CASE 5005:"
                  << " NextSeqNo: " << r.next_seq_no << " RequestTimestamp: " << print_time(r.request_time_stamp)
                  << " Count: " << r.count << std::endl;
      } break;

      case 7007: {
        HFSAT::ORS::TWIME::OrderMassCancelResponse r;
        reader2.read(&r, header.msg_len + MDS_SIZE_);

        std::cout << "CASE 7007:"
                  << " ClOrdID: " << r.cl_ord_id << " TimeStamp: " << print_time(r.time_stamp)
                  << " TotalAffectedOrders: " << r.total_affected_orders
                  << " OrdRejReason: " << (int8_t)r.ord_rej_reason << ")" << std::endl;
      } break;

      case 7011: {
        HFSAT::ORS::TWIME::SystemEvent r;
        reader2.read(&r, header.msg_len + MDS_SIZE_);

        std::cout << "CASE 7011:"
                  << " TimeStamp: " << print_time(r.time_stamp) << " TradingSessionID: " << r.trading_session_id
                  << " TradSesEvent: " << (int8_t)r.trad_ses_event << std::endl;
      } break;

      default: { reader2.read(buf, header.msg_len + MDS_SIZE_); } break;
    }
  }
}

void ParseAuditOut(std::string filename) {
  HFSAT::BulkFileReader reader1, reader2;
  reader1.open(filename);
  reader2.open(filename);

  size_t MDS_SIZE_ = sizeof(HeaderPrefix);
  size_t available_len;

  while (true) {
    char buf[RTS_TWIME_MAX_MSG_LENGTH];
    HeaderPrefix header;
    available_len = reader1.read(&header, MDS_SIZE_);
    if (available_len < MDS_SIZE_) break;
    reader1.read(buf, header.msg_len);

    switch (header.template_id) {
      case LOGON_ID: {
        HFSAT::ORS::TWIME::Establish logon;
        reader2.read(&logon, MDS_SIZE_ + header.msg_len);

        std::cout << "LOGIN:"
                  << " Timestamp: " << print_time(logon.time_stamp)
                  << " KeepaliveInterval: " << logon.keep_alive_interval << std::endl;

      } break;

      case LOGOUT_ID: {
        HFSAT::ORS::TWIME::Terminate logout;
        reader2.read(&logout, header.msg_len + MDS_SIZE_);

        std::cout << "LOGOUT" << std::endl;
      } break;

      case HEARTBEAT_ID: {
        HFSAT::ORS::TWIME::Sequence heartbeat;
        reader2.read(&heartbeat, header.msg_len + MDS_SIZE_);

        std::cout << "HEARTBEAT RESPONSE:"
                  << " NextSeqNo: " << heartbeat.next_seq_no << std::endl;
      } break;

      case NEW_ORDER_SINGLE_ID: {
        HFSAT::ORS::TWIME::NewOrderSingle new_order;
        reader2.read(&new_order, header.msg_len + MDS_SIZE_);
        string time_in_force_str = "NOT_DEFINED";
        switch (new_order.time_in_force) {
          case HFSAT::ORS::TWIME::TimeInForceEnum::Day: {
            time_in_force_str = "Day";
          } break;
          case HFSAT::ORS::TWIME::TimeInForceEnum::IOC: {
            time_in_force_str = "IOC";
          } break;
          case HFSAT::ORS::TWIME::TimeInForceEnum::FOK: {
            time_in_force_str = "FOK";
          } break;
          case HFSAT::ORS::TWIME::TimeInForceEnum::GTD: {
            time_in_force_str = "GTD";
          } break;
        }
        string check_limit_str = "NOT_DEFINED";
        switch (new_order.check_limit) {
          case HFSAT::ORS::TWIME::CheckLimitEnum::DontCheck: {
            check_limit_str = "DontCheck";
          } break;
          case HFSAT::ORS::TWIME::CheckLimitEnum::Check: {
            check_limit_str = "Check";
          } break;
        }
        std::cout << "NEW_ORDER_SINGLE_ID:"
                  << " ClOrdID: " << new_order.cl_ord_id << " ExpireDate: " << new_order.expire_date
                  << " Price: " << new_order.price << " SecurityID: " << new_order.security_id
                  << " ClOrdLinkID: " << new_order.cl_ord_link_id << " OrderQty: " << new_order.order_qty
                  << " TimeInForce: " << time_in_force_str << " Side: " << (int)new_order.side
                  << " CheckLimit: " << check_limit_str << " Account: " << new_order.account << std::endl;
      } break;

      case CANCEL_ORDER_ID: {
        HFSAT::ORS::TWIME::OrderCancelRequest cancel_order;
        reader2.read(&cancel_order, header.msg_len + MDS_SIZE_);
        std::cout << "CANCEL_ORDER:"
                  << " ClOrdID: " << cancel_order.cl_ord_id << " OrderID: " << cancel_order.order_id
                  << " Account: " << cancel_order.account << std::endl;
      } break;

      case MODIFY_ORDER_ID: {
        HFSAT::ORS::TWIME::OrderReplaceRequest modify_order;
        reader2.read(&modify_order, header.msg_len + MDS_SIZE_);
        string check_limit_str = "NOT_DEFINED";
        switch (modify_order.check_limit) {
          case HFSAT::ORS::TWIME::CheckLimitEnum::DontCheck: {
            check_limit_str = "DontCheck";
          } break;
          case HFSAT::ORS::TWIME::CheckLimitEnum::Check: {
            check_limit_str = "Check";
          } break;
        }
        std::cout << "MODIFY_ORDER:"
                  << " ClOrdID: " << modify_order.cl_ord_id << " OrderID: " << modify_order.order_id
                  << " Price: " << modify_order.price << " OrderQty: " << modify_order.order_qty
                  << " ClOrdLinkID: " << modify_order.cl_ord_link_id << " Mode: " << (int8_t)modify_order.mode
                  << " CheckLimit: " << check_limit_str << " Account: " << modify_order.account << std::endl;
      } break;

      default: { reader2.read(buf, header.msg_len + MDS_SIZE_); } break;
    }
  }
}
