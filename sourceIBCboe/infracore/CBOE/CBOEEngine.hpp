
#pragma once

#ifndef CBOEENGINE_HPP
#define CBOEENGINE_HPP

#define USE_NORMAL_SOCKET 0

#include <ctime>
#include <stack>
#include "dvccode/CDef/fwd_decl.hpp"

#include <string>
#include <map>
#include <ctime>
#include <fstream>
#include <memory>
#include <algorithm>
#include <thread>
#include <unordered_set>
#include <chrono>

#include "dvccode/Utils/settings.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"
#include "dvccode/CDef/cboe_security_definition.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "dvccode/Utils/cboe_refdata_loader.hpp"
#include "dvccode/Utils/cboe_daily_token_symbol_handler.hpp"

#include "dvccode/IBUtils/StdAfx.hpp"
#include "dvccode/IBUtils/EWrapper.hpp"
#include "dvccode/IBUtils/EReaderOSSignal.hpp"
#include "dvccode/IBUtils/EReader.hpp"
#include "dvccode/IBUtils/EClientSocket.hpp"
#include "dvccode/IBUtils/EPosixClientSocketPlatform.hpp"
#include "dvccode/IBUtils/Contract.hpp"
#include "dvccode/IBUtils/Order.hpp"
#include "dvccode/IBUtils/OrderState.hpp"
#include "dvccode/IBUtils/Execution.hpp"
#include "dvccode/IBUtils/CommissionReport.hpp"
#include "dvccode/IBUtils/ContractSamples.hpp"
#include "dvccode/IBUtils/OrderSamples.hpp"
#include "dvccode/IBUtils/ScannerSubscription.hpp"
#include "dvccode/IBUtils/ScannerSubscriptionSamples.hpp"
#include "dvccode/IBUtils/executioncondition.hpp"
#include "dvccode/IBUtils/PriceCondition.hpp"
#include "dvccode/IBUtils/MarginCondition.hpp"
#include "dvccode/IBUtils/PercentChangeCondition.hpp"
#include "dvccode/IBUtils/TimeCondition.hpp"
#include "dvccode/IBUtils/VolumeCondition.hpp"
#include "dvccode/IBUtils/AvailableAlgoParams.hpp"
#include "dvccode/IBUtils/FAMethodSamples.hpp"
#include "dvccode/IBUtils/CommonDefs.hpp"
#include "dvccode/IBUtils/AccountSummaryTags.hpp"
#include "dvccode/IBUtils/contract_manager.hpp"

#include "infracore/BasicOrderRoutingServer/defines.hpp"  // for DEF_MAX_SEC_ID
#include "infracore/BasicOrderRoutingServer/engine_listener.hpp"
#include "infracore/BasicOrderRoutingServer/base_engine.hpp"
#include "infracore/ORSUtils/broadcast_manager.hpp"
#include "infracore/BasicOrderRoutingServer/position_manager.hpp"
#include "infracore/BasicOrderRoutingServer/sequence_generator.hpp"
#include "infracore/CBOE/CBOET/DataDefines.hpp"
#include "infracore/CBOE/cboe_container.hpp"

