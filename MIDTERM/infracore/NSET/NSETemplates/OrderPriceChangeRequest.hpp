
#pragma once
#include "infracore/NSET/NSETemplates/OEBaseStructs.hpp"

namespace HFSAT {
namespace NSE {

//! PriceMod Structure: 74 Bytes
//! Non-trimmed but optimized structure
#pragma pack(push, 1)
struct PriceMod : public DCHeader, MessageHeader {
  int32_t token_no_;
  int32_t trader_id_;
  int64_t order_no_;
  int16_t buy_sell_;
  int32_t price_;
  int32_t volume_;
  int32_t last_mod_;
  int32_t saos_;
  int64_t last_activity_ref_;
  char reserved_[24];

  PriceMod() {
    length_ = hton16(sizeof(PriceMod));
    trans_code_ = hton16((int16_t)2013);
    msg_len_ = hton16(sizeof(PriceMod) - sizeof(DCHeader));
    token_no_ = 0;
    trader_id_ = 0;
    order_no_ = 0;
    buy_sell_ = 0;
    price_ = 0;
    volume_ = 0;
    last_mod_ = 0;
    last_activity_ref_ = 0;
    saos_ = 0;
  }
  inline void SetTranscode(int16_t trans_code) { trans_code_ = ntoh16(trans_code); }
  inline void SetTraderId(int32_t user_id) {
    m_trader_id_ = hton32(user_id);
    trader_id_ = hton32(user_id);
  }
  inline void SetTokenNo(int32_t token_no) { token_no_ = hton32(token_no); }
  inline void SetTwiddledTokenNo(int32_t twiddled_token_no) { token_no_ = twiddled_token_no; }
  inline void SetPacketSequenceNumber(int32_t packet_sequence_number) { seq_no_ = hton32(packet_sequence_number); }
  inline void SetOrderNumber(int64_t order_num) { order_no_ = hton64(order_num); }
  inline void SetLastModifiedDateTime(int32_t last_date) { last_mod_ = hton32(last_date); }
  inline void SetBuySell(int16_t buy_sell) { buy_sell_ = hton16(buy_sell); }
  inline void SetVolume(int32_t volume) { volume_ = hton32(volume); }
  inline void SetPrice(int32_t price) { price_ = hton32(price); }
  inline void SetSaos(int32_t const &server_assigned_seq_no) { saos_ = hton32(server_assigned_seq_no); }
  inline void SetLastActivityReference(int64_t last_activity_ref) { last_activity_ref_ = hton64(last_activity_ref); }
  std::string ToString() {
    std::stringstream ss;
    ss << "  DCHeader::" << DCHeader::ToString();
    ss << "  MessageHeader::" << MessageHeader::ToString();
    ss << "  trans_code_:" << ntoh16(trans_code_);
    ss << "  token_no_:" << ntoh32(token_no_);
    ss << "  trader_id_:" << ntoh32(trader_id_);
    ss << "  order_no_:" << ntoh64(order_no_);
    ss << "  buy_sell_:" << ntoh16(buy_sell_);
    ss << "  Price:" << ntoh32(price_);
    ss << "  Volume:" << ntoh32(volume_);
    ss << "  Last_mod:" << ntoh32(last_mod_);
    ss << "  SAOS:" << ntoh32(saos_) << " ";
    std::string str = ss.str();
    return str;
  }

  inline void SetPreLoadedOrderEntryRequestFields(int32_t const &user_id) {
    m_trader_id_ = hton32(user_id);
    trader_id_ = hton32(user_id);
  }

  inline void SetDynamicOrderEntryRequestFields(int32_t const &packet_sequence_number, int64_t const &order_num,
                                                int32_t saos, int32_t const &last_date, int16_t const &buy_sell,
                                                int32_t const &volume, int64_t last_activity_ref, int32_t const &price,
						int32_t const twiddled_token_no) {
    SetTwiddledTokenNo(twiddled_token_no);
    SetPacketSequenceNumber(packet_sequence_number);
    SetOrderNumber(order_num);
    SetLastModifiedDateTime(last_date);
    SetBuySell(buy_sell);
    SetVolume(volume);
    SetPrice(price);
    SetSaos(saos);
    SetLastActivityReference(last_activity_ref);
    HFSAT::MD5::MD5((uint8_t *)&trans_code_, sizeof(PriceMod) - sizeof(DCHeader), (uint32_t *)checksum_);
  }

  char const *GetBuffer() const { return (char const *)&length_; }
};
#pragma pack(pop)

//! OrderPriceModifyRequest: 74 Byte
struct OrderPriceModifyRequest : public PriceMod {
 public:
  OrderPriceModifyRequest() : PriceMod() { SetTranscode(PRICE_MOD_IN); }
  char const *GetOrderPriceModifyRequestBuffer() const { return (char const *)&length_; }
};
}
}