namespace HFSAT {
namespace CBOE {

enum State {
  ST_CONNECT,
  ST_TICKDATAOPERATION,
  ST_TICKDATAOPERATION_ACK,
  ST_TICKOPTIONCOMPUTATIONOPERATION,
  ST_TICKOPTIONCOMPUTATIONOPERATION_ACK,
  ST_DELAYEDTICKDATAOPERATION,
  ST_DELAYEDTICKDATAOPERATION_ACK,
  ST_MARKETDEPTHOPERATION,
  ST_MARKETDEPTHOPERATION_ACK,
  ST_REALTIMEBARS,
  ST_REALTIMEBARS_ACK,
  ST_MARKETDATATYPE,
  ST_MARKETDATATYPE_ACK,
  ST_HISTORICALDATAREQUESTS,
  ST_HISTORICALDATAREQUESTS_ACK,
  ST_OPTIONSOPERATIONS,
  ST_OPTIONSOPERATIONS_ACK,
  ST_CONTRACTOPERATION,
  ST_CONTRACTOPERATION_ACK,
  ST_MARKETSCANNERS,
  ST_MARKETSCANNERS_ACK,
  ST_FUNDAMENTALS,
  ST_FUNDAMENTALS_ACK,
  ST_BULLETINS,
  ST_BULLETINS_ACK,
  ST_ACCOUNTOPERATIONS,
  ST_ACCOUNTOPERATIONS_ACK,
  ST_ORDEROPERATIONS,
  ST_ORDEROPERATIONS_ACK,
  ST_OCASAMPLES,
  ST_OCASAMPLES_ACK,
  ST_CONDITIONSAMPLES,
  ST_CONDITIONSAMPLES_ACK,
  ST_BRACKETSAMPLES,
  ST_BRACKETSAMPLES_ACK,
  ST_HEDGESAMPLES,
  ST_HEDGESAMPLES_ACK,
  ST_TESTALGOSAMPLES,
  ST_TESTALGOSAMPLES_ACK,
  ST_FAORDERSAMPLES,
  ST_FAORDERSAMPLES_ACK,
  ST_FAOPERATIONS,
  ST_FAOPERATIONS_ACK,
  ST_DISPLAYGROUPS,
  ST_DISPLAYGROUPS_ACK,
  ST_MISCELANEOUS,
  ST_MISCELANEOUS_ACK,
  ST_CANCELORDER,
  ST_CANCELORDER_ACK,
  ST_FAMILYCODES,
  ST_FAMILYCODES_ACK,
  ST_SYMBOLSAMPLES,
  ST_SYMBOLSAMPLES_ACK,
  ST_REQMKTDEPTHEXCHANGES,
  ST_REQMKTDEPTHEXCHANGES_ACK,
  ST_REQNEWSTICKS,
  ST_REQNEWSTICKS_ACK,
  ST_REQSMARTCOMPONENTS,
  ST_REQSMARTCOMPONENTS_ACK,
  ST_NEWSPROVIDERS,
  ST_NEWSPROVIDERS_ACK,
  ST_REQNEWSARTICLE,
  ST_REQNEWSARTICLE_ACK,
  ST_REQHISTORICALNEWS,
  ST_REQHISTORICALNEWS_ACK,
  ST_REQHEADTIMESTAMP,
  ST_REQHEADTIMESTAMP_ACK,
  ST_REQHISTOGRAMDATA,
  ST_REQHISTOGRAMDATA_ACK,
  ST_REROUTECFD,
  ST_REROUTECFD_ACK,
  ST_MARKETRULE,
  ST_MARKETRULE_ACK,
  ST_PNL,
  ST_PNL_ACK,
  ST_PNLSINGLE,
  ST_PNLSINGLE_ACK,
  ST_CONTFUT,
  ST_CONTFUT_ACK,
  ST_PING,
  ST_PING_ACK,
  ST_REQHISTORICALTICKS,
  ST_REQHISTORICALTICKS_ACK,
  ST_REQTICKBYTICKDATA,
  ST_REQTICKBYTICKDATA_ACK,
  ST_WHATIFSAMPLES,
  ST_WHATIFSAMPLES_ACK,
  ST_IDLE,
  ST_IBKRATSSAMPLE,
  ST_IBKRATSSAMPLE_ACK,
  ST_WSH,
  ST_WSH_ACK
};

typedef enum { kNewOrder = 0, kModifyOrder } OrderType_t;

class CBOEEngine : public ORS::BaseEngine,
                   public SimpleSecuritySymbolIndexerListener,
                   public EWrapper,
                   public IBDataListener {
 public:
#include "dvccode/IBUtils/EWrapper_prototypes.hpp"

  EReaderOSSignal m_osSignal;
  EClientSocket *const m_pClient;
  State m_state;
  time_t m_sleepDeadline;
  OrderId m_orderId;
  std::unique_ptr<EReader> m_pReader;
  bool m_extraAuth;
  std::string m_bboExchange;

 private:
  bool keep_engine_running_;
  time_t last_send_time_;
  HFSAT::DebugLogger &dbglogger_;
  bool use_affinity_;
  uint32_t next_message_sequnece_;
  bool is_logged_in_;
  bool allow_new_orders_;
  std::map<std::string, uint32_t> exchange_symbol_to_exchange_security_code_map_;
  std::map<uint32_t, std::string> exchange_security_code_to_exchange_symbol_map_;
  uint32_t msecs_from_midnight_;
  uint32_t last_midnight_sec_;
  std::vector<int32_t> unique_message_sequence_to_saos_vec_;
  std::vector<uint32_t> saos_to_unique_message_sequence_vec_;
  int32_t user_id_;
  int32_t version_;
  int32_t price_multiplier_;
  std::vector<int> saos_conf_vec;  // redundant
  long long last_order_conf_time;  // redundant
  long long curr_order_conf_time;  // redundant
  long int last_conf_saos;         // redundant
  tr1::unordered_map<int64_t, int64_t>
      exch_order_num_to_saos_;  // Used for storing internal reference for orders redundant
  tr1::unordered_map<int64_t, bool>
      saos_notification_req;  // Used to forbid if duplicate callbacks are recieved from TWS Application
  tr1::unordered_map<int32_t, int64_t> saos_to_exch_order_num;  // Used to map saos with TWS Server assigned PermanentID
  tr1::unordered_map<int32_t, Order>
      permID_to_order_;  // Used to Retrive orders in Order Status Call back to inform listners (Account thread)
  tr1::unordered_map<int32_t, const char *>
      permID_to_exchangesymbol_;  // Used to get exchange symbol to pass trade notification to Account thread
  std::unordered_set<std::int64_t> permIDPool;  // Used for New Order Check or Existing Order Modification check
  std::unordered_map<std::int64_t, std::int64_t> orderId_to_saos_map_;  // Used to store msg sequence number to saos
  std::unordered_map<std::int64_t, std::int64_t> saos_to_orderId_map_;  // Used to store saos to msg sequence number
  tr1::unordered_map<int64_t, int64_t>
      permID_to_conId_;  // Used to store OrderID to conID to handle immediate order execution
  tr1::unordered_map<std::int64_t, TradeType_t>
      permID_to_buy_sell_map_;  // Used to store OrderID to BUY/SELL to handle immediate order execution
  tr1::unordered_map<std::int64_t, std::string> conID_to_exchangesymbol_;
  tr1::unordered_map<std::string, std::int64_t> exchangesymbol_to_conID;
  tr1::unordered_map<std::int64_t, std::int64_t> orderId_to_conID;
  tr1::unordered_map<std::int64_t, std::string> orderId_exchangesymbol_;
  tr1::unordered_map<std::int64_t, std::int64_t> orderId_to_permId_;  // Used to handle disconnection
  tr1::unordered_map<std::int64_t, std::int64_t> orderId_to_size_remaining_;
  std::unordered_map<long, std::vector<std::string>> orderExecutions;
  std::unordered_map<std::int64_t, std::uint64_t> orderId_to_lastFilledAmount;
  std::unordered_map<std::int64_t, bool> orderId_to_OrderConfNotif;
  std::unordered_map<std::int64_t, bool> orderId_to_OrderModifyNotif;
  std::unordered_map<std::int64_t, OrderType_t> orderId_to_OrderType;

  HFSAT::BroadcastManager *bcast_manager_;
  std::map<int32_t, std::string> token_to_internal_exchange_symbol_;
  HFSAT::CBOESecurityDefinitions &cboe_sec_def_;
  std::vector<HFSAT::FastPriceConvertor *> fast_px_convertor_vec_;
  HFSAT::ORS::Settings &setting_;
  CBOEContainer container_;
  HFSAT::Utils::CBOERefDataLoader &ref_data_loader;
  HFSAT::Utils::CBOEDailyTokenSymbolHandler &cboe_daily_token_symbol_handler_;
  char cboe_segment_type_;
  bool is_mkt_order_;
  bool is_pre_open_;
  bool is_post_open;
  char *tcp_client_normal_socket_read_buffer_;
  int32_t send_write_length;
  int32_t cancel_write_length;
  int32_t modify_write_length;
  bool is_connected;
  std::string host_ip;
  long int host_port;
  int clientId = 0;
  bool response_subscription;
  std::string accountsList_;
  int32_t reqId_;
  ExecutionFilter filter;
  bool handle_reconnection;

 public:
  int32_t id_;
  AsyncWriter *pReader_;
  TcpClientSocketWithLogging *tcp_client_normal_socket_;

 private: 
  bool use_data_over_html;

 public:
  CBOEEngine(HFSAT::ORS::Settings &settings, HFSAT::DebugLogger &dbglogger, std::string output_log_dir,
             int32_t engine_id, AsyncWriter *pWriter, AsyncWriter *pReader);

  ~CBOEEngine();

  void CleanUp();

  void SendOrder(HFSAT::ORS::Order *order);
  void SendOrder(std::vector<HFSAT::ORS::Order *> multi_leg_order_ptr_vec_) override {}
  void SendSpreadOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) override;
  void SendThreeLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_, HFSAT::ORS::Order *order3_) override;
  void SendTwoLegOrder(HFSAT::ORS::Order *order1_, HFSAT::ORS::Order *order2_) override;
  void CancelOrder(HFSAT::ORS::Order *order);
  void ModifyOrder(HFSAT::ORS::Order *order, ORS::Order *orig_order);
  void OnAddString(uint32_t num_sid);
  void PreloadExchangeSymbol();

  inline time_t &lastMsgSentAtThisTime() { return last_send_time_; }
  inline bool is_engine_running() { return keep_engine_running_; }

  void Connect();
  void DisConnect();
  void Login();
  void Logout();
  void thread_main();
  virtual void ProcessLockFreeTCPDirectRead();
  void ProcessTWSResponse();
  void onInputAvailable();
  void SetDynamicOrderEntryRequestFields(Contract &contract_, std::string lastTradeDateOrContractMonth_, double strike_,
                                         std::string right_);

  void printContractDetailsSecIdList(const TagValueListSPtr &secIdList);
  void printContractMsg(const Contract &contract);
  void printBondContractDetailsMsg(const ContractDetails &contractDetails);
  void printContractDetailsMsg(const ContractDetails &contractDetails);
  void pnlOperation();
  void pnlSingleOperation();
  void tickDataOperation();
  void tickOptionComputationOperation();
  void delayedTickDataOperation();
  void marketDepthOperations();
  void realTimeBars();
  void marketDataType();
  void historicalDataRequests();
  void optionsOperations();
  void accountOperations();
  void orderOperations(){};
  void orderOperations(ORS::Order *ord, InstrumentDesc *inst_desc);
  void ocaSamples();
  void conditionSamples();
  void bracketSample();
  void hedgeSample();
  void contractOperations();
  void marketScanners();
  void fundamentals();
  void bulletins();
  void testAlgoSamples();
  void financialAdvisorOrderSamples();
  void financialAdvisorOperations();
  void testDisplayGroups();
  void miscelaneous();
  void reqFamilyCodes();
  void reqMatchingSymbols();
  void reqMktDepthExchanges();
  void reqNewsTicks();
  void reqSmartComponents();
  void reqNewsProviders();
  void reqNewsArticle();
  void reqHistoricalNews();
  void reqHeadTimestamp();
  void reqHistogramData();
  void rerouteCFDOperations();
  void marketRuleOperations();
  void continuousFuturesOperations();
  void reqHistoricalTicks();
  void reqTickByTickData();
  void whatIfSamples();
  void ibkratsSample();
  void wshCalendarOperations();
  void disconnect(){};
  void processCBOEImmediateExecutionResponse(OrderId orderId, int permId, double avgFillPrice, Decimal filled,
                                             Decimal remaining, OrderType_t order_type);
  void RequestORSPNL();
  void RequestORSOpenPositions();
  void KillSwitch(int32_t sec_id);
  void FetchMarginUsage();
  void RequestExecutedOrders();
  std::string getCurrentDateTime();

  void reqCurrentTime();
  void setConnectOptions(const std::string &);
  const int PING_DEADLINE = 2;         // seconds
  const int SLEEP_BETWEEN_PINGS = 30;  // seconds

  // Function to convert enum to string
  std::string orderTypeToString(OrderType_t orderType) {
    switch (orderType) {
      case kNewOrder:
        return "kNewOrder";
      case kModifyOrder:
        return "kModifyOrder";
      default:
        return "UnknownOrderType";
    }
  }

  std::string doubleMaxString(double d, std::string def) {
    if (d == DBL_MAX) {
      return def;
    } else {
      std::ostringstream oss;
      oss.precision(8);
      oss << std::fixed << d;
      std::string str = oss.str();

      std::size_t pos1 = str.find_last_not_of("0");
      if (pos1 != std::string::npos) str.erase(pos1 + 1);

      std::size_t pos2 = str.find_last_not_of(".");
      if (pos2 != std::string::npos) str.erase(pos2 + 1);

      return str;
    }
  }

  std::string base64_chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  inline bool is_base64(std::uint8_t c) { return (isalnum(c) || (c == '+') || (c == '/')); }

  std::vector<std::uint8_t> base64_decode(std::string const &encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    std::uint8_t char_array_4[4], char_array_3[3];
    std::vector<std::uint8_t> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
      char_array_4[i++] = encoded_string[in_];
      in_++;
      if (i == 4) {
        for (i = 0; i < 4; i++) char_array_4[i] = base64_chars.find(char_array_4[i]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (i = 0; (i < 3); i++) ret.push_back(char_array_3[i]);
        i = 0;
      }
    }

    if (i) {
      for (j = i; j < 4; j++) char_array_4[j] = 0;

      for (j = 0; j < 4; j++) char_array_4[j] = base64_chars.find(char_array_4[j]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
  }

  std::string intMaxString(int value) { return value == INT_MAX ? "" : std::to_string(value); }

  std::string longMaxString(long value) { return value == LONG_MAX ? "" : std::to_string(value); }

  std::string llongMaxString(long long value) { return value == LLONG_MAX ? "" : std::to_string(value); }

  std::string doubleMaxString(double d) { return doubleMaxString(d, ""); }
};

}  // namespace CBOE
}  // namespace HFSAT

#endif  // CBOEENGINE_HPP

/*
To do should remove entry from below maps ??

Remove all the entries from the map in a function by passing orderID, PermID & saos etc.
permID_to_exchangesymbol_
orderId_to_saos_map_
saos_to_orderId_map_
permID_to_conId_
orderId_exchangesymbol_
orderId_to_size_remaining_
orderId_to_conID
orderId_to_OrderType

Should improve OnOrder Cancel received from TWS Application as we can receive it any time


*/