
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/html_generator.hpp"
#include "infracore/CBOE/CBOEEngine.hpp"
#include "dvccode/CDef/ttime.hpp"

#define MAX_TCP_CLIENT_NORMAL_SOCKET_READ_BUFFER_LENGTH 1024

namespace HFSAT {
namespace CBOE {

CBOEEngine::CBOEEngine(HFSAT::ORS::Settings& settings, HFSAT::DebugLogger& dbglogger, std::string output_log_dir,
                       int32_t engine_id, AsyncWriter* pWriter, AsyncWriter* pReader)
    : HFSAT::ORS::BaseEngine(settings, dbglogger),
      keep_engine_running_(false),
      last_send_time_(),
      dbglogger_(dbglogger),
      use_affinity_(false),
      next_message_sequnece_(0),
      is_logged_in_(false),
      allow_new_orders_(false),
      exchange_symbol_to_exchange_security_code_map_(),
      exchange_security_code_to_exchange_symbol_map_(),
      msecs_from_midnight_(0),
      last_midnight_sec_(HFSAT::DateTime::GetTimeMidnightUTC(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      unique_message_sequence_to_saos_vec_(),
      saos_to_unique_message_sequence_vec_(),
      user_id_(0),
      version_(0),
      price_multiplier_(settings.getIntValue("PriceMultiplier", 1)),
      saos_conf_vec(),
      last_order_conf_time(0),
      curr_order_conf_time(0),
      last_conf_saos(0),
      exch_order_num_to_saos_(),
      saos_notification_req(),
      bcast_manager_(BroadcastManager::GetUniqueInstance(dbglogger, "", 0,
                                                         (atoi(settings.getValue("Client_Base").c_str())) << 16)),
      token_to_internal_exchange_symbol_(),
      cboe_sec_def_(HFSAT::CBOESecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      fast_px_convertor_vec_(DEF_MAX_SEC_ID, NULL),
      setting_(settings),
      container_(),
      ref_data_loader(HFSAT::Utils::CBOERefDataLoader::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      cboe_daily_token_symbol_handler_(
          HFSAT::Utils::CBOEDailyTokenSymbolHandler::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal())),
      cboe_segment_type_('I'),
      is_mkt_order_(false),
      is_pre_open_(false),
      is_post_open(false),
      tcp_client_normal_socket_read_buffer_(new char[MAX_TCP_CLIENT_NORMAL_SOCKET_READ_BUFFER_LENGTH]),
      m_osSignal(2000),  // 2-seconds timeout
      m_pClient(new EClientSocket(this, &m_osSignal)),
      m_state(ST_CONNECT),
      m_sleepDeadline(0),
      m_orderId(0),
      m_extraAuth(false),
      response_subscription(false),
      reqId_(1),
      id_(engine_id),
      pReader_(pReader),
      handle_reconnection(false),

      tcp_client_normal_socket_(new TcpClientSocketWithLogging(true, pWriter, pReader, output_log_dir)),
      use_data_over_html(false) {
  std::cout << "CBOE Engine Constructor Initalization completed\n";

  if (!settings.has("HOST_IP") || !settings.has("HOST_PORT") || !settings.has("CLIENT_ID")) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "HOST_IP or HOST_PORT or CLIENT_ID IS MISSING " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    exit(-1);
  }
  if (settings.has("USE_DATA_OVER_HTML") && (settings.getValue("USE_DATA_OVER_HTML")) == std::string("Y")) {
    use_data_over_html=true;
    DBGLOG_CLASS_FUNC_LINE_FATAL << "USE_DATA_OVER_HTML " <<use_data_over_html<< DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }
  host_ip = settings.getValue("HOST_IP", "127.0.0.1");
  host_port = settings.getIntValue("HOST_PORT", 7497);
  clientId = settings.getIntValue("CLIENT_ID", 0);

  // Decide segment and msg handler based on exchange provided in config
  if (std::string(settings.getValue("Exchange")) == std::string("CBOE_FO")) {
    cboe_segment_type_ = CBOE_FO_SEGMENT_MARKING;
  } else if (std::string(settings.getValue("Exchange")) == std::string("NSE_CD")) {
    cboe_segment_type_ = NSE_CD_SEGMENT_MARKING;
  } else if (std::string(settings.getValue("Exchange")) == std::string("NSE_EQ")) {
    cboe_segment_type_ = NSE_EQ_SEGMENT_MARKING;
  }

  SimpleSecuritySymbolIndexer::GetUniqueInstance().AddSSSIListener(this);
  PreloadExchangeSymbol();
}

CBOEEngine::~CBOEEngine() {
  // Deallocate fast_px_convertor_vec_
  for (int secid = 0; secid < DEF_MAX_SEC_ID; secid++) {
    if (fast_px_convertor_vec_[secid] != NULL) {
      delete fast_px_convertor_vec_[secid];
      fast_px_convertor_vec_[secid] = NULL;
    }
  }

  if (m_pReader) m_pReader.reset();

  delete m_pClient;
}

void CBOEEngine::CleanUp() {
  // Ensure the SAOS and BranchCode/Sequence Are Always in sync
  HFSAT::ORS::SequenceGenerator::GetUniqueInstance().persist_seq_num();
}

inline void CBOEEngine::ProcessLockFreeTCPDirectRead() {
  if (true == is_connected) m_pReader->processMsgs();
}

void CBOEEngine::Connect() {
  std::cout << "Connect to TWS called\n";

  dbglogger_ << "TRYING CONNECTING TO HOST_IP " << host_ip << " HOST_PORT " << host_port << " WITH CLIENT ID "
             << clientId << "\n";

  last_send_time_ = time(NULL);
  //! [connect]
  bool bRes = m_pClient->eConnect((const char*)host_ip.c_str(), host_port, clientId, m_extraAuth);
  // std::this_thread::sleep_for(std::chrono::seconds(5));
  //! [connect]
  bool connection_result = m_pClient->isConnected();

  if (bRes && connection_result) {
    printf("Connected to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);
    //! [ereader]
    m_pReader = std::unique_ptr<EReader>(new EReader(m_pClient, &m_osSignal));
    // m_pReader->AddIBDataListenerFromReader(this);
    m_pReader->start();
    //! [ereader]
    p_engine_listener_->OnConnect(true);
    is_connected = true;

    filter.m_time = getCurrentDateTime();

  } else {
    dbglogger_ << "CONNECTION FAILED " << host_ip << " HOST_PORT " << host_port << " @ CLIENT ID" << clientId << "\n";
    dbglogger_ << "RETRY "
               << "\n";
    printf("Cannot connect to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);

    exit(1);
  }

  return;
  // return bRes;
}

void CBOEEngine::Login() { std::cout << "Login Implementation\n"; }

void CBOEEngine::DisConnect() {
  std::cout << "DisConnect Implementation\n";

  if (!m_pClient->isConnected()) {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "DISCONNECT CALLED WITHOUT BEING CONNECTED" << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
  }

  dbglogger_ << "PLACING REQUEST TO CANCEL ALL OPEN ORDER AT GLOBAL LEVEL\n";
  for (auto& pair : saos_notification_req) {
    pair.second = true;  // Set the value to true
  }
  m_pClient->reqGlobalCancel();
  dbglogger_ << "PLACING REQUEST TO CANCEL ALL OPEN ORDER AT GLOBAL LEVEL COMPLETED\n";
  dbglogger_ << "CAN VIEW ALL THE OUTSTANDING LIVE ORDERS IN TWS APPLICATION IN LIVE ORDERS TAB\n";

  std::this_thread::sleep_for(std::chrono::seconds(5));  // Sleep for 5 seconds

  m_pClient->eDisconnect();
  is_logged_in_ = false;
  p_engine_listener_->OnLogout();
  return;
}

void CBOEEngine::Logout() { std::cout << "Logout Implementation\n"; }

void CBOEEngine::SetDynamicOrderEntryRequestFields(Contract& contract_, std::string lastTradeDateOrContractMonth_,
                                                   double strike_, std::string right_) {
  contract_.lastTradeDateOrContractMonth = lastTradeDateOrContractMonth_;
  contract_.strike = strike_;
  contract_.right = right_;
}

void CBOEEngine::SendOrder(ORS::Order* order) {
  dbglogger_ << "Func: " << __func__ << "\n";

  struct timeval tv;
  gettimeofday(&tv, NULL);

  dbglogger_ << "PLACING NEW LIMIT ORDER @ " << tv.tv_sec << "." << tv.tv_usec << "\n";
  dbglogger_ << "PLACING NEW ORDER WITH ID " << m_orderId << "\n";

  last_send_time_ = time(NULL);

  dbglogger_ << "CBOEEngine::SendOrder " << order->toString() << "\n";
  InstrumentDesc* inst_desc = &container_.inst_desc_[order->security_id_];
  dbglogger_ << "Searching for sec id " << order->security_id_ << "\n";
  dbglogger_ << inst_desc->ToString() << "\n";
  dbglogger_ << "inst_desc->expiry_date_ " << inst_desc->expiry_date_ << "\n";
  dbglogger_ << "inst_desc->strike_price_ " << inst_desc->strike_price_ << "\n";
  dbglogger_ << "inst_desc->option_type_ " << inst_desc->option_type_ << "\n";

  Contract contract = ContractSamples::USOptionContract();
  SetDynamicOrderEntryRequestFields(contract, std::to_string(inst_desc->expiry_date_), (double)inst_desc->strike_price_,
                                    std::string(1, inst_desc->option_type_[0]));

  dbglogger_ << "contract Exp " << contract.lastTradeDateOrContractMonth << "\n";
  dbglogger_ << "contract Strike " << contract.strike << "\n";
  dbglogger_ << "contract Opt type " << contract.right << "\n";

  std::string buy_sell = (order->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL";
  long order_size = order->size_remaining_;
  double price = (double)(order->price_);

  dbglogger_ << "buy_sell " << buy_sell << "\n";
  dbglogger_ << "order_size " << order_size << "\n";
  dbglogger_ << "price " << price << "\n";

  Order limit_order = OrderSamples::LimitOrder(buy_sell, stringToDecimal(std::to_string(order_size)), price);

  dbglogger_ << "\n\n\n\n";

  dbglogger_ << "Contract Details:\n";
  dbglogger_ << "Symbol: " << contract.symbol << "\n"
             << "Security Type: " << contract.secType << "\n"
             << "Exchange: " << contract.exchange << "\n"
             << "Currency: " << contract.currency << "\n"
             << "Last Trade Date/Contract Month: " << contract.lastTradeDateOrContractMonth << "\n"
             << "Strike: " << contract.strike << "\n"
             << "Right: " << contract.right << "\n"
             << "Multiplier: " << contract.multiplier << "\n";

  dbglogger_ << "\n\n\n\n";

  dbglogger_ << "Order Details\n";
  dbglogger_ << "Action: " << limit_order.action << "\n"
             << "Order Type: " << limit_order.orderType << "\n"
             << "Total Quantity: " << decimalToString(limit_order.totalQuantity) << "\n"
             << "Limit Price: " << limit_order.lmtPrice << "\n";

  orderId_exchangesymbol_[m_orderId] = std::string(order->symbol_);
  orderId_to_conID[m_orderId] = inst_desc->token_;
  dbglogger_ << "SENDING ORDER FOR ORDER ID " << m_orderId << " CON ID " << inst_desc->token_ << " EXCHANGE SYMBOL "
             << orderId_exchangesymbol_[m_orderId] << "\n";

  orderId_to_OrderType[m_orderId] = kNewOrder;
  orderId_to_OrderConfNotif[m_orderId] = false;
  saos_notification_req[m_orderId] = true;  // Expecting a notification from TWS used for handling duplicates call backs
  orderId_to_saos_map_[m_orderId] = order->server_assigned_order_sequence_;
  saos_to_orderId_map_[order->server_assigned_order_sequence_] = m_orderId;
  permID_to_buy_sell_map_[m_orderId] = order->buysell_;
  orderId_to_size_remaining_[m_orderId] = order->size_remaining_;
  m_pClient->placeOrder(m_orderId++, contract, limit_order);
}

void CBOEEngine::ModifyOrder(ORS::Order* order, ORS::Order* orig_order) {
  dbglogger_ << "Func: " << __func__ << "\n";

  last_send_time_ = time(NULL);
  struct timeval tv;
  gettimeofday(&tv, NULL);

  dbglogger_ << "MODIFY ORDER FOR SAOS : " << order->server_assigned_order_sequence_ << "\n";
  dbglogger_ << "PLACING MODIFY LIMIT ORDER @ " << tv.tv_sec << "." << tv.tv_usec << "\n";

  InstrumentDesc* inst_desc = &container_.inst_desc_[order->security_id_];
  Contract contract = ContractSamples::USOptionContract();
  SetDynamicOrderEntryRequestFields(contract, std::to_string(inst_desc->expiry_date_), (double)inst_desc->strike_price_,
                                    std::string(1, inst_desc->option_type_[0]));

  std::string buy_sell = (order->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL";
  long order_size = order->size_remaining_;
  double price = (double)(order->price_);

  Order limit_order = OrderSamples::LimitOrder(buy_sell, stringToDecimal(std::to_string(order_size)), price);

  dbglogger_ << "buy_sell " << buy_sell << "\n";
  dbglogger_ << "order_size " << order_size << "\n";
  dbglogger_ << "price " << price << "\n";

  orderId_to_OrderType[saos_to_orderId_map_[order->server_assigned_order_sequence_]] = kModifyOrder;
  orderId_to_OrderModifyNotif[saos_to_orderId_map_[order->server_assigned_order_sequence_]] = false;
  orderId_to_size_remaining_[saos_to_orderId_map_[order->server_assigned_order_sequence_]] = order->size_remaining_;
  saos_notification_req[saos_to_orderId_map_[order->server_assigned_order_sequence_]] = true;
  m_pClient->placeOrder(saos_to_orderId_map_[order->server_assigned_order_sequence_], contract, limit_order);
}

void CBOEEngine::CancelOrder(ORS::Order* order) {
  dbglogger_ << "Func: " << __func__ << "\n";

  last_send_time_ = time(NULL);
  struct timeval tv;
  gettimeofday(&tv, NULL);

  dbglogger_ << "PLACING CANCEL LIMIT ORDER @ " << tv.tv_sec << "." << tv.tv_usec << "\n";
  dbglogger_ << "CANCELING ORDER WITH SAOS " << order->server_assigned_order_sequence_ << "\n";

  saos_notification_req[saos_to_orderId_map_[order->server_assigned_order_sequence_]] = true;
  m_pClient->cancelOrder(saos_to_orderId_map_[order->server_assigned_order_sequence_], "");
}

void CBOEEngine::SendSpreadOrder(HFSAT::ORS::Order* order1_, HFSAT::ORS::Order* order2_) {
  std::cout << "Actual Implemention of cancel order starts here\n";
}

void CBOEEngine::SendThreeLegOrder(HFSAT::ORS::Order* order1_, HFSAT::ORS::Order* order2_, HFSAT::ORS::Order* order3_) {
  std::cout << "Actual Implemention of three leg order starts here\n";
}

void CBOEEngine::SendTwoLegOrder(HFSAT::ORS::Order* order1_, HFSAT::ORS::Order* order2_) {
  std::cout << "Actual Implemention of two leg order starts here\n";
}

void CBOEEngine::ProcessTWSResponse() {
  time_t now = time(NULL);
  /*****************************************************************/
  /* Below are few quick-to-test examples on the IB API functions grouped by functionality. Uncomment the relevant
   * methods. */
  /*****************************************************************/
  switch (m_state) {
    case ST_PNLSINGLE:
      pnlSingleOperation();
      break;
    case ST_PNLSINGLE_ACK:
      break;
    case ST_PNL:
      pnlOperation();
      break;
    case ST_PNL_ACK:
      break;
    case ST_TICKDATAOPERATION:
      tickDataOperation();
      break;
    case ST_TICKDATAOPERATION_ACK:
      break;
    case ST_TICKOPTIONCOMPUTATIONOPERATION:
      tickOptionComputationOperation();
      break;
    case ST_TICKOPTIONCOMPUTATIONOPERATION_ACK:
      break;
    case ST_DELAYEDTICKDATAOPERATION:
      delayedTickDataOperation();
      break;
    case ST_DELAYEDTICKDATAOPERATION_ACK:
      break;
    case ST_MARKETDEPTHOPERATION:
      marketDepthOperations();
      break;
    case ST_MARKETDEPTHOPERATION_ACK:
      break;
    case ST_REALTIMEBARS:
      realTimeBars();
      break;
    case ST_REALTIMEBARS_ACK:
      break;
    case ST_MARKETDATATYPE:
      marketDataType();
      break;
    case ST_MARKETDATATYPE_ACK:
      break;
    case ST_HISTORICALDATAREQUESTS:
      historicalDataRequests();
      break;
    case ST_HISTORICALDATAREQUESTS_ACK:
      break;
    case ST_OPTIONSOPERATIONS:
      // optionsOperations();
      break;
    case ST_OPTIONSOPERATIONS_ACK:
      break;
    case ST_CONTRACTOPERATION:
      contractOperations();
      break;
    case ST_CONTRACTOPERATION_ACK:
      break;
    case ST_MARKETSCANNERS:
      marketScanners();
      break;
    case ST_MARKETSCANNERS_ACK:
      break;
    case ST_FUNDAMENTALS:
      fundamentals();
      break;
    case ST_FUNDAMENTALS_ACK:
      break;
    case ST_BULLETINS:
      bulletins();
      break;
    case ST_BULLETINS_ACK:
      break;
    case ST_ACCOUNTOPERATIONS:
      accountOperations();
      break;
    case ST_ACCOUNTOPERATIONS_ACK:
      break;
    case ST_ORDEROPERATIONS:
      orderOperations();
      break;
    case ST_ORDEROPERATIONS_ACK:
      break;
    case ST_OCASAMPLES:
      ocaSamples();
      break;
    case ST_OCASAMPLES_ACK:
      break;
    case ST_CONDITIONSAMPLES:
      conditionSamples();
      break;
    case ST_CONDITIONSAMPLES_ACK:
      break;
    case ST_BRACKETSAMPLES:
      bracketSample();
      break;
    case ST_BRACKETSAMPLES_ACK:
      break;
    case ST_HEDGESAMPLES:
      hedgeSample();
      break;
    case ST_HEDGESAMPLES_ACK:
      break;
    case ST_TESTALGOSAMPLES:
      testAlgoSamples();
      break;
    case ST_TESTALGOSAMPLES_ACK:
      break;
    case ST_FAORDERSAMPLES:
      financialAdvisorOrderSamples();
      break;
    case ST_FAORDERSAMPLES_ACK:
      break;
    case ST_FAOPERATIONS:
      financialAdvisorOperations();
      break;
    case ST_FAOPERATIONS_ACK:
      break;
    case ST_DISPLAYGROUPS:
      testDisplayGroups();
      break;
    case ST_DISPLAYGROUPS_ACK:
      break;
    case ST_MISCELANEOUS:
      miscelaneous();
      break;
    case ST_MISCELANEOUS_ACK:
      break;
    case ST_FAMILYCODES:
      reqFamilyCodes();
      break;
    case ST_FAMILYCODES_ACK:
      break;
    case ST_SYMBOLSAMPLES:
      reqMatchingSymbols();
      break;
    case ST_SYMBOLSAMPLES_ACK:
      break;
    case ST_REQMKTDEPTHEXCHANGES:
      reqMktDepthExchanges();
      break;
    case ST_REQMKTDEPTHEXCHANGES_ACK:
      break;
    case ST_REQNEWSTICKS:
      reqNewsTicks();
      break;
    case ST_REQNEWSTICKS_ACK:
      break;
    case ST_REQSMARTCOMPONENTS:
      reqSmartComponents();
      break;
    case ST_REQSMARTCOMPONENTS_ACK:
      break;
    case ST_NEWSPROVIDERS:
      reqNewsProviders();
      break;
    case ST_NEWSPROVIDERS_ACK:
      break;
    case ST_REQNEWSARTICLE:
      reqNewsArticle();
      break;
    case ST_REQNEWSARTICLE_ACK:
      break;
    case ST_REQHISTORICALNEWS:
      reqHistoricalNews();
      break;
    case ST_REQHISTORICALNEWS_ACK:
      break;
    case ST_REQHEADTIMESTAMP:
      reqHeadTimestamp();
      break;
    case ST_REQHISTOGRAMDATA:
      reqHistogramData();
      break;
    case ST_REROUTECFD:
      rerouteCFDOperations();
      break;
    case ST_MARKETRULE:
      marketRuleOperations();
      break;
    case ST_CONTFUT:
      continuousFuturesOperations();
      break;
    case ST_REQHISTORICALTICKS:
      reqHistoricalTicks();
      break;
    case ST_REQHISTORICALTICKS_ACK:
      break;
    case ST_REQTICKBYTICKDATA:
      reqTickByTickData();
      break;
    case ST_REQTICKBYTICKDATA_ACK:
      break;
    case ST_WHATIFSAMPLES:
      whatIfSamples();
      break;
    case ST_WHATIFSAMPLES_ACK:
      break;
    case ST_IBKRATSSAMPLE:
      ibkratsSample();
      break;
    case ST_IBKRATSSAMPLE_ACK:
      break;
    case ST_WSH:
      wshCalendarOperations();
      break;
    case ST_WSH_ACK:
      break;
    case ST_PING:
      reqCurrentTime();
      break;
    case ST_PING_ACK:
      if (m_sleepDeadline < now) {
        disconnect();
        return;
      }
      break;
    case ST_IDLE:
      if (m_sleepDeadline < now) {
        m_state = ST_PING;
        return;
      }
      break;
    default: { } break; }

  m_osSignal.waitForSignal();
  errno = 0;
  m_pReader->processMsgs();
}

void CBOEEngine::onInputAvailable() { ProcessTWSResponse(); }

void CBOEEngine::OnAddString(uint32_t num_sid) {
  std::cout << "ADDTS SETS HERE\n";

  SimpleSecuritySymbolIndexer& indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();
  std::string exchange_symbol = indexer.GetSecuritySymbolFromId(num_sid - 1);

  // Get data symbol. For cash market, data symbol is shortcode itself
  std::string data_symbol = cboe_sec_def_.ConvertExchSymboltoDataSourceName(exchange_symbol.c_str());
  if (std::string("INVALID") == data_symbol) {
    DBGLOG_CLASS_FUNC_LINE_FATAL << "FAILED TO ADD SYMBOL STRING AS DATASYMBOL WAS INVALID, SYMBOL : "
                                 << exchange_symbol << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;

    // Should never have reached here
    // exit(-1);
    return;
  }
  DBGLOG_CLASS_FUNC_LINE_INFO << "SecID: " << num_sid << " ExchSym: " << exchange_symbol << DBGLOG_ENDL_FLUSH;

  int32_t token = cboe_daily_token_symbol_handler_.GetTokenFromInternalSymbol(data_symbol.c_str(), cboe_segment_type_);
  char* instrument = ref_data_loader.GetCBOERefData(cboe_segment_type_)[token].instrument;

  char symbol[11];
  memset(symbol, ' ', sizeof(symbol));
  memcpy(symbol, ref_data_loader.GetCBOERefData(cboe_segment_type_)[token].symbol,
         strlen(ref_data_loader.GetCBOERefData(cboe_segment_type_)[token].symbol));
  symbol[10] = 0;

  char* option_type = ref_data_loader.GetCBOERefData(cboe_segment_type_)[token].option_type;
  int32_t strike_price = (int32_t)(std::round(ref_data_loader.GetCBOERefData(cboe_segment_type_)[token].strike_price));
  int32_t expiry = ref_data_loader.GetCBOERefData(cboe_segment_type_)[token].expiry;

  if (strcmp(option_type, "XX") == 0) {
    strike_price = -1;
    std::cout << "strike changed to -1 \n";
  }

  std::cout << "Token: " << token << " ExchSym: " << exchange_symbol
            << " RefData: " << ref_data_loader.GetCBOERefData(cboe_segment_type_)[token].ToString()
            << " Symbol: " << symbol << " Instrument: " << instrument << " Expiry: " << expiry
            << " Strike: " << strike_price << " Option_type: " << option_type << "\n"
            << std::endl;

  container_.inst_desc_[num_sid - 1].SetInstrumentDesc(token, instrument, symbol, expiry, strike_price, option_type);

  // Add to FastPxConverter map

  HFSAT::SecurityDefinitions& security_definitions_ =
      HFSAT::SecurityDefinitions::GetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());
  HFSAT::ShortcodeContractSpecificationMap& this_contract_specification_map_ =
      security_definitions_.contract_specification_map_;
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(HFSAT::DateTime::GetCurrentIsoDateLocal());

  for (auto itr_ = this_contract_specification_map_.begin(); itr_ != this_contract_specification_map_.end(); itr_++) {
    std::string shortcode_ = (itr_->first);
    if (itr_->second.exch_source_ != HFSAT::kExchSourceCBOE) {
      continue;
    }
    std::string this_symbol = cboe_sec_def_.GetExchSymbolCBOE(shortcode_);
    if (exchange_symbol == this_symbol) {
      // This is the corresponding security
      HFSAT::ContractSpecification& contract_spec_ = (itr_->second);
      if (fast_px_convertor_vec_[num_sid - 1] == NULL) {
        fast_px_convertor_vec_[num_sid - 1] = new HFSAT::FastPriceConvertor(contract_spec_.min_price_increment_);
        DBGLOG_CLASS_FUNC_LINE_INFO << " FAST PRICE MAP ADDED FOR SYMBOL : " << exchange_symbol
                                    << " MIN PRICE : " << contract_spec_.min_price_increment_ << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
      break;
    }
  }
}

void CBOEEngine::PreloadExchangeSymbol() {
  dbglogger_ << "PRE POPULATING CONID TO EXCHANGE SYMBOL MAP\n";

  // Retrieve the reference data map
  std::map<int32_t, CBOE_UDP_MDS::CBOERefData> fo_token_to_cboe_refdata_ =
      ref_data_loader.GetCBOERefData(cboe_segment_type_);
  dbglogger_ << "SIZE OF REF DATA MAP IS " << fo_token_to_cboe_refdata_.size() << "\n";

  for (auto itr = fo_token_to_cboe_refdata_.begin(); itr != fo_token_to_cboe_refdata_.end(); ++itr) {
    // dbglogger_ << "SEARCHING FOR EXCHANGE SYMBOL FOR CONTRACT ID " << itr->first << "\n";

    // Get the data source symbol
    std::string datasource_symbol =
        cboe_daily_token_symbol_handler_.GetInternalSymbolFromToken(itr->first, cboe_segment_type_);

    // Convert the data source name to exchange symbol
    std::string exchange_symbol = cboe_sec_def_.ConvertDataSourceNametoExchSymbol(datasource_symbol);

    // Check if the exchange symbol is valid
    if (exchange_symbol != "INVALID") {
      // Populate the map with the valid exchange symbol
      conID_to_exchangesymbol_[itr->first] = exchange_symbol;
      exchangesymbol_to_conID[exchange_symbol] = itr->first;

      // Log the information
      /*
      dbglogger_ << "Pre-Populating ConId: " << itr->first
                 << ", DataSource: " << datasource_symbol
                 << ", Exchange Symbol: " << exchange_symbol << "\n";
      */

    } else {
      // Log when the exchange symbol is invalid
      /*
       dbglogger_ << "ConId: " << itr->first
                 << " RETURN EXCHANGE SYMBOL: INVALID\n";
      */
    }
  }

  // dbglogger_ << "SIZE OF PREPOPULATED MAP IS " << conID_to_exchangesymbol_.size() << "\n";
  // dbglogger_ << "Contents of conID_to_exchangesymbol_ map:\n";

  // Log the contents of the populated map

  /*
  for (auto id = conID_to_exchangesymbol_.begin(); id != conID_to_exchangesymbol_.end(); id++) {
    dbglogger_ << "ConID: " << id->first << ", Exchange Symbol: " << id->second << "\n";
  }
  */
}

void CBOEEngine::thread_main() { std::cout << "Thread main\n"; }

void CBOEEngine::connectAck() {
  if (!m_extraAuth && m_pClient->asyncEConnect()) m_pClient->startApi();
}
//! [connectack]

// [nextvalidid]
void CBOEEngine::nextValidId(OrderId orderId) {
  m_orderId = orderId + 1;
  printf("Starting from Valid Id: %ld\n", m_orderId);
  dbglogger_ << "Next Valid Id  " << m_orderId << "\n";
}

void CBOEEngine::currentTime(long time) {
  if (m_state == ST_PING_ACK) {
    time_t t = (time_t)time;
    struct tm* timeinfo = localtime(&t);
    printf("The current date/time is: %s", asctime(timeinfo));

    time_t now = ::time(NULL);
    m_sleepDeadline = now + SLEEP_BETWEEN_PINGS;

    m_state = ST_PING_ACK;
  }
}

//! [error]
void CBOEEngine::error(int id, int errorCode, const std::string& errorString,
                       const std::string& advancedOrderRejectJson) {
  if (!advancedOrderRejectJson.empty()) {
    printf("Error. Id: %d, Code: %d, Msg: %s, AdvancedOrderRejectJson: %s\n", id, errorCode, errorString.c_str(),
           advancedOrderRejectJson.c_str());

    dbglogger_ << "Error. Id: " << id << ", Code: " << errorCode << ", Msg: " << errorString
               << ", AdvancedOrderRejectJson: " << advancedOrderRejectJson.c_str() << "\n";

  } else {
    printf("Error. Id: %d, Code: %d, Msg: %s\n", id, errorCode, errorString.c_str());

    dbglogger_ << "Error. Id: " << id << ", Code: " << errorCode << ", Msg: " << errorString << "\n";
  }

  auto reject_saos = orderId_to_saos_map_.find(id);

  if (reject_saos != orderId_to_saos_map_.end()) {
    dbglogger_ << "ORDER REJECTED FOR ORDER ID " << id << " SAOS : " << orderId_to_saos_map_[id] << "\n";
    dbglogger_ << "REJECT CODE :  " << errorCode << " REJECT REASON " << errorString << "\n";
    dbglogger_ << "INFORMING LISTENERS ACCOUNT THREAD\n";
    p_engine_listener_->OnReject(orderId_to_saos_map_[id]);
  } else {
    dbglogger_ << "ORDER REJECTED FOR ORDER ID " << id << " SAOS NOT AVAILABLE\n";
  }

  switch (errorCode) {
    case 1100:  // Connection lost
      std::cerr << "Error 1100: TWS connection lost. " << errorString << std::endl;
      dbglogger_ << "Error 1100: TWS connection lost. " << errorString << "\n";
      break;

    case 1101:  // Reconnecting
      std::cerr << "Error 1101: TWS attempting to reconnect. " << errorString << std::endl;
      dbglogger_ << "Error 1101: TWS attempting to reconnect. " << errorString << "\n";
      break;

    case 1102:  // Reconnected
      std::cerr << "Error 1102: TWS reconnected successfully. " << errorString << std::endl;
      dbglogger_ << "Error 1102: TWS reconnected successfully.  "
                 << "\n";

      dbglogger_ << "Handling Reconnection "
                 << "\n";
      std::cout << "Handling Reconnection " << std::endl;

      std::cout << "Calling Open Order now " << std::endl;
      dbglogger_ << "Calling Open Order now "
                 << "\n";

      //m_pClient->reqOpenOrders();

      std::cout << "Calling Open Order completed " << std::endl;
      dbglogger_ << "Calling Open Order completed\n";

      std::cout << "Calling for executed orders " << std::endl;
      dbglogger_ << "Calling for executed orders\n";
      dbglogger_ << "SLEEP FOR 3 SEC\n";
      std::this_thread::sleep_for(std::chrono::seconds(3));

      handle_reconnection = true;
      dbglogger_ << "HANDLE RECONNECTION " << handle_reconnection << "\n";

      dbglogger_ << "ExecutionFilter Details:\n";
      dbglogger_ << "  Client ID:       " << filter.m_clientId << "\n";
      dbglogger_ << "  Account:         " << filter.m_acctCode << "\n";
      dbglogger_ << "  Time:            " << filter.m_time << "\n";
      dbglogger_ << "  Symbol:          " << filter.m_symbol << "\n";
      dbglogger_ << "  Security Type:   " << filter.m_secType << "\n";
      dbglogger_ << "  Exchange:        " << filter.m_exchange << "\n";
      dbglogger_ << "  Side:            " << filter.m_side << "\n";

      //m_pClient->reqExecutions(0,filter);

      std::cout << "Calling for executed orders completed" << std::endl;
      dbglogger_ << "Calling for executed orders completed\n";

      break;

    default:
      std::cerr << "Error " << errorCode << ": " << errorString << std::endl;
      dbglogger_ << "Error " << errorCode << ": " << errorString << "\n";
      break;
  }
}
//! [error]

void CBOEEngine::RequestExecutedOrders() {
  dbglogger_ << "ExecutionFilter Details:\n";
  dbglogger_ << "Client ID:       " << filter.m_clientId << "\n";
  dbglogger_ << "Account:         " << filter.m_acctCode << "\n";
  dbglogger_ << "Time:            " << filter.m_time << "\n";
  dbglogger_ << "Symbol:          " << filter.m_symbol << "\n";
  dbglogger_ << "Security Type:   " << filter.m_secType << "\n";
  dbglogger_ << "Exchange:        " << filter.m_exchange << "\n";
  dbglogger_ << "Side:            " << filter.m_side << "\n";

  dbglogger_ << "Calling for RequestExecutedOrders\n";
  m_pClient->reqExecutions(0, ExecutionFilter());
  dbglogger_ << "Calling for RequestExecutedOrders completed\n";
}

//! [tickprice]
void CBOEEngine::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attribs) {
  printf("Tick Price. Ticker Id: %ld, Field: %d, Price: %s, CanAutoExecute: %d, PastLimit: %d, PreOpen: %d\n", tickerId,
         (int)field, doubleMaxString(price).c_str(), attribs.canAutoExecute, attribs.pastLimit, attribs.preOpen);
}
//! [tickprice]

//! [ticksize]
void CBOEEngine::tickSize(TickerId tickerId, TickType field, Decimal size) {
  printf("Tick Size. Ticker Id: %ld, Field: %d, Size: %s\n", tickerId, (int)field,
         decimalStringToDisplay(size).c_str());
}
//! [ticksize]

//! [tickoptioncomputation]
void CBOEEngine::tickOptionComputation(TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol,
                                       double delta, double optPrice, double pvDividend, double gamma, double vega,
                                       double theta, double undPrice) {
  printf(
      "TickOptionComputation. Ticker Id: %ld, Type: %d, TickAttrib: %s, ImpliedVolatility: %s, Delta: %s, OptionPrice: "
      "%s, pvDividend: %s, Gamma: %s, Vega: %s, Theta: %s, Underlying Price: %s\n",
      tickerId, (int)tickType, intMaxString(tickAttrib).c_str(), doubleMaxString(impliedVol).c_str(),
      doubleMaxString(delta).c_str(), doubleMaxString(optPrice).c_str(), doubleMaxString(pvDividend).c_str(),
      doubleMaxString(gamma).c_str(), doubleMaxString(vega).c_str(), doubleMaxString(theta).c_str(),
      doubleMaxString(undPrice).c_str());
}
//! [tickoptioncomputation]

//! [tickgeneric]
void CBOEEngine::tickGeneric(TickerId tickerId, TickType tickType, double value) {
  printf("Tick Generic. Ticker Id: %ld, Type: %d, Value: %s\n", tickerId, (int)tickType,
         doubleMaxString(value).c_str());
}
//! [tickgeneric]

//! [tickstring]
void CBOEEngine::tickString(TickerId tickerId, TickType tickType, const std::string& value) {
  printf("Tick String. Ticker Id: %ld, Type: %d, Value: %s\n", tickerId, (int)tickType, value.c_str());
}
//! [tickstring]

void CBOEEngine::tickEFP(TickerId tickerId, TickType tickType, double basisPoints,
                         const std::string& formattedBasisPoints, double totalDividends, int holdDays,
                         const std::string& futureLastTradeDate, double dividendImpact,
                         double dividendsToLastTradeDate) {
  printf(
      "TickEFP. %ld, Type: %d, BasisPoints: %s, FormattedBasisPoints: %s, Total Dividends: %s, HoldDays: %s, Future "
      "Last Trade Date: %s, Dividend Impact: %s, Dividends To Last Trade Date: %s\n",
      tickerId, (int)tickType, doubleMaxString(basisPoints).c_str(), formattedBasisPoints.c_str(),
      doubleMaxString(totalDividends).c_str(), intMaxString(holdDays).c_str(), futureLastTradeDate.c_str(),
      doubleMaxString(dividendImpact).c_str(), doubleMaxString(dividendsToLastTradeDate).c_str());
}

void CBOEEngine::processCBOEImmediateExecutionResponse(OrderId orderId, int permId, double avgFillPrice, Decimal filled,
                                                       Decimal remaining, OrderType_t order_type) {
  dbglogger_ << "Func: " << __func__ << "\n";

  dbglogger_ << "Order Request type is " << order_type << "\n";
  dbglogger_ << "Order Request type is " << orderTypeToString(order_type) << "\n";

  auto conId_itr = permID_to_conId_.find(permId);

  if (conId_itr == permID_to_conId_.end()) {
    dbglogger_ << "processCBOEImmediateExecutionResponse PRE SUBMITTED CALLBACK NOT RECEIVED FOR ORDER ID " << orderId
               << "\n";
    dbglogger_ << "Checking for contract id in orderId_to_conID map\n";

    conId_itr = orderId_to_conID.find((int64_t)orderId);

    if (conId_itr == orderId_to_conID.end()) {
      dbglogger_ << "CON ID NOT PRESENT IN orderId_to_conID MAP ALSO RETURNING\n";
      return;
    } else {
      dbglogger_ << "CON ID PRESENT IN orderId_to_conID MAP PROCESSING\n";
      dbglogger_ << "CON ID: " << conId_itr->second << "\n";
      permID_to_conId_[permId] = conId_itr->second;
    }
  }

  auto internal_symbol = token_to_internal_exchange_symbol_.find(conId_itr->second);

  if (internal_symbol == token_to_internal_exchange_symbol_.end()) {
    std::string datasource_symbol =
        cboe_daily_token_symbol_handler_.GetInternalSymbolFromToken(conId_itr->second, cboe_segment_type_);
    token_to_internal_exchange_symbol_[conId_itr->second] =
        cboe_sec_def_.ConvertDataSourceNametoExchSymbol(datasource_symbol);
    dbglogger_ << "Adding ConId " << conId_itr->second << " DataSource " << datasource_symbol << " Exchange Symbol "
               << cboe_sec_def_.ConvertDataSourceNametoExchSymbol(datasource_symbol) << "\n";
  } else {
    dbglogger_ << "Exchange symbol for " << conId_itr->second << " already exists " << internal_symbol->second << "\n";
  }

  // @ We can also use a unordered_map of ConID to Exchange symbol to avoid latency issues as it limit map size to
  // specific number.
  permID_to_exchangesymbol_[permId] =
      token_to_internal_exchange_symbol_[static_cast<int32_t>(conId_itr->second)].c_str();

  dbglogger_ << "Entry Added --> PERM_ID  " << permId << " Exchange symbol "
             << token_to_internal_exchange_symbol_[static_cast<int32_t>(conId_itr->second)].c_str() << "\n";

  dbglogger_ << "Contents of token_to_internal_exchange_symbol_\n";
  for (const auto& pair : token_to_internal_exchange_symbol_) {
    std::cout << "Token: " << pair.first << ", Exchange Symbol: " << pair.second << std::endl;
    dbglogger_ << "Token: " << pair.first << ", Exchange Symbol: " << pair.second << "\n";
  }

  dbglogger_ << "Contents of permID_to_exchangesymbol_:"
             << "\n";

  for (std::tr1::unordered_map<int32_t, const char*>::const_iterator it = permID_to_exchangesymbol_.begin();
       it != permID_to_exchangesymbol_.end(); ++it) {
    dbglogger_ << "permID: " << it->first << ", Exchange Symbol: " << it->second << "\n";
  }

  if (order_type == kNewOrder) {
    // Now simulating OnOrderConf
    dbglogger_ << "SIMULATING ONORDER CONF CALLBACK\n";

    dbglogger_ << "OnOrderConf Arguments:\n"
               << "OrderId: " << orderId << "\n"
               << "StringArg: "
               << ""
               << "\n"
               << "AvgFillPrice: " << avgFillPrice << "\n"
               << "Filled: " << filled << "\n"
               << "ZeroArg: " << 0 << "\n"
               << "PermId: " << permId << "\n"
               << "Arg6: " << -1 << "\n"
               << "Arg7: " << -1 << "\n";

    int64_t order_confirmed_size = (int32_t)filled + (int32_t)remaining;
    p_engine_listener_->OnOrderConf(orderId_to_saos_map_[orderId], "", (double)avgFillPrice,
                                    (int32_t)order_confirmed_size, 0, permId, -1, -1);

    dbglogger_ << "processCBOEImmediateExecutionResponse OnOrderConf simulation completed\n";
  } else if (order_type == kModifyOrder) {
    // Now simulating OnOrderCancelReplaced
    dbglogger_ << "SIMULATING OnOrderCancelReplaced CALLBACK\n";

    dbglogger_ << "OnOrderCancelReplaced Arguments:\n"
               << "OrderId: " << orderId << "\n"
               << "StringArg: "
               << ""
               << "\n"
               << "AvgFillPrice: " << avgFillPrice << "\n"
               << "Filled: " << filled << "\n"
               << "ZeroArg: " << 0 << "\n"
               << "PermId: " << permId << "\n"
               << "Arg6: " << -1 << "\n"
               << "Arg7: " << -1 << "\n";

    p_engine_listener_->OnOrderCancelReplaced(orderId_to_saos_map_[orderId], permId, (double)avgFillPrice,
                                              (int32_t)remaining, fast_px_convertor_vec_, -1, -1);

    dbglogger_ << "processCBOEImmediateExecutionResponse OnOrderCancelReplaced simulation completed\n";
  }

  dbglogger_ << "NOW SIMULATING ON ORDER EXEC\n";

  dbglogger_ << "OnOrderExec Arguments: \n"
             << "OrderExec: " << orderId_to_saos_map_[orderId] << "\n"
             << "ExchangeSymbol: " << permID_to_exchangesymbol_[permId] << "\n"
             << "Buy/Sell: " << permID_to_buy_sell_map_[orderId] << "\n"
             << "AvgFillPrice: " << avgFillPrice << "\n"
             << "Filled: " << filled << "\n"
             << "Remaining: " << remaining << "\n"
             << "PermId: " << permId << "\n"
             << "Arg8: " << 0 << "\n"
             << "Arg9: " << 0 << "\n";

  p_engine_listener_->OnOrderExec(orderId_to_saos_map_[orderId], permID_to_exchangesymbol_[permId],
                                  permID_to_buy_sell_map_[orderId], (double)avgFillPrice,
                                  orderId_to_lastFilledAmount[orderId], (int)remaining, permId, 0, 0);

  dbglogger_ << "NOW SIMULATING ON ORDER EXEC COMPLETED\n";

  // Handing for removing from map

  dbglogger_ << "REMAINING ORDER SIZE IN STRING " << decimalStringToDisplay(remaining) << "\n";

  if (std::strcmp(decimalStringToDisplay(remaining).c_str(), "0") == 0) {
    dbglogger_ << "IMMEDIATE FUNC FULL ORDER EXECUTION NOTIFICATION/CALLBACK RECEIVED FOR ORDER ID " << orderId
               << " PERM_ID " << permId << " REMAINING SIZE " << decimalStringToDisplay(remaining) << "\n";
    dbglogger_ << "REMOVING FROM exch_order_num_to_saos_ & other maps to avoid duplicate call backs\n";
    // Check and erase for std::map or std::unordered_map

    if (exch_order_num_to_saos_.find(permId) != exch_order_num_to_saos_.end()) {
      exch_order_num_to_saos_.erase(permId);
    }

    if (permID_to_order_.find(permId) != permID_to_order_.end()) {
      permID_to_order_.erase(permId);
    }

    if (permIDPool.find(permId) != permIDPool.end()) {
      permIDPool.erase(permId);
    }

    if (saos_notification_req.find(orderId) != saos_notification_req.end()) {
      saos_notification_req.erase(orderId);
    }

    if (permID_to_buy_sell_map_.find(orderId) != permID_to_buy_sell_map_.end()) {
      permID_to_buy_sell_map_.erase(orderId);
    }

    if (orderId_to_permId_.find(orderId) != orderId_to_permId_.end()) {
      orderId_to_permId_.erase(orderId);
    }

  } else {
    dbglogger_ << "IMMEDIATE FUNC PARTIAL ORDER EXECUTION REMAINING ORDER SIZE " << decimalStringToDisplay(remaining)
               << " FILLED ORDER SIZE " << decimalStringToDisplay(filled) << "\n";

    // To do Fill all the map's required now
    saos_notification_req[orderId] =
        false;  // As we don't require OrderConf,OrderModify,OrderCancel any more request now
    // permID_to_order_ Left Empty  // Assumed to be filled in OpenOrder callback
    // permID_to_exchangesymbol_ Already filled above
    permIDPool.insert(permId);  // Useful to check whether to pass OrderConf or OrderModify to listners
    // orderId_to_saos_map_ Already filled in SendOrder
    // saos_to_orderId_map_ Already filled in SendOrder
    // permID_to_conId_ Already filled above
    // permID_to_buy_sell_map_ Filled in SendOrder
    // conID_to_exchangesymbol_ Filled in Constructor call
    // exchangesymbol_to_conID Filled in Constructor call
    // orderId_to_conID Filled in SendOrder
    // orderId_exchangesymbol_ Filled in SendOrder
    orderId_to_permId_[orderId] = permId;
  }

  return;
}

void CBOEEngine::RequestORSPNL() {
  dbglogger_ << "Requesting for PNL "
             << "\n";
  dbglogger_ << "PNL REQUEST FOR REQID " << reqId_ << "\n";
  std::string modelCode = "";                             // Optional: specify model code if applicable
  m_pClient->reqPnL(reqId_++, accountsList_, modelCode);  // Request PnL for the account

  return;
}

void CBOEEngine::RequestORSOpenPositions() {
  dbglogger_ << "Requesting for TWS ORS Open postions "
             << "\n";
  m_pClient->reqPositions();  // Request for open positions
  dbglogger_ << "Requesting for TWS ORS Open postions completed "
             << "\n";

  return;
}

void CBOEEngine::KillSwitch(int32_t sec_id) {
  dbglogger_ << "Requesting for TWS Cancel all order "
             << "\n";

  if (-1 == sec_id) {
    dbglogger_ << "KILLSWITCH COMMAND RECEIVED CANCELLING ALL THE ORDERS\n";
    dbglogger_ << "PLACING REQUEST TO CANCEL ALL OPEN ORDER AT GLOBAL LEVEL\n";
    // Iterate over the map and set all values to true
    for (auto& pair : saos_notification_req) {
      pair.second = true;  // Set the value to true
    }
    m_pClient->reqGlobalCancel();
    dbglogger_ << "PLACING REQUEST TO CANCEL ALL OPEN ORDER AT GLOBAL LEVEL COMPLETED\n";
  } else {
    SimpleSecuritySymbolIndexer& indexer = SimpleSecuritySymbolIndexer::GetUniqueInstance();
    std::string exchange_symbol = indexer.GetSecuritySymbolFromId(sec_id);

    dbglogger_ << "CANCELLING ALL PENDING ORDERS FOR SEC ID : " << sec_id << " Exchange Symbol : " << exchange_symbol
               << "\n";
    // Iterate over the map
    for (const auto& pair : orderId_exchangesymbol_) {
      std::int64_t orderId = pair.first;
      const std::string& symbol = pair.second;

      // Check if the exchange_symbol matches
      if (symbol == exchange_symbol) {
        saos_notification_req[orderId] =
            true;  // Expecting a cancel notification from TWS used for handling duplicates call backs
        dbglogger_ << "CANCELLING ORDER FOR ORDER ID : " << orderId << " SAOS : " << orderId_to_saos_map_[orderId]
                   << "\n";
        m_pClient->cancelOrder(orderId, "");  // Cancel the order
      }
    }

    dbglogger_ << "CANCELLING ALL PENDING ORDERS FOR SEC ID : " << sec_id << " Exchange Symbol : " << exchange_symbol
               << " COMPLETED"
               << "\n";
  }

  dbglogger_ << "Requesting for TWS Cancel all order completed "
             << "\n";

  return;
}

void CBOEEngine::FetchMarginUsage() {
  dbglogger_ << "CBOEEngine::FetchMarginUsage\n";
  m_pClient->reqAccountUpdates(true, accountsList_);
}

std::string CBOEEngine::getCurrentDateTime() {
  // Get current time from system clock
  auto now = std::chrono::system_clock::now();

  // Convert current time to time_t for formatting
  std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

  // Convert to local time
  std::tm localTime = *std::localtime(&currentTime);

  // Format time as "YYYY-MM-DD HH:MM:SS"
  std::stringstream ss;
  ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

  return ss.str();
}

void CBOEEngine::orderStatus(OrderId orderId, const std::string& status, Decimal filled, Decimal remaining,
                             double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId,
                             const std::string& whyHeld, double mktCapPrice) {
  dbglogger_ << "Func: " << __func__ << "\n";

  printf(
      "OrderStatus. Id: %ld, Status: %s, Filled: %s, Remaining: %s, AvgFillPrice: %s, PermId: %s, LastFillPrice: %s, "
      "ClientId: %s, WhyHeld: %s, MktCapPrice: %s\n",
      orderId, status.c_str(), decimalStringToDisplay(filled).c_str(), decimalStringToDisplay(remaining).c_str(),
      doubleMaxString(avgFillPrice).c_str(), intMaxString(permId).c_str(), doubleMaxString(lastFillPrice).c_str(),
      intMaxString(clientId).c_str(), whyHeld.c_str(), doubleMaxString(mktCapPrice).c_str());

  dbglogger_ << "OrderStatus. Id: " << orderId << ", Status: " << status
             << ", Filled: " << decimalStringToDisplay(filled) << ", Remaining: " << decimalStringToDisplay(remaining)
             << ", AvgFillPrice: " << doubleMaxString(avgFillPrice) << ", PermId: " << intMaxString(permId)
             << ", LastFillPrice: " << doubleMaxString(lastFillPrice) << ", ClientId: " << intMaxString(clientId)
             << ", WhyHeld: " << whyHeld << ", MktCapPrice: " << doubleMaxString(mktCapPrice) << "\n";

  auto current_session_order = saos_notification_req.find(orderId);

  dbglogger_ << "SAOS NOTIFICATION RECIEVED FOR ORDER ID " << orderId << "\n";
  if (saos_notification_req.end() == current_session_order) {
    dbglogger_ << "NOT A CURRENT SESSION ORDER OR ORDER IS ALREADY EXECUTED AT CBOE EXCHANGE NO FURTHER PROCESSING "
                  "REQUIRED RETURNING\n";
    return;
  }

  // @ Assuming PermId to be TWS assigned order id which remains unique to entire day
  // @ Assuming orderId to the seq number that we have assigned while sending the order
  // @ https://groups.io/g/twsapi/topic/orderid_permid_parentid/77894406#

  // std::cout << "orderId " << orderId << " PermId " << intMaxString(order.permId).c_str() <<
  // " order.orderId " << order.orderId << std::endl;

  dbglogger_ << "ORDER STATUS NOTIFICATION/CALLBACK RECEIVED FOR ORDER ID " << orderId << " PERM_ID " << permId
             << " STATUS " << status << "\n";
  dbglogger_ << "SAOS " << orderId << "  NOTIFICATION EXPECTED  " << saos_notification_req[orderId] << "\n";
  DBGLOG_DUMP;

  if ((true == saos_notification_req[orderId]) && !strcmp(status.c_str(), "Submitted")) {
    // Below check is required to handle same updates/call_backs
    dbglogger_ << "ORDER ID " << orderId << " ORDER STATUS " << status << "\n";
    saos_notification_req[orderId] = false;

    auto order_info = permID_to_order_.find(permId);

    if (order_info == permID_to_order_.end()) {
      dbglogger_ << "WEIRD ORDER CONFIRMATION\n";
      dbglogger_ << "OPEN ORDER CALLBACK NOT RECIEVED FOR ORDER ID " << orderId << " PERM ID " << permId
                 << " ORDER STATUS " << status << " RETURNING";
      return;
    }

    dbglogger_ << "Checking if it is a new order\n";
    auto order_exists = permIDPool.find(permId);

    if (order_exists == permIDPool.end()) {
      std::cout << "NEW ORDER CONFIRMATION\n" << std::flush;
      dbglogger_ << "NEW ORDER CONFIRMATION\n";
      permIDPool.insert(permId);
      orderId_to_OrderConfNotif[orderId] = true;
      p_engine_listener_->OnOrderConf(orderId_to_saos_map_[orderId], "", (double)order_info->second.lmtPrice,
                                      (int32_t)order_info->second.totalQuantity, 0, permId, -1, -1);

      exch_order_num_to_saos_[permId] = orderId;  // Seems to be vague can be removed
    } else {
      std::cout << "MODIFY ORDER CONFIRMATION\n" << std::flush;
      dbglogger_ << "MODIFY ORDER CONFIRMATION\n";
      orderId_to_OrderModifyNotif[orderId] = true;
      p_engine_listener_->OnOrderCancelReplaced(orderId_to_saos_map_[orderId], permId,
                                                (double)order_info->second.lmtPrice, (int32_t)remaining,
                                                fast_px_convertor_vec_, -1, -1);
    }
  } else if ((true == saos_notification_req[orderId]) && !strcmp(status.c_str(), "Cancelled")) {
    // Below check is required to handle same updates/call_backs
    saos_notification_req[orderId] = false;
    std::cout << "order Cancelled\n" << std::flush;
    dbglogger_ << "Cancel Order Confirmation\n";
    auto order_info = permID_to_order_.find(permId);

    if (order_info == permID_to_order_.end()) {
      dbglogger_ << "WEIRD ORDER CANCELATION CONFIRMATION\n";
      dbglogger_ << "OPEN ORDER CALLBACK NOT RECIEVED FOR ORDER ID " << orderId << " PERM ID " << permId
                 << " ORDER STATUS " << status << " RETURNING";
      return;
    }

    p_engine_listener_->OnOrderCxl(orderId_to_saos_map_[orderId], permId);

    // Check and erase for std::map or std::unordered_map
    if (saos_notification_req.find(orderId) != saos_notification_req.end()) {
      saos_notification_req.erase(orderId);
    }

    if (exch_order_num_to_saos_.find(permId) != exch_order_num_to_saos_.end()) {
      exch_order_num_to_saos_.erase(permId);
    }

    if (permIDPool.find(permId) != permIDPool.end()) {
      permIDPool.erase(permId);
    }

    if (permID_to_order_.find(permId) != permID_to_order_.end()) {
      permID_to_order_.erase(permId);
    }

    if (permID_to_buy_sell_map_.find(orderId) != permID_to_buy_sell_map_.end()) {
      permID_to_buy_sell_map_.erase(orderId);
    }

    if (orderId_to_permId_.find(orderId) != orderId_to_permId_.end()) {
      orderId_to_permId_.erase(orderId);
    }

    // To do add handling to remove orderId_to_saos_map_ & saos_to_orderId_map_ entry also

  } else if (!strcmp(status.c_str(), "Filled")) {
    // Below check can be removed as we have already checked earlier
    auto order_filled = saos_notification_req.find(orderId);

    if (order_filled == saos_notification_req.end()) {
      dbglogger_ << "ORDER ALREADY EXECUTED FROM CBOE EXCHANGE END RETURNING TO AVOID DUPLICATE CALLBACK TO ON EXEC "
                    "LISTNERS (Account Thread))\n";
      return;
    }

    std::cout << "Order Filled\n" << std::flush;
    std::cout << "Trade perm id " << permId << "\n" << std::flush;

    dbglogger_ << "Order/Trade Filled for " << orderId << "\n";
    dbglogger_ << "Trade PermId " << permId << "\n";

    dbglogger_ << "Checking the OrderType Sent\n";

    auto order_req_type = orderId_to_OrderType.find(orderId);

    if (order_req_type == orderId_to_OrderType.end()) {
      dbglogger_ << "NO ORDER REQUEST FOR ORDER ID IN orderId_to_OrderType " << orderId << " RETURNING\n";
      return;
    }

    OrderType_t order_type = orderId_to_OrderType[orderId];

    if (order_type == kNewOrder) {
      dbglogger_ << "NEW ORDER WAS REQUESTED IN LAST ORDER TYPE\n";
      dbglogger_ << "Checking if New Order Confirmation Notification was received\n";

      auto new_order_conf_notified = orderId_to_OrderConfNotif.find(orderId);

      if (new_order_conf_notified == orderId_to_OrderConfNotif.end()) {
        dbglogger_ << "ORDER STATUS: FILLED ORDER ID DOESN'T EXISTS orderId_to_OrderConfNotif " << orderId
                   << " RETURNING "
                   << "\n";
        return;
      }

      bool notification_val = orderId_to_OrderConfNotif[orderId];

      if (false == notification_val) {
        orderId_to_OrderConfNotif[orderId] = true;
        dbglogger_ << "NEW ORDER CONFIRMATION NOTIFICATION WAS NOT SENT AS SUBMITTED CALLBACK WAS NOT RECEIVED\n";
        dbglogger_ << "HANDLING NEW ORDER CONFIRMATION\n";
        // @ CASE OF IMMEDIATE ORDER EXECUTION SIMULATING ON ORDER CONF
        processCBOEImmediateExecutionResponse(orderId, permId, avgFillPrice, filled, remaining, kNewOrder);
        dbglogger_ << "processCBOEImmediateExecutionResponse Completed returning now\n";
        return;

      } else {
        dbglogger_ << "Order Status Submitted was received for Order ID: " << orderId << "\n";
        dbglogger_ << "Processing normal on order exec\n";
      }

    } else if (order_type == kModifyOrder) {
      dbglogger_ << "MODIFY ORDER WAS REQUESTED IN LAST ORDER TYPE\n";
      dbglogger_ << "Checking if Modify Order Confirmation Notification was received\n";

      auto modify_order_conf_notified = orderId_to_OrderModifyNotif.find(orderId);

      if (modify_order_conf_notified == orderId_to_OrderModifyNotif.end()) {
        dbglogger_ << "ORDER STATUS: FILLED ORDER ID DOESN'T EXISTS orderId_to_OrderModifyNotif " << orderId
                   << " RETURNING "
                   << "\n";
          return;
      }

      bool notification_val = orderId_to_OrderModifyNotif[orderId];

      if (false == notification_val) {
        orderId_to_OrderModifyNotif[orderId] = true;
        dbglogger_ << "MODIFY ORDER CONFIRMATION NOTIFICATION WAS NOT SENT AS SUBMITTED CALLBACK WAS NOT RECEIVED\n";
        dbglogger_ << "HANDLING MODIFY ORDER CONFIRMATION\n";
        // @ CASE OF IMMEDIATE ORDER EXECUTION SIMULATING ON ORDER MODIFY CONF
        processCBOEImmediateExecutionResponse(orderId, permId, avgFillPrice, filled,
                                              remaining, kModifyOrder);
        dbglogger_ << "processCBOEImmediateExecutionResponse Completed returning now\n";
        return;
      } else {
        dbglogger_ << "Order Status Submitted was received for Order ID: " << orderId << "\n";
        dbglogger_ << "Processing normal on order exec\n";
      }
    }

    auto order_info = permID_to_order_.find(permId);
    if (order_info == permID_to_order_.end()) {
      dbglogger_ << "ORDER DOESN'T EXISTS IN permID_to_order_ MAP BUT TRADE NOTIFICATION FILLED RECIEVED\n";
      return;
    }

    std::cout << "Trade confirmaton for Exchange symbol is "
              << std::string(permID_to_exchangesymbol_[order_info->second.permId]) << std::endl
              << std::flush;
    dbglogger_ << "Trade confirmaton for Exchange symbol is "
               << std::string(permID_to_exchangesymbol_[order_info->second.permId]) << "\n";

    dbglogger_ << "Trade confirmaton for Exchange symbol is " << permID_to_exchangesymbol_[order_info->second.permId]
               << "\n";

    TradeType_t buy_sell = strcmp(order_info->second.action.c_str(), "BUY") == 0 ? kTradeTypeBuy : kTradeTypeSell;

    std::cout << order_info->second.action << " " << buy_sell << std::endl << std::flush;
    dbglogger_ << order_info->second.action << " " << buy_sell << "\n";

    dbglogger_ << "LOGGING permID_to_exchangesymbol_ MAP inside Order Filled callback\n";

    for (std::tr1::unordered_map<int32_t, const char*>::const_iterator it = permID_to_exchangesymbol_.begin();
         it != permID_to_exchangesymbol_.end(); ++it) {
      std::cout << "permID: " << it->first << ", Exchange Symbol: " << it->second << std::endl;
      dbglogger_ << "permID: " << it->first << ", Exchange Symbol: " << it->second << "\n";
    }

    dbglogger_ << "I TRADE NOTIFICATION PERM ID " << order_info->second.permId << " Exchange Symbol "
               << permID_to_exchangesymbol_[order_info->second.permId] << "\n";
    dbglogger_ << "II TRADE NOTIFICATION PERM ID " << order_info->second.permId << " Exchange Symbol "
               << std::string(permID_to_exchangesymbol_[order_info->second.permId]) << "\n";

    p_engine_listener_->OnOrderExec(orderId_to_saos_map_[orderId], permID_to_exchangesymbol_[order_info->second.permId],
                                    buy_sell, avgFillPrice, orderId_to_lastFilledAmount[orderId], remaining, permId, 0,
                                    0);

    dbglogger_ << "REMAINING ORDER SIZE IN STRING " << decimalStringToDisplay(remaining) << "\n";

    if (std::strcmp(decimalStringToDisplay(remaining).c_str(), "0") == 0) {
      dbglogger_ << "FULL ORDER EXECUTION NOTIFICATION/CALLBACK RECEIVED FOR ORDER ID " << orderId << " PERM_ID "
                 << permId << " STATUS " << status << " REMAINING SIZE " << decimalStringToDisplay(remaining) << "\n";
      dbglogger_ << "REMOVING FROM exch_order_num_to_saos_ map to avoid duplicate call back\n";
      // Check and erase for std::map or std::unordered_map
      if (saos_notification_req.find(orderId) != saos_notification_req.end()) {
        saos_notification_req.erase(orderId);
      }

      if (exch_order_num_to_saos_.find(permId) != exch_order_num_to_saos_.end()) {
        exch_order_num_to_saos_.erase(permId);
      }

      if (permIDPool.find(permId) != permIDPool.end()) {
        permIDPool.erase(permId);
      }

      if (permID_to_order_.find(permId) != permID_to_order_.end()) {
        permID_to_order_.erase(permId);
      }

      if (permID_to_buy_sell_map_.find(orderId) != permID_to_buy_sell_map_.end()) {
        permID_to_buy_sell_map_.erase(orderId);
      }

      if (orderId_to_permId_.find(orderId) != orderId_to_permId_.end()) {
        orderId_to_permId_.erase(orderId);
      }

    } else {
      dbglogger_ << "PARTIAL ORDER EXECUTION REMAINING ORDER SIZE " << decimalStringToDisplay(remaining)
                 << " FILLED ORDER SIZE " << decimalStringToDisplay(filled) << "\n";
    }
  }

  std::cout << "\n\n\n\n" << std::flush;
  dbglogger_ << "\n\n\n\n";
}

//! [orderstatus]

//! [openorder]
void CBOEEngine::openOrder(OrderId orderId, const Contract& contract, const Order& order,
                           const OrderState& orderState) {
  dbglogger_ << "Func : " << __func__ << "\n";

  printf(
      "OpenOrder. PermId: %s, ClientId: %s, OrderId: %s, Account: %s, Symbol: %s, SecType: %s, Exchange: %s:, Action: "
      "%s, OrderType:%s, TotalQty: %s, CashQty: %s, "
      "LmtPrice: %s, AuxPrice: %s, Status: %s, MinTradeQty: %s, MinCompeteSize: %s, CompeteAgainstBestOffset: %s, "
      "MidOffsetAtWhole: %s, MidOffsetAtHalf: %s\n",
      intMaxString(order.permId).c_str(), longMaxString(order.clientId).c_str(), longMaxString(orderId).c_str(),
      order.account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.exchange.c_str(),
      order.action.c_str(), order.orderType.c_str(), decimalStringToDisplay(order.totalQuantity).c_str(),
      doubleMaxString(order.cashQty).c_str(), doubleMaxString(order.lmtPrice).c_str(),
      doubleMaxString(order.auxPrice).c_str(), orderState.status.c_str(), intMaxString(order.minTradeQty).c_str(),
      intMaxString(order.minCompeteSize).c_str(),
      order.competeAgainstBestOffset == COMPETE_AGAINST_BEST_OFFSET_UP_TO_MID
          ? "UpToMid"
          : doubleMaxString(order.competeAgainstBestOffset).c_str(),
      doubleMaxString(order.midOffsetAtWhole).c_str(), doubleMaxString(order.midOffsetAtHalf).c_str());

  dbglogger_ << "OpenOrder. PermId: " << intMaxString(order.permId) << ", ClientId: " << longMaxString(order.clientId)
             << ", OrderId: " << longMaxString(orderId) << ", Account: " << order.account
             << ", Symbol: " << contract.symbol << ", SecType: " << contract.secType
             << ", Exchange: " << contract.exchange << ", Action: " << order.action
             << ", OrderType: " << order.orderType << ", TotalQty: " << decimalStringToDisplay(order.totalQuantity)
             << ", CashQty: " << doubleMaxString(order.cashQty) << ", LmtPrice: " << doubleMaxString(order.lmtPrice)
             << ", AuxPrice: " << doubleMaxString(order.auxPrice) << ", Status: " << orderState.status
             << ", MinTradeQty: " << intMaxString(order.minTradeQty)
             << ", MinCompeteSize: " << intMaxString(order.minCompeteSize) << ", CompeteAgainstBestOffset: "
             << (order.competeAgainstBestOffset == COMPETE_AGAINST_BEST_OFFSET_UP_TO_MID
                     ? "UpToMid"
                     : doubleMaxString(order.competeAgainstBestOffset))
             << ", MidOffsetAtWhole: " << doubleMaxString(order.midOffsetAtWhole)
             << ", MidOffsetAtHalf: " << doubleMaxString(order.midOffsetAtHalf) << "\n";

  permID_to_conId_[order.permId] = contract.conId;  // Todo Can we update this map at the time of sending of order can
                                                    // be useful in case of pre-submitted notification not recieved
  orderId_to_permId_[orderId] = order.permId;

  if (orderId_exchangesymbol_.find(orderId) != orderId_exchangesymbol_.end())
    dbglogger_ << "PREEMPTIVE EXCHANGE SYMBOL FOR ORDER ID : " << orderId
               << "  EXCHANGE SYMBOL : " << std::string(orderId_exchangesymbol_[orderId]) << "\n";

  if (!strcmp(orderState.status.c_str(), "Submitted")) {
    dbglogger_ << "ORDER ACKNOWLEDGED\n";
    std::cout << "ORDER ACKNOWLEDGED\n";
    std::cout << "INSERTING/UPDATING PERMID STATUS " << order.permId << "\n" << std::flush;
    dbglogger_ << "INSERTING/UPDATING PERMID  STATUS " << order.permId << "\n";
    std::cout << "Con ID " << contract.conId << "\n" << std::flush;
    dbglogger_ << "Con ID " << contract.conId << "\n";

    permID_to_order_[order.permId] = order;
    auto internal_symbol = token_to_internal_exchange_symbol_.find(contract.conId);

    if (internal_symbol == token_to_internal_exchange_symbol_.end()) {
      std::string datasource_symbol =
          cboe_daily_token_symbol_handler_.GetInternalSymbolFromToken(contract.conId, cboe_segment_type_);
      token_to_internal_exchange_symbol_[contract.conId] =
          cboe_sec_def_.ConvertDataSourceNametoExchSymbol(datasource_symbol);
      dbglogger_ << "Adding ConId " << contract.conId << " DataSource " << datasource_symbol << " Exchange Symbol "
                 << cboe_sec_def_.ConvertDataSourceNametoExchSymbol(datasource_symbol) << "\n";
    } else {
      dbglogger_ << "Exchange symbol for " << contract.conId << " already exists " << internal_symbol->second << "\n";
    }

    // @ We can also use a unordered_map of ConID to Exchange symbol to avoid latency issues as it limit map size to
    // specific number.
    permID_to_exchangesymbol_[order.permId] =
        token_to_internal_exchange_symbol_[static_cast<int32_t>(contract.conId)].c_str();

    dbglogger_ << "Entry Added --> PERM_ID  " << order.permId << " Exchange symbol "
               << token_to_internal_exchange_symbol_[static_cast<int32_t>(contract.conId)].c_str() << "\n";

    dbglogger_ << "Contents of token_to_internal_exchange_symbol_\n";
    for (const auto& pair : token_to_internal_exchange_symbol_) {
      std::cout << "Token: " << pair.first << ", Exchange Symbol: " << pair.second << std::endl;
      dbglogger_ << "Token: " << pair.first << ", Exchange Symbol: " << pair.second << "\n";
    }

    dbglogger_ << "Contents of permID_to_exchangesymbol_:"
               << "\n";

    for (std::tr1::unordered_map<int32_t, const char*>::const_iterator it = permID_to_exchangesymbol_.begin();
         it != permID_to_exchangesymbol_.end(); ++it) {
      dbglogger_ << "permID: " << it->first << ", Exchange Symbol: " << it->second << "\n";
    }
  } else if (!strcmp(orderState.status.c_str(), "Filled")) {
    dbglogger_ << "OpenOrder. filled notification received\n";
    dbglogger_ << "Checking if permId exists in permID_to_order_ map\n";

    auto order_info = permID_to_order_.find(order.permId);

    if (order_info == permID_to_order_.end()) {
      permID_to_order_[order.permId] = order;
      dbglogger_ << "permId: " << order.permId << " DOES'NT EXISTS IN MAP ADDDED NOW\n";
    } else {
      dbglogger_ << "permId: " << order.permId << " ALREADY EXISTS IN MAP NOW\n";
    }
  }

  std::cout << "\n\n\n\n" << std::flush;
  dbglogger_ << "\n\n\n\n";
}
//! [openorder]

//! [openorderend]
void CBOEEngine::openOrderEnd() { printf("OpenOrderEnd\n"); }
//! [openorderend]

void CBOEEngine::winError(const std::string& str, int lastError) {}
void CBOEEngine::connectionClosed() { printf("Connection Closed\n"); }

//! [updateaccountvalue]
void CBOEEngine::updateAccountValue(const std::string& key, const std::string& val, const std::string& currency,
                                    const std::string& accountName) {
  printf("UpdateAccountValue. Key: %s, Value: %s, Currency: %s, Account Name: %s\n", key.c_str(), val.c_str(),
         currency.c_str(), accountName.c_str());

  if (key == "InitMarginReq") {
    HFSAT::Utils::HTMLGenerator::updInitMarginReq(val);
    dbglogger_ << "Initial Margin Requirement (Gross): " << val << " " << currency << "\n";
  } else if (key == "MaintMarginReq") {
    HFSAT::Utils::HTMLGenerator::updMaintMarginReq(val);
    dbglogger_ << "Maintenance Margin Requirement (Gross): " << val << " " << currency << "\n";
  } else if (key == "NetLiquidation") {
    HFSAT::Utils::HTMLGenerator::updNetLiquidation(val);
    dbglogger_ << "Net Liquidation Value: " << val << " " << currency << "\n";
  } else if (key == "AvailableFunds") {
    HFSAT::Utils::HTMLGenerator::updAvailableFunds(val);
    dbglogger_ << "Available Funds (Net): " << val << " " << currency << "\n";
  } else if (key == "GrossPositionValue") {
    HFSAT::Utils::HTMLGenerator::updGrossPositionValue(val);
    dbglogger_ << "Gross Position Value: " << val << " " << currency << "\n";
  }
  // HFSAT::Utils::HTMLGenerator::updateMarginDetails(initMarginReq,);

  m_pClient->reqAccountUpdates(false, accountsList_);
}
//! [updateaccountvalue]

//! [updateportfolio]
void CBOEEngine::updatePortfolio(const Contract& contract, Decimal position, double marketPrice, double marketValue,
                                 double averageCost, double unrealizedPNL, double realizedPNL,
                                 const std::string& accountName) {
  printf(
      "UpdatePortfolio. %s, %s @ %s: Position: %s, MarketPrice: %s, MarketValue: %s, AverageCost: %s, UnrealizedPNL: "
      "%s, RealizedPNL: %s, AccountName: %s\n",
      (contract.symbol).c_str(), (contract.secType).c_str(), (contract.primaryExchange).c_str(),
      decimalStringToDisplay(position).c_str(), doubleMaxString(marketPrice).c_str(),
      doubleMaxString(marketValue).c_str(), doubleMaxString(averageCost).c_str(),
      doubleMaxString(unrealizedPNL).c_str(), doubleMaxString(realizedPNL).c_str(), accountName.c_str());

  dbglogger_ << "UpdatePortfolio. Symbol: " << contract.symbol << ", SecType: " << contract.secType
             << ", PrimaryExchange: " << contract.primaryExchange << ", Position: " << decimalStringToDisplay(position)
             << ", MarketPrice: " << doubleMaxString(marketPrice) << ", MarketValue: " << doubleMaxString(marketValue)
             << ", AverageCost: " << doubleMaxString(averageCost)
             << ", UnrealizedPNL: " << doubleMaxString(unrealizedPNL)
             << ", RealizedPNL: " << doubleMaxString(realizedPNL) << ", AccountName: " << accountName << "\n";
  std::string exchange_symbol = "";
  auto exchange_symbol_itr = conID_to_exchangesymbol_.find(contract.conId);
  exchange_symbol =
      (exchange_symbol_itr != conID_to_exchangesymbol_.end()) ? conID_to_exchangesymbol_[contract.conId] : "INVALID";

  if(use_data_over_html){
    HFSAT::Utils::HTMLGenerator::addOrUpdateRow(
        contract.conId, accountName, contract.symbol, contract.secType, contract.currency,
        decimalStringToDisplay(position), doubleMaxString(averageCost),
        exchange_symbol, cboe_sec_def_.GetShortCodeFromExchangeSymbol(exchange_symbol),
        doubleMaxString(marketPrice) ,doubleMaxString(marketValue),
        doubleMaxString(unrealizedPNL),doubleMaxString(realizedPNL)  
    );
  }
}
//! [updateportfolio]

//! [updateaccounttime]
void CBOEEngine::updateAccountTime(const std::string& timeStamp) {
  printf("UpdateAccountTime. Time: %s\n", timeStamp.c_str());
}
//! [updateaccounttime]

//! [accountdownloadend]
void CBOEEngine::accountDownloadEnd(const std::string& accountName) {
  printf("Account download finished: %s\n", accountName.c_str());
}
//! [accountdownloadend]

//! [contractdetails]
void CBOEEngine::contractDetails(int reqId, const ContractDetails& contractDetails) {
  printf("ContractDetails begin. ReqId: %d\n", reqId);
  printContractMsg(contractDetails.contract);
  printContractDetailsMsg(contractDetails);
  printf("ContractDetails end. ReqId: %d\n", reqId);
}
//! [contractdetails]

//! [bondcontractdetails]
void CBOEEngine::bondContractDetails(int reqId, const ContractDetails& contractDetails) {
  printf("BondContractDetails begin. ReqId: %d\n", reqId);
  printBondContractDetailsMsg(contractDetails);
  printf("BondContractDetails end. ReqId: %d\n", reqId);
}
//! [bondcontractdetails]

void CBOEEngine::printContractMsg(const Contract& contract) {
  printf("\tConId: %ld\n", contract.conId);
  printf("\tSymbol: %s\n", contract.symbol.c_str());
  printf("\tSecType: %s\n", contract.secType.c_str());
  printf("\tLastTradeDateOrContractMonth: %s\n", contract.lastTradeDateOrContractMonth.c_str());
  printf("\tStrike: %s\n", doubleMaxString(contract.strike).c_str());
  printf("\tRight: %s\n", contract.right.c_str());
  printf("\tMultiplier: %s\n", contract.multiplier.c_str());
  printf("\tExchange: %s\n", contract.exchange.c_str());
  printf("\tPrimaryExchange: %s\n", contract.primaryExchange.c_str());
  printf("\tCurrency: %s\n", contract.currency.c_str());
  printf("\tLocalSymbol: %s\n", contract.localSymbol.c_str());
  printf("\tTradingClass: %s\n", contract.tradingClass.c_str());
}

void CBOEEngine::printContractDetailsMsg(const ContractDetails& contractDetails) {
  printf("\tMarketName: %s\n", contractDetails.marketName.c_str());
  printf("\tMinTick: %s\n", doubleMaxString(contractDetails.minTick).c_str());
  printf("\tPriceMagnifier: %s\n", longMaxString(contractDetails.priceMagnifier).c_str());
  printf("\tOrderTypes: %s\n", contractDetails.orderTypes.c_str());
  printf("\tValidExchanges: %s\n", contractDetails.validExchanges.c_str());
  printf("\tUnderConId: %s\n", intMaxString(contractDetails.underConId).c_str());
  printf("\tLongName: %s\n", contractDetails.longName.c_str());
  printf("\tContractMonth: %s\n", contractDetails.contractMonth.c_str());
  printf("\tIndystry: %s\n", contractDetails.industry.c_str());
  printf("\tCategory: %s\n", contractDetails.category.c_str());
  printf("\tSubCategory: %s\n", contractDetails.subcategory.c_str());
  printf("\tTimeZoneId: %s\n", contractDetails.timeZoneId.c_str());
  printf("\tTradingHours: %s\n", contractDetails.tradingHours.c_str());
  printf("\tLiquidHours: %s\n", contractDetails.liquidHours.c_str());
  printf("\tEvRule: %s\n", contractDetails.evRule.c_str());
  printf("\tEvMultiplier: %s\n", doubleMaxString(contractDetails.evMultiplier).c_str());
  printf("\tAggGroup: %s\n", intMaxString(contractDetails.aggGroup).c_str());
  printf("\tUnderSymbol: %s\n", contractDetails.underSymbol.c_str());
  printf("\tUnderSecType: %s\n", contractDetails.underSecType.c_str());
  printf("\tMarketRuleIds: %s\n", contractDetails.marketRuleIds.c_str());
  printf("\tRealExpirationDate: %s\n", contractDetails.realExpirationDate.c_str());
  printf("\tLastTradeTime: %s\n", contractDetails.lastTradeTime.c_str());
  printf("\tStockType: %s\n", contractDetails.stockType.c_str());
  printf("\tMinSize: %s\n", decimalStringToDisplay(contractDetails.minSize).c_str());
  printf("\tSizeIncrement: %s\n", decimalStringToDisplay(contractDetails.sizeIncrement).c_str());
  printf("\tSuggestedSizeIncrement: %s\n", decimalStringToDisplay(contractDetails.suggestedSizeIncrement).c_str());
  printContractDetailsSecIdList(contractDetails.secIdList);
}

void CBOEEngine::printContractDetailsSecIdList(const TagValueListSPtr& secIdList) {
  const int secIdListCount = secIdList.get() ? secIdList->size() : 0;
  if (secIdListCount > 0) {
    printf("\tSecIdList: {");
    for (int i = 0; i < secIdListCount; ++i) {
      const TagValue* tagValue = ((*secIdList)[i]).get();
      printf("%s=%s;", tagValue->tag.c_str(), tagValue->value.c_str());
    }
    printf("}\n");
  }
}

void CBOEEngine::printBondContractDetailsMsg(const ContractDetails& contractDetails) {
  printf("\tSymbol: %s\n", contractDetails.contract.symbol.c_str());
  printf("\tSecType: %s\n", contractDetails.contract.secType.c_str());
  printf("\tCusip: %s\n", contractDetails.cusip.c_str());
  printf("\tCoupon: %s\n", doubleMaxString(contractDetails.coupon).c_str());
  printf("\tMaturity: %s\n", contractDetails.maturity.c_str());
  printf("\tIssueDate: %s\n", contractDetails.issueDate.c_str());
  printf("\tRatings: %s\n", contractDetails.ratings.c_str());
  printf("\tBondType: %s\n", contractDetails.bondType.c_str());
  printf("\tCouponType: %s\n", contractDetails.couponType.c_str());
  printf("\tConvertible: %s\n", contractDetails.convertible ? "yes" : "no");
  printf("\tCallable: %s\n", contractDetails.callable ? "yes" : "no");
  printf("\tPutable: %s\n", contractDetails.putable ? "yes" : "no");
  printf("\tDescAppend: %s\n", contractDetails.descAppend.c_str());
  printf("\tExchange: %s\n", contractDetails.contract.exchange.c_str());
  printf("\tCurrency: %s\n", contractDetails.contract.currency.c_str());
  printf("\tMarketName: %s\n", contractDetails.marketName.c_str());
  printf("\tTradingClass: %s\n", contractDetails.contract.tradingClass.c_str());
  printf("\tConId: %s\n", longMaxString(contractDetails.contract.conId).c_str());
  printf("\tMinTick: %s\n", doubleMaxString(contractDetails.minTick).c_str());
  printf("\tOrderTypes: %s\n", contractDetails.orderTypes.c_str());
  printf("\tValidExchanges: %s\n", contractDetails.validExchanges.c_str());
  printf("\tNextOptionDate: %s\n", contractDetails.nextOptionDate.c_str());
  printf("\tNextOptionType: %s\n", contractDetails.nextOptionType.c_str());
  printf("\tNextOptionPartial: %s\n", contractDetails.nextOptionPartial ? "yes" : "no");
  printf("\tNotes: %s\n", contractDetails.notes.c_str());
  printf("\tLong Name: %s\n", contractDetails.longName.c_str());
  printf("\tEvRule: %s\n", contractDetails.evRule.c_str());
  printf("\tEvMultiplier: %s\n", doubleMaxString(contractDetails.evMultiplier).c_str());
  printf("\tAggGroup: %s\n", intMaxString(contractDetails.aggGroup).c_str());
  printf("\tMarketRuleIds: %s\n", contractDetails.marketRuleIds.c_str());
  printf("\tTimeZoneId: %s\n", contractDetails.timeZoneId.c_str());
  printf("\tLastTradeTime: %s\n", contractDetails.lastTradeTime.c_str());
  printf("\tMinSize: %s\n", decimalStringToDisplay(contractDetails.minSize).c_str());
  printf("\tSizeIncrement: %s\n", decimalStringToDisplay(contractDetails.sizeIncrement).c_str());
  printf("\tSuggestedSizeIncrement: %s\n", decimalStringToDisplay(contractDetails.suggestedSizeIncrement).c_str());
  printContractDetailsSecIdList(contractDetails.secIdList);
}

//! [contractdetailsend]
void CBOEEngine::contractDetailsEnd(int reqId) { printf("ContractDetailsEnd. %d\n", reqId); }
//! [contractdetailsend]

//! [execdetails]
void CBOEEngine::execDetails(int reqId, const Contract& contract, const Execution& execution) {
  dbglogger_ << "Function name: " << __func__ << "\n";
  dbglogger_ << "Order Executed at CBOE Exchange "
             << "\n";

  printf("ExecDetails. ReqId: %d - %s, %s, %s - %s, %s, %s, %s, %s\n", reqId, contract.symbol.c_str(),
         contract.secType.c_str(), contract.currency.c_str(), execution.execId.c_str(),
         longMaxString(execution.orderId).c_str(), decimalStringToDisplay(execution.shares).c_str(),
         decimalStringToDisplay(execution.cumQty).c_str(), intMaxString(execution.lastLiquidity).c_str());

  dbglogger_ << "ExecDetails. ReqId: " << reqId << " - Symbol: " << contract.symbol << ", SecType: " << contract.secType
             << ", Currency: " << contract.currency << " - ExecId: " << execution.execId
             << ", OrderId: " << longMaxString(execution.orderId)
             << ", Shares: " << decimalStringToDisplay(execution.shares)
             << ", CumQty: " << decimalStringToDisplay(execution.cumQty)
             << ", LastLiquidity: " << intMaxString(execution.lastLiquidity) << ", FillPrice : " << execution.price
             << "\n";
  
  if(use_data_over_html){
    std::string exchange_symbol = "";
    auto exchange_symbol_itr = conID_to_exchangesymbol_.find(contract.conId);
    exchange_symbol =
        (exchange_symbol_itr != conID_to_exchangesymbol_.end()) ? conID_to_exchangesymbol_[contract.conId] : "INVALID";

    HFSAT::Utils::HTMLGenerator::addOrUpdateExecutionRow(execution.orderId,execution.time,reqId,contract.symbol,
                                            contract.secType, contract.currency,
                                            execution.execId, decimalStringToDisplay(execution.shares),
                                            decimalStringToDisplay(execution.cumQty), intMaxString(execution.lastLiquidity),
                                            execution.price,cboe_sec_def_.GetShortCodeFromExchangeSymbol(exchange_symbol));
    return;
  }

  // Checking if orders exists in saos_notification_req map
  auto order_exists = saos_notification_req.find(execution.orderId);

  if (saos_notification_req.end() == order_exists) {
    // Case 0 : // Trade notification was already received so we can directly ignore this & return
    dbglogger_ << "OrderId: " << longMaxString(execution.orderId)
               << " ORDER ID IS NOT PRESENT IN saos_notification_req ORDER IS ALREADY EXECUTED & TRADE NOTIFICATION IS ALREADY RECEIVED\n";
    return;
  }

  orderId_to_lastFilledAmount[execution.orderId] = (uint64_t)execution.shares;
  // Check if we need to pass trade notification to account thread listener in case of disconnect

  //orderId_to_lastFilledAmount[execution.orderId] = (uint64_t)execution.shares;
  if (true == handle_reconnection) {
    dbglogger_ << "HANDLING ALL THE EXECUTIONS OF CURRENT SESSION AS TWS-IBKR CONNECTION WAS DISCONNECTED\n";
    dbglogger_ << "HANDLING TRADE NOTIFICATION FOR  OrderId: " << longMaxString(execution.orderId);

    std::vector<std::string> execId_vec = orderExecutions[execution.orderId];
    if (std::find(execId_vec.begin(), execId_vec.end(), execution.execId) != execId_vec.end()) {
      dbglogger_ << "HANDLE RECONNECTION = TRUE Exec ID " << execution.execId
                 << " IS ALREADY NOTIFIED BEFORE RETURNING\n";
      return;
    } else {
      dbglogger_ << "HANDLE RECONNECTION = TRUE Exec ID " << execution.execId << " IS NOT NOTIFIED\n";

      dbglogger_ << "Check 1:  PERMID PRESENCE\n";
      
      auto permID_itr = orderId_to_permId_.find(execution.orderId);
      if (permID_itr == orderId_to_permId_.end()) {
        dbglogger_ << "PERM ID NOT PRESENT FOR ORDER ID : " << longMaxString(execution.orderId) << " REVERTING BACK\n";
        return;
      }
      std::int64_t permID = orderId_to_permId_[execution.orderId];


      dbglogger_ << "Check 2:  ORDER REQUEST MESSAGE PRESENCE\n";
      dbglogger_ << "Checking the last sent Order Request message\n";
      auto order_req_type = orderId_to_OrderType.find(execution.orderId);
      if (order_req_type == orderId_to_OrderType.end()) {
        dbglogger_ << "NO ORDER REQUEST FOR ORDER ID " << execution.orderId << " RETURNING\n";
        return;
      }


      orderId_to_lastFilledAmount[execution.orderId] = (uint64_t)execution.shares;
      OrderType_t order_type = orderId_to_OrderType[execution.orderId];

      dbglogger_ << "ExecDetails. Order Request type is " << orderTypeToString(order_type) << "\n";

      if (order_type == kNewOrder) {
        dbglogger_ << "NEW ORDER WAS REQUESTED IN LAST ORDER TYPE\n";
        dbglogger_ << "Checking if New Order Confirmation Notification was received\n";

        auto new_order_conf_notified = orderId_to_OrderConfNotif.find(execution.orderId);

        if (new_order_conf_notified == orderId_to_OrderConfNotif.end()) {
          dbglogger_ << "ExecDetails ORDER ID DOESN'T EXISTS orderId_to_OrderConfNotif " << execution.orderId
                     << " RETURNING "
                     << "\n";
          return;
        }

        bool notification_val = orderId_to_OrderConfNotif[execution.orderId];
        orderId_to_size_remaining_[execution.orderId]-=(uint64_t)execution.shares;

        int64_t size_remaining =
            (orderId_to_size_remaining_[execution.orderId] <= 0) ? 0 : orderId_to_size_remaining_[execution.orderId];

        orderId_to_size_remaining_[execution.orderId]=size_remaining;

        if (false == notification_val) {
          orderId_to_OrderConfNotif[execution.orderId] = true;
          dbglogger_ << "NEW ORDER CONFIRMATION NOTIFICATION WAS NOT SENT AS SUBMITTED CALLBACK WAS NOT RECEIVED\n";
          dbglogger_ << "HANDLING NEW ORDER CONFIRMATION\n";
          // @ CASE OF IMMEDIATE ORDER EXECUTION SIMULATING ON ORDER CONF

          processCBOEImmediateExecutionResponse(execution.orderId, permID, (double)execution.avgPrice, execution.shares,
                                                size_remaining, kNewOrder);
          dbglogger_ << "processCBOEImmediateExecutionResponse Completed returning now\n";
          return;

        } else {
          dbglogger_ << "CBOEEngine::execDetails Order Status Submitted was received for Order ID: "
                     << execution.orderId << "\n";

          p_engine_listener_->OnOrderExec(orderId_to_saos_map_[execution.orderId], permID_to_exchangesymbol_[permID],
                                          permID_to_buy_sell_map_[execution.orderId], (double)execution.price,
                                          (uint64_t)execution.shares, size_remaining, permID, 0, 0);
        }

      } else if (order_type == kModifyOrder) {
        dbglogger_ << "MODIFY ORDER WAS REQUESTED IN LAST ORDER TYPE\n";
        dbglogger_ << "Checking if Modify Order Confirmation Notification was received\n";

        auto modify_order_conf_notified = orderId_to_OrderModifyNotif.find(execution.orderId);

        if (modify_order_conf_notified == orderId_to_OrderModifyNotif.end()) {
          dbglogger_ << "ORDER STATUS: FILLED ORDER ID DOESN'T EXISTS orderId_to_OrderModifyNotif " << execution.orderId
                     << " RETURNING "
                     << "\n";
        }

        orderId_to_size_remaining_[execution.orderId]-=(uint64_t)execution.shares;
        bool notification_val = orderId_to_OrderModifyNotif[execution.orderId];
        uint64_t size_remaining =
            (orderId_to_size_remaining_[execution.orderId] <= 0) ? 0 : orderId_to_size_remaining_[execution.orderId];

        orderId_to_size_remaining_[execution.orderId]=size_remaining;

        if (false == notification_val) {
          orderId_to_OrderModifyNotif[execution.orderId] = true;
          dbglogger_ << "MODIFY ORDER CONFIRMATION NOTIFICATION WAS NOT SENT AS SUBMITTED CALLBACK WAS NOT RECEIVED\n";
          dbglogger_ << "HANDLING MODIFY ORDER CONFIRMATION\n";
          // @ CASE OF IMMEDIATE ORDER EXECUTION SIMULATING ON ORDER MODIFY CONF
          processCBOEImmediateExecutionResponse(execution.orderId, permID, (double)execution.avgPrice, execution.shares,
                                                size_remaining, kModifyOrder);
          dbglogger_ << "processCBOEImmediateExecutionResponse Completed returning now\n";
          return;
        } else {
          dbglogger_ << "Order Status Submitted was received for Order ID: " << execution.orderId << "\n";
          p_engine_listener_->OnOrderExec(orderId_to_saos_map_[execution.orderId], permID_to_exchangesymbol_[permID],
                                          permID_to_buy_sell_map_[execution.orderId], (double)execution.price,
                                          (uint64_t)execution.shares, size_remaining, permID, 0, 0);
        }
      }
    }
  } else {
    dbglogger_ << "Order Executed at CBOE Exchange & TWS & IBKR connection is up"
               << "\n";

    std::vector<std::string> execId_vec = orderExecutions[execution.orderId];
    if (std::find(execId_vec.begin(), execId_vec.end(), execution.execId) != execId_vec.end()) {
      dbglogger_ << "HANDLE RECONNECTION = FALSE Exec ID " << execution.execId
                 << " IS ALREADY NOTIFIED BEFORE RETURNING\n";
      return;
    }

    orderId_to_lastFilledAmount[execution.orderId] = (uint64_t)execution.shares;
    orderExecutions[execution.orderId].push_back(execution.execId);
    orderId_to_size_remaining_[execution.orderId] -= (uint64_t)execution.shares;

    if (orderId_to_size_remaining_[execution.orderId] <= 0) {
      dbglogger_ << "CBOEEngine::execDetails All orders executed for ORDER ID : " << longMaxString(execution.orderId)
                 << "\n";
      orderId_to_size_remaining_.erase(execution.orderId);
    }
  }
}

//! [execdetails]

//! [execdetailsend]
void CBOEEngine::execDetailsEnd(int reqId) {
  dbglogger_ << "reqExecutions request completed\n";
  dbglogger_ << "REQ ID " << reqId << "\n";
  handle_reconnection = false;

  dbglogger_ << "DISABLING HANDLING NOW\n";
  dbglogger_ << "HANDLE RECONNECTION " << handle_reconnection << "\n";

  printf("ExecDetailsEnd. %d\n", reqId);
}
//! [execdetailsend]

//! [updatemktdepth]
void CBOEEngine::updateMktDepth(TickerId id, int position, int operation, int side, double price, Decimal size) {
  printf("UpdateMarketDepth. %ld - Position: %s, Operation: %d, Side: %d, Price: %s, Size: %s\n", id,
         intMaxString(position).c_str(), operation, side, doubleMaxString(price).c_str(),
         decimalStringToDisplay(size).c_str());
}
//! [updatemktdepth]

//! [updatemktdepthl2]
void CBOEEngine::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation, int side,
                                  double price, Decimal size, bool isSmartDepth) {
  printf("UpdateMarketDepthL2. %ld - Position: %s, Operation: %d, Side: %d, Price: %s, Size: %s, isSmartDepth: %d\n",
         id, intMaxString(position).c_str(), operation, side, doubleMaxString(price).c_str(),
         decimalStringToDisplay(size).c_str(), isSmartDepth);
}
//! [updatemktdepthl2]

//! [updatenewsbulletin]
void CBOEEngine::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage,
                                    const std::string& originExch) {
  printf("News Bulletins. %d - Type: %d, Message: %s, Exchange of Origin: %s\n", msgId, msgType, newsMessage.c_str(),
         originExch.c_str());
}
//! [updatenewsbulletin]

//! [managedaccounts]
void CBOEEngine::managedAccounts(const std::string& accountsList) {
  printf("Account List: %s\n", accountsList.c_str());
  accountsList_ = accountsList;
  dbglogger_ << "Account List: " << accountsList.c_str() << "\n";
}
//! [managedaccounts]

//! [receivefa]
void CBOEEngine::receiveFA(faDataType pFaDataType, const std::string& cxml) {
  std::cout << "Receiving FA: " << (int)pFaDataType << std::endl << cxml << std::endl;
}
//! [receivefa]

//! [historicaldata]
void CBOEEngine::historicalData(TickerId reqId, const Bar& bar) {
  printf(
      "HistoricalData. ReqId: %ld - Date: %s, Open: %s, High: %s, Low: %s, Close: %s, Volume: %s, Count: %s, WAP: %s\n",
      reqId, bar.time.c_str(), doubleMaxString(bar.open).c_str(), doubleMaxString(bar.high).c_str(),
      doubleMaxString(bar.low).c_str(), doubleMaxString(bar.close).c_str(), decimalStringToDisplay(bar.volume).c_str(),
      intMaxString(bar.count).c_str(), decimalStringToDisplay(bar.wap).c_str());
}
//! [historicaldata]

//! [historicaldataend]
void CBOEEngine::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) {
  std::cout << "HistoricalDataEnd. ReqId: " << reqId << " - Start Date: " << startDateStr
            << ", End Date: " << endDateStr << std::endl;
}
//! [historicaldataend]

//! [scannerparameters]
void CBOEEngine::scannerParameters(const std::string& xml) { printf("ScannerParameters. %s\n", xml.c_str()); }
//! [scannerparameters]

//! [scannerdata]
void CBOEEngine::scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance,
                             const std::string& benchmark, const std::string& projection, const std::string& legsStr) {
  printf(
      "ScannerData. %d - Rank: %d, Symbol: %s, SecType: %s, Currency: %s, Distance: %s, Benchmark: %s, Projection: %s, "
      "Legs String: %s\n",
      reqId, rank, contractDetails.contract.symbol.c_str(), contractDetails.contract.secType.c_str(),
      contractDetails.contract.currency.c_str(), distance.c_str(), benchmark.c_str(), projection.c_str(),
      legsStr.c_str());
}
//! [scannerdata]

//! [scannerdataend]
void CBOEEngine::scannerDataEnd(int reqId) { printf("ScannerDataEnd. %d\n", reqId); }
//! [scannerdataend]

//! [realtimebar]
void CBOEEngine::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
                             Decimal volume, Decimal wap, int count) {
  printf("RealTimeBars. %ld - Time: %s, Open: %s, High: %s, Low: %s, Close: %s, Volume: %s, Count: %s, WAP: %s\n",
         reqId, longMaxString(time).c_str(), doubleMaxString(open).c_str(), doubleMaxString(high).c_str(),
         doubleMaxString(low).c_str(), doubleMaxString(close).c_str(), decimalStringToDisplay(volume).c_str(),
         intMaxString(count).c_str(), decimalStringToDisplay(wap).c_str());
}
//! [realtimebar]

//! [fundamentaldata]
void CBOEEngine::fundamentalData(TickerId reqId, const std::string& data) {
  printf("FundamentalData. ReqId: %ld, %s\n", reqId, data.c_str());
}
//! [fundamentaldata]

void CBOEEngine::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) {
  printf("DeltaNeutralValidation. %d, ConId: %ld, Delta: %s, Price: %s\n", reqId, deltaNeutralContract.conId,
         doubleMaxString(deltaNeutralContract.delta).c_str(), doubleMaxString(deltaNeutralContract.price).c_str());
}

//! [ticksnapshotend]
void CBOEEngine::tickSnapshotEnd(int reqId) { printf("TickSnapshotEnd: %d\n", reqId); }
//! [ticksnapshotend]

//! [marketdatatype]
void CBOEEngine::marketDataType(TickerId reqId, int marketDataType) {
  printf("MarketDataType. ReqId: %ld, Type: %d\n", reqId, marketDataType);
}
//! [marketdatatype]

//! [commissionreport]
void CBOEEngine::commissionReport(const CommissionReport& commissionReport) {
  printf("CommissionReport. %s - %s %s RPNL %s\n", commissionReport.execId.c_str(),
         doubleMaxString(commissionReport.commission).c_str(), commissionReport.currency.c_str(),
         doubleMaxString(commissionReport.realizedPNL).c_str());
}
//! [commissionreport]

//! [position]
void CBOEEngine::position(const std::string& account, const Contract& contract, Decimal position, double avgCost) {
  printf("Position. %s - Symbol: %s, SecType: %s, Currency: %s, Position: %s, Avg Cost: %s\n", account.c_str(),
         contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(),
         decimalStringToDisplay(position).c_str(), doubleMaxString(avgCost).c_str());

  dbglogger_ << "Func: " << __func__ << "\n";

  std::string exchange_symbol = "";
  auto exchange_symbol_itr = conID_to_exchangesymbol_.find(contract.conId);
  exchange_symbol =
      (exchange_symbol_itr != conID_to_exchangesymbol_.end()) ? conID_to_exchangesymbol_[contract.conId] : "INVALID";

  dbglogger_ << "OPEN POSTION  CON ID " << contract.conId << " Exchange Symbol " << exchange_symbol << " DATA SOURCE "
             << cboe_daily_token_symbol_handler_.GetInternalSymbolFromToken(contract.conId, cboe_segment_type_)
             << " SHC : " << cboe_sec_def_.GetShortCodeFromExchangeSymbol(exchange_symbol) << " POS "
             << decimalStringToDisplay(position) << "\n";

  if(use_data_over_html){
    HFSAT::Utils::HTMLGenerator::addOrUpdateRow(
        contract.conId, account, contract.symbol, contract.secType, contract.currency,
        decimalStringToDisplay(position), doubleMaxString(avgCost),
        exchange_symbol, cboe_sec_def_.GetShortCodeFromExchangeSymbol(exchange_symbol)
    );
  }
  /*
  auto exchange_symbol_itr = conID_to_exchangesymbol_.find(740308526);
  exchange_symbol = (exchange_symbol_itr != conID_to_exchangesymbol_.end()) ? conID_to_exchangesymbol_[740308526] : "";

  dbglogger_  << "OPEN POSTION  CON ID " <<  contract.conId
              << " Exchange Symbol " << exchange_symbol
              << " DATA SOURCE " << cboe_daily_token_symbol_handler_.GetInternalSymbolFromToken(740308526,
  cboe_segment_type_)
              << " SHC : " << cboe_sec_def_.GetShortCodeFromExchangeSymbol(exchange_symbol)
              << " POS " << decimalStringToDisplay(position) << "\n";
  */
}
//! [position]

//! [positionend]
void CBOEEngine::positionEnd() {
  dbglogger_ << "FETCH POSITIONS COMPLETED\n";
  dbglogger_ << "Requesting for cancellation no further updates\n";
  m_pClient->cancelPositions();

  printf("PositionEnd\n");
}
//! [positionend]

//! [accountsummary]
void CBOEEngine::accountSummary(int reqId, const std::string& account, const std::string& tag, const std::string& value,
                                const std::string& currency) {
  printf("Acct Summary. ReqId: %d, Account: %s, Tag: %s, Value: %s, Currency: %s\n", reqId, account.c_str(),
         tag.c_str(), value.c_str(), currency.c_str());
}
//! [accountsummary]

//! [accountsummaryend]
void CBOEEngine::accountSummaryEnd(int reqId) { printf("AccountSummaryEnd. Req Id: %d\n", reqId); }
//! [accountsummaryend]

void CBOEEngine::verifyMessageAPI(const std::string& apiData) { printf("verifyMessageAPI: %s\b", apiData.c_str()); }

void CBOEEngine::verifyCompleted(bool isSuccessful, const std::string& errorText) {
  printf("verifyCompleted. IsSuccessfule: %d - Error: %s\n", isSuccessful, errorText.c_str());
}

void CBOEEngine::verifyAndAuthMessageAPI(const std::string& apiDatai, const std::string& xyzChallenge) {
  printf("verifyAndAuthMessageAPI: %s %s\n", apiDatai.c_str(), xyzChallenge.c_str());
}

void CBOEEngine::verifyAndAuthCompleted(bool isSuccessful, const std::string& errorText) {
  printf("verifyAndAuthCompleted. IsSuccessful: %d - Error: %s\n", isSuccessful, errorText.c_str());
  if (isSuccessful) m_pClient->startApi();
}

//! [displaygrouplist]
void CBOEEngine::displayGroupList(int reqId, const std::string& groups) {
  printf("Display Group List. ReqId: %d, Groups: %s\n", reqId, groups.c_str());
}
//! [displaygrouplist]

//! [displaygroupupdated]
void CBOEEngine::displayGroupUpdated(int reqId, const std::string& contractInfo) {
  std::cout << "Display Group Updated. ReqId: " << reqId << ", Contract Info: " << contractInfo << std::endl;
}
//! [displaygroupupdated]

//! [positionmulti]
void CBOEEngine::positionMulti(int reqId, const std::string& account, const std::string& modelCode,
                               const Contract& contract, Decimal pos, double avgCost) {
  printf(
      "Position Multi. Request: %d, Account: %s, ModelCode: %s, Symbol: %s, SecType: %s, Currency: %s, Position: %s, "
      "Avg Cost: %s\n",
      reqId, account.c_str(), modelCode.c_str(), contract.symbol.c_str(), contract.secType.c_str(),
      contract.currency.c_str(), decimalStringToDisplay(pos).c_str(), doubleMaxString(avgCost).c_str());
}
//! [positionmulti]

//! [positionmultiend]
void CBOEEngine::positionMultiEnd(int reqId) { printf("Position Multi End. Request: %d\n", reqId); }
//! [positionmultiend]

//! [accountupdatemulti]
void CBOEEngine::accountUpdateMulti(int reqId, const std::string& account, const std::string& modelCode,
                                    const std::string& key, const std::string& value, const std::string& currency) {
  printf("AccountUpdate Multi. Request: %d, Account: %s, ModelCode: %s, Key, %s, Value: %s, Currency: %s\n", reqId,
         account.c_str(), modelCode.c_str(), key.c_str(), value.c_str(), currency.c_str());
}
//! [accountupdatemulti]

//! [accountupdatemultiend]
void CBOEEngine::accountUpdateMultiEnd(int reqId) { printf("Account Update Multi End. Request: %d\n", reqId); }
//! [accountupdatemultiend]

//! [securityDefinitionOptionParameter]
void CBOEEngine::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId,
                                                     const std::string& tradingClass, const std::string& multiplier,
                                                     const std::set<std::string>& expirations,
                                                     const std::set<double>& strikes) {
  printf("Security Definition Optional Parameter. Request: %d, Trading Class: %s, Multiplier: %s\n", reqId,
         tradingClass.c_str(), multiplier.c_str());
}
//! [securityDefinitionOptionParameter]

//! [securityDefinitionOptionParameterEnd]
void CBOEEngine::securityDefinitionOptionalParameterEnd(int reqId) {
  printf("Security Definition Optional Parameter End. Request: %d\n", reqId);
}
//! [securityDefinitionOptionParameterEnd]

//! [softDollarTiers]
void CBOEEngine::softDollarTiers(int reqId, const std::vector<SoftDollarTier>& tiers) {
  printf("Soft dollar tiers (%lu):", tiers.size());

  for (unsigned int i = 0; i < tiers.size(); i++) {
    printf("%s\n", tiers[i].displayName().c_str());
  }
}
//! [softDollarTiers]

//! [familyCodes]
void CBOEEngine::familyCodes(const std::vector<FamilyCode>& familyCodes) {
  printf("Family codes (%lu):\n", familyCodes.size());

  for (unsigned int i = 0; i < familyCodes.size(); i++) {
    printf("Family code [%d] - accountID: %s familyCodeStr: %s\n", i, familyCodes[i].accountID.c_str(),
           familyCodes[i].familyCodeStr.c_str());
  }
}
//! [familyCodes]

//! [symbolSamples]
void CBOEEngine::symbolSamples(int reqId, const std::vector<ContractDescription>& contractDescriptions) {
  printf("Symbol Samples (total=%lu) reqId: %d\n", contractDescriptions.size(), reqId);

  for (unsigned int i = 0; i < contractDescriptions.size(); i++) {
    Contract contract = contractDescriptions[i].contract;
    std::vector<std::string> derivativeSecTypes = contractDescriptions[i].derivativeSecTypes;
    printf("Contract (%u): conId: %ld, symbol: %s, secType: %s, primaryExchange: %s, currency: %s, ", i, contract.conId,
           contract.symbol.c_str(), contract.secType.c_str(), contract.primaryExchange.c_str(),
           contract.currency.c_str());
    printf("Derivative Sec-types (%lu):", derivativeSecTypes.size());
    for (unsigned int j = 0; j < derivativeSecTypes.size(); j++) {
      printf(" %s", derivativeSecTypes[j].c_str());
    }
    printf(", description: %s, issuerId: %s", contract.description.c_str(), contract.issuerId.c_str());
    printf("\n");
  }
}
//! [symbolSamples]

//! [mktDepthExchanges]
void CBOEEngine::mktDepthExchanges(const std::vector<DepthMktDataDescription>& depthMktDataDescriptions) {
  printf("Mkt Depth Exchanges (%lu):\n", depthMktDataDescriptions.size());

  for (unsigned int i = 0; i < depthMktDataDescriptions.size(); i++) {
    printf(
        "Depth Mkt Data Description [%d] - exchange: %s secType: %s listingExch: %s serviceDataType: %s aggGroup: %s\n",
        i, depthMktDataDescriptions[i].exchange.c_str(), depthMktDataDescriptions[i].secType.c_str(),
        depthMktDataDescriptions[i].listingExch.c_str(), depthMktDataDescriptions[i].serviceDataType.c_str(),
        intMaxString(depthMktDataDescriptions[i].aggGroup).c_str());
  }
}
//! [mktDepthExchanges]

//! [tickNews]
void CBOEEngine::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId,
                          const std::string& headline, const std::string& extraData) {
  printf("News Tick. TickerId: %d, TimeStamp: %s, ProviderCode: %s, ArticleId: %s, Headline: %s, ExtraData: %s\n",
         tickerId, ctime(&(timeStamp /= 1000)), providerCode.c_str(), articleId.c_str(), headline.c_str(),
         extraData.c_str());
}
//! [tickNews]

//! [smartcomponents]]
void CBOEEngine::smartComponents(int reqId, const SmartComponentsMap& theMap) {
  printf("Smart components: (%lu):\n", theMap.size());

  for (SmartComponentsMap::const_iterator i = theMap.begin(); i != theMap.end(); i++) {
    printf(" bit number: %d exchange: %s exchange letter: %c\n", i->first, std::get<0>(i->second).c_str(),
           std::get<1>(i->second));
  }
}
//! [smartcomponents]

//! [tickReqParams]
void CBOEEngine::tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) {
  printf("tickerId: %d, minTick: %s, bboExchange: %s, snapshotPermissions: %u\n", tickerId,
         doubleMaxString(minTick).c_str(), bboExchange.c_str(), snapshotPermissions);

  m_bboExchange = bboExchange;
}
//! [tickReqParams]

//! [newsProviders]
void CBOEEngine::newsProviders(const std::vector<NewsProvider>& newsProviders) {
  printf("News providers (%lu):\n", newsProviders.size());

  for (unsigned int i = 0; i < newsProviders.size(); i++) {
    printf("News provider [%d] - providerCode: %s providerName: %s\n", i, newsProviders[i].providerCode.c_str(),
           newsProviders[i].providerName.c_str());
  }
}
//! [newsProviders]

//! [newsArticle]
void CBOEEngine::newsArticle(int requestId, int articleType, const std::string& articleText) {
  printf("News Article. Request Id: %d, Article Type: %d\n", requestId, articleType);
  if (articleType == 0) {
    printf("News Article Text (text or html): %s\n", articleText.c_str());
  } else if (articleType == 1) {
    std::string path;
#if defined(IB_WIN32)
    TCHAR s[200];
    GetCurrentDirectory(200, s);
    path = s + std::string("\\MST$06f53098.pdf");
#elif defined(IB_POSIX)
    char s[1024];
    if (getcwd(s, sizeof(s)) == NULL) {
      printf("getcwd() error\n");
      return;
    }
    path = s + std::string("/MST$06f53098.pdf");
#endif
    std::vector<std::uint8_t> bytes = base64_decode(articleText);
    std::ofstream outfile(path, std::ios::out | std::ios::binary);
    outfile.write((const char*)bytes.data(), bytes.size());
    printf("Binary/pdf article was saved to: %s\n", path.c_str());
  }
}
//! [newsArticle]

//! [historicalNews]
void CBOEEngine::historicalNews(int requestId, const std::string& time, const std::string& providerCode,
                                const std::string& articleId, const std::string& headline) {
  printf("Historical News. RequestId: %d, Time: %s, ProviderCode: %s, ArticleId: %s, Headline: %s\n", requestId,
         time.c_str(), providerCode.c_str(), articleId.c_str(), headline.c_str());
}
//! [historicalNews]

//! [historicalNewsEnd]
void CBOEEngine::historicalNewsEnd(int requestId, bool hasMore) {
  printf("Historical News End. RequestId: %d, HasMore: %s\n", requestId, (hasMore ? "true" : " false"));
}
//! [historicalNewsEnd]

//! [headTimestamp]
void CBOEEngine::headTimestamp(int reqId, const std::string& headTimestamp) {
  printf("Head time stamp. ReqId: %d - Head time stamp: %s,\n", reqId, headTimestamp.c_str());
}
//! [headTimestamp]

//! [histogramData]
void CBOEEngine::histogramData(int reqId, const HistogramDataVector& data) {
  printf("Histogram. ReqId: %d, data length: %lu\n", reqId, data.size());

  for (const HistogramEntry& entry : data) {
    printf("\t price: %s, size: %s\n", doubleMaxString(entry.price).c_str(),
           decimalStringToDisplay(entry.size).c_str());
  }
}
//! [histogramData]

//! [historicalDataUpdate]
void CBOEEngine::historicalDataUpdate(TickerId reqId, const Bar& bar) {
  printf(
      "HistoricalDataUpdate. ReqId: %ld - Date: %s, Open: %s, High: %s, Low: %s, Close: %s, Volume: %s, Count: %s, "
      "WAP: %s\n",
      reqId, bar.time.c_str(), doubleMaxString(bar.open).c_str(), doubleMaxString(bar.high).c_str(),
      doubleMaxString(bar.low).c_str(), doubleMaxString(bar.close).c_str(), decimalStringToDisplay(bar.volume).c_str(),
      intMaxString(bar.count).c_str(), decimalStringToDisplay(bar.wap).c_str());
}
//! [historicalDataUpdate]

//! [rerouteMktDataReq]
void CBOEEngine::rerouteMktDataReq(int reqId, int conid, const std::string& exchange) {
  printf("Re-route market data request. ReqId: %d, ConId: %d, Exchange: %s\n", reqId, conid, exchange.c_str());
}
//! [rerouteMktDataReq]

//! [rerouteMktDepthReq]
void CBOEEngine::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) {
  printf("Re-route market depth request. ReqId: %d, ConId: %d, Exchange: %s\n", reqId, conid, exchange.c_str());
}
//! [rerouteMktDepthReq]

//! [marketRule]
void CBOEEngine::marketRule(int marketRuleId, const std::vector<PriceIncrement>& priceIncrements) {
  printf("Market Rule Id: %s\n", intMaxString(marketRuleId).c_str());
  for (unsigned int i = 0; i < priceIncrements.size(); i++) {
    printf("Low Edge: %s, Increment: %s\n", doubleMaxString(priceIncrements[i].lowEdge).c_str(),
           doubleMaxString(priceIncrements[i].increment).c_str());
  }
}
//! [marketRule]

//! [pnl]
void CBOEEngine::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) {
  printf("PnL. ReqId: %d, daily PnL: %s, unrealized PnL: %s, realized PnL: %s\n", reqId,
         doubleMaxString(dailyPnL).c_str(), doubleMaxString(unrealizedPnL).c_str(),
         doubleMaxString(realizedPnL).c_str());

  dbglogger_ << "PnL Update - ReqId: " << reqId << ", DailyPnL: " << dailyPnL << ", UnrealizedPnL: " << unrealizedPnL
             << ", RealizedPnL: " << realizedPnL << "\n";

  dbglogger_ << "PNL REQUEST RECEIVED DISABLING UPDATES FOR REQID " << reqId << "\n";
  m_pClient->cancelPnL(reqId);

  if(use_data_over_html){
    // std::cout<<HFSAT::GetTimeOfDay().ToString()<<std::endl;
    HFSAT::Utils::HTMLGenerator::updatePnl(dailyPnL,unrealizedPnL,realizedPnL,HFSAT::GetTimeOfDay().ToString());
  }
}
//! [pnl]

//! [pnlsingle]
void CBOEEngine::pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL,
                           double value) {
  printf("PnL Single. ReqId: %d, pos: %s, daily PnL: %s, unrealized PnL: %s, realized PnL: %s, value: %s\n", reqId,
         decimalStringToDisplay(pos).c_str(), doubleMaxString(dailyPnL).c_str(), doubleMaxString(unrealizedPnL).c_str(),
         doubleMaxString(realizedPnL).c_str(), doubleMaxString(value).c_str());
}
//! [pnlsingle]

//! [historicalticks]
void CBOEEngine::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done) {
  for (const HistoricalTick& tick : ticks) {
    std::time_t t = tick.time;
    std::cout << "Historical tick. ReqId: " << reqId << ", time: " << ctime(&t)
              << ", price: " << doubleMaxString(tick.price).c_str()
              << ", size: " << decimalStringToDisplay(tick.size).c_str() << std::endl;
  }
}
//! [historicalticks]

//! [historicalticksbidask]
void CBOEEngine::historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done) {
  for (const HistoricalTickBidAsk& tick : ticks) {
    std::time_t t = tick.time;
    std::cout << "Historical tick bid/ask. ReqId: " << reqId << ", time: " << ctime(&t)
              << ", price bid: " << doubleMaxString(tick.priceBid).c_str()
              << ", price ask: " << doubleMaxString(tick.priceAsk).c_str()
              << ", size bid: " << decimalStringToDisplay(tick.sizeBid).c_str()
              << ", size ask: " << decimalStringToDisplay(tick.sizeAsk).c_str()
              << ", bidPastLow: " << tick.tickAttribBidAsk.bidPastLow
              << ", askPastHigh: " << tick.tickAttribBidAsk.askPastHigh << std::endl;
  }
}
//! [historicalticksbidask]

//! [historicaltickslast]
void CBOEEngine::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast>& ticks, bool done) {
  for (HistoricalTickLast tick : ticks) {
    std::time_t t = tick.time;
    std::cout << "Historical tick last. ReqId: " << reqId << ", time: " << ctime(&t)
              << ", price: " << doubleMaxString(tick.price).c_str()
              << ", size: " << decimalStringToDisplay(tick.size).c_str() << ", exchange: " << tick.exchange
              << ", special conditions: " << tick.specialConditions
              << ", unreported: " << tick.tickAttribLast.unreported << ", pastLimit: " << tick.tickAttribLast.pastLimit
              << std::endl;
  }
}
//! [historicaltickslast]

//! [tickbytickalllast]
void CBOEEngine::tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
                                   const TickAttribLast& tickAttribLast, const std::string& exchange,
                                   const std::string& specialConditions) {
  printf(
      "Tick-By-Tick. ReqId: %d, TickType: %s, Time: %s, Price: %s, Size: %s, PastLimit: %d, Unreported: %d, Exchange: "
      "%s, SpecialConditions:%s\n",
      reqId, (tickType == 1 ? "Last" : "AllLast"), ctime(&time), doubleMaxString(price).c_str(),
      decimalStringToDisplay(size).c_str(), tickAttribLast.pastLimit, tickAttribLast.unreported, exchange.c_str(),
      specialConditions.c_str());
}
//! [tickbytickalllast]

//! [tickbytickbidask]
void CBOEEngine::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
                                  Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) {
  printf(
      "Tick-By-Tick. ReqId: %d, TickType: BidAsk, Time: %s, BidPrice: %s, AskPrice: %s, BidSize: %s, AskSize: %s, "
      "BidPastLow: %d, AskPastHigh: %d\n",
      reqId, ctime(&time), doubleMaxString(bidPrice).c_str(), doubleMaxString(askPrice).c_str(),
      decimalStringToDisplay(bidSize).c_str(), decimalStringToDisplay(askSize).c_str(), tickAttribBidAsk.bidPastLow,
      tickAttribBidAsk.askPastHigh);
}
//! [tickbytickbidask]

//! [tickbytickmidpoint]
void CBOEEngine::tickByTickMidPoint(int reqId, time_t time, double midPoint) {
  printf("Tick-By-Tick. ReqId: %d, TickType: MidPoint, Time: %s, MidPoint: %s\n", reqId, ctime(&time),
         doubleMaxString(midPoint).c_str());
}
//! [tickbytickmidpoint]

//! [orderbound]
void CBOEEngine::orderBound(long long orderId, int apiClientId, int apiOrderId) {
  printf("Order bound. OrderId: %s, ApiClientId: %s, ApiOrderId: %s\n", llongMaxString(orderId).c_str(),
         intMaxString(apiClientId).c_str(), intMaxString(apiOrderId).c_str());
}
//! [orderbound]

//! [completedorder]
void CBOEEngine::completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) {
  printf(
      "CompletedOrder. PermId: %s, ParentPermId: %s, Account: %s, Symbol: %s, SecType: %s, Exchange: %s:, Action: %s, "
      "OrderType: %s, TotalQty: %s, CashQty: %s, FilledQty: %s, "
      "LmtPrice: %s, AuxPrice: %s, Status: %s, CompletedTime: %s, CompletedStatus: %s, MinTradeQty: %s, "
      "MinCompeteSize: %s, CompeteAgainstBestOffset: %s, MidOffsetAtWhole: %s, MidOffsetAtHalf: %s\n",
      intMaxString(order.permId).c_str(), llongMaxString(order.parentPermId).c_str(), order.account.c_str(),
      contract.symbol.c_str(), contract.secType.c_str(), contract.exchange.c_str(), order.action.c_str(),
      order.orderType.c_str(), decimalStringToDisplay(order.totalQuantity).c_str(),
      doubleMaxString(order.cashQty).c_str(), decimalStringToDisplay(order.filledQuantity).c_str(),
      doubleMaxString(order.lmtPrice).c_str(), doubleMaxString(order.auxPrice).c_str(), orderState.status.c_str(),
      orderState.completedTime.c_str(), orderState.completedStatus.c_str(), intMaxString(order.minTradeQty).c_str(),
      intMaxString(order.minCompeteSize).c_str(),
      order.competeAgainstBestOffset == COMPETE_AGAINST_BEST_OFFSET_UP_TO_MID
          ? "UpToMid"
          : doubleMaxString(order.competeAgainstBestOffset).c_str(),
      doubleMaxString(order.midOffsetAtWhole).c_str(), doubleMaxString(order.midOffsetAtHalf).c_str());
}
//! [completedorder]

//! [completedordersend]
void CBOEEngine::completedOrdersEnd() { printf("CompletedOrdersEnd\n"); }
//! [completedordersend]

//! [replacefaend]
void CBOEEngine::replaceFAEnd(int reqId, const std::string& text) {
  printf("Replace FA End. Request: %d, Text:%s\n", reqId, text.c_str());
}
//! [replacefaend]

//! [wshMetaData]
void CBOEEngine::wshMetaData(int reqId, const std::string& dataJson) {
  printf("WSH Meta Data. ReqId: %d, dataJson: %s\n", reqId, dataJson.c_str());
}
//! [wshMetaData]

//! [wshEventData]
void CBOEEngine::wshEventData(int reqId, const std::string& dataJson) {
  printf("WSH Event Data. ReqId: %d, dataJson: %s\n", reqId, dataJson.c_str());
}
//! [wshEventData]

//! [historicalSchedule]
void CBOEEngine::historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime,
                                    const std::string& timeZone, const std::vector<HistoricalSession>& sessions) {
  printf("Historical Schedule. ReqId: %d, Start: %s, End: %s, TimeZone: %s\n", reqId, startDateTime.c_str(),
         endDateTime.c_str(), timeZone.c_str());
  for (unsigned int i = 0; i < sessions.size(); i++) {
    printf("\tSession. Start: %s, End: %s, RefDate: %s\n", sessions[i].startDateTime.c_str(),
           sessions[i].endDateTime.c_str(), sessions[i].refDate.c_str());
  }
}
//! [historicalSchedule]

//! [userInfo]
void CBOEEngine::userInfo(int reqId, const std::string& whiteBrandingId) {
  printf("User Info. ReqId: %d, WhiteBrandingId: %s\n", reqId, whiteBrandingId.c_str());
}
//! [userInfo]

void CBOEEngine::orderOperations(ORS::Order* order, InstrumentDesc* inst_desc) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  std::cout << " PLACE ORDER : " << std::endl;
  dbglogger_ << "PLACING LIMIT ORDER "
             << "\n";
  nextValidId(m_orderId);
  m_orderId += tv.tv_sec;

  Contract contract = ContractSamples::USOptionContract();
  SetDynamicOrderEntryRequestFields(contract, std::to_string(inst_desc->expiry_date_), inst_desc->strike_price_,
                                    inst_desc->option_type_);

  std::string buy_sell = (order->buysell_ == kTradeTypeBuy) ? "BUY" : "SELL";
  unsigned long long order_size = order->size_remaining_;
  double price = (double)(order->price_);

  std::cout << "Writing order\n";
  m_pClient->placeOrder(m_orderId++, contract, OrderSamples::LimitOrder(buy_sell, order_size, price));
  std::cout << "Writing order completed\n";

  /*
  m_pClient->placeOrder(m_orderId++, ContractSamples::USOptionContract(),
                        OrderSamples::LimitOrder("BUY", stringToDecimal("1"), 0.05));
  */

  // m_state = ST_ORDEROPERATIONS_ACK;

  // std::this_thread::sleep_for(std::chrono::seconds(1));

  //  m_pClient->placeOrder(m_orderId++, ContractSamples::OptionComboContract(), OrderSamples::ComboLimitOrder("SELL",
  //  stringToDecimal("1"), -4.3, false));
  //	m_pClient->reqGlobalCancel();

  // m_pClient->reqIds(-1);
  // //! [reqids]
  // //! [reqallopenorders]
  // m_pClient->reqAllOpenOrders();
  // //! [reqallopenorders]
  // //! [reqautoopenorders]
  // m_pClient->reqAutoOpenOrders(true);
  // //! [reqautoopenorders]
  // //! [reqopenorders]
  // m_pClient->reqOpenOrders();

  // /*** Requesting the next valid id ***/
  // //! [reqids]
  // //The parameter is always ignored.
  // //! [reqopenorders]

  // /*** Placing/modifying an order - remember to ALWAYS increment the nextValidId after placing an order so it can be
  // used for the next one! ***/
  //   //! [order_submission]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOrder("SELL",
  // stringToDecimal("1"), 50));
  //   //! [order_submission]

  // //m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::Block("BUY",
  // stringToDecimal("50"), 20));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::BoxTop("SELL",
  // stringToDecimal("10")));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::FutureComboContract(), OrderSamples::ComboLimitOrder("SELL",
  // stringToDecimal("1"), 1, false));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::StockComboContract(), OrderSamples::ComboMarketOrder("BUY",
  // stringToDecimal("1"), false));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::OptionComboContract(), OrderSamples::ComboMarketOrder("BUY",
  // stringToDecimal("1"), true));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::StockComboContract(),
  // OrderSamples::LimitOrderForComboWithLegPrices("BUY", stringToDecimal("1"), std::vector<double>(10, 5), true));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::Discretionary("SELL",
  // stringToDecimal("1"), 45, 0.5));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::LimitIfTouched("BUY",
  // stringToDecimal("1"), 30, 34));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOnClose("SELL",
  // stringToDecimal("1"), 34));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOnOpen("BUY",
  // stringToDecimal("1"), 35));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketIfTouched("BUY",
  // stringToDecimal("1"), 35));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOnClose("SELL",
  // stringToDecimal("1")));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOnOpen("BUY",
  // stringToDecimal("1")));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOrder("SELL",
  // stringToDecimal("1")));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketToLimit("BUY",
  // stringToDecimal("1")));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtIse(), OrderSamples::MidpointMatch("BUY",
  // stringToDecimal("1")));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::Stop("SELL",
  // stringToDecimal("1"), 34.4));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::StopLimit("BUY",
  // stringToDecimal("1"), 35, 33));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::StopWithProtection("SELL",
  // stringToDecimal("1"), 45));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::SweepToFill("BUY",
  // stringToDecimal("1"), 35));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::TrailingStop("SELL",
  // stringToDecimal("1"), 0.5, 30));
  // //m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::TrailingStopLimit("BUY",
  // stringToDecimal("100"), 2, 5, 50));

  // //! [place_midprice]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), OrderSamples::Midprice("BUY",
  // stringToDecimal("1"), 150));
  // //! [place_midprice]

  // //! [place order with cashQty]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), OrderSamples::LimitOrderWithCashQty("BUY",
  // 111.11, 5000));
  // //! [place order with cashQty]

  // std::this_thread::sleep_for(std::chrono::seconds(1));

  // /*** Cancel one order ***/
  // //! [cancelorder]
  // m_pClient->cancelOrder(m_orderId-1, "");
  // //! [cancelorder]

  // /*** Cancel all orders for all accounts ***/
  // //! [reqglobalcancel]
  // m_pClient->reqGlobalCancel();
  // //! [reqglobalcancel]

  // /*** Request the day's executions ***/
  // //! [reqexecutions]
  // m_pClient->reqExecutions(10001, ExecutionFilter());
  // //! [reqexecutions]

  // //! [reqcompletedorders]
  // m_pClient->reqCompletedOrders(false);
  // //! [reqcompletedorders]

  // //! [order_submission]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::CryptoContract(), OrderSamples::LimitOrder("BUY",
  // stringToDecimal("0.12345678"), 3700));
  // //! [order_submission]

  // //! [manual_order_time]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(),
  // OrderSamples::LimitOrderWithManualOrderTime("BUY", stringToDecimal("100"), 111.11, "20220314-13:00:00"));
  // //! [manual_order_time]

  // //! [manual_order_cancel_time]
  // m_pClient->cancelOrder(m_orderId - 1, "20220314-19:00:00");
  // //! [manual_order_cancel_time]

  // //! [pegbest_up_to_mid_order_submission]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::IBKRATSContract(), OrderSamples::PegBestUpToMidOrder("BUY",
  // stringToDecimal("100"), 111.11, 100, 200, 0.02, 0.025));
  // //! [pegbest_up_to_mid_order_submission]

  // //! [pegbest_order_submission]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::IBKRATSContract(), OrderSamples::PegBestOrder("BUY",
  // stringToDecimal("100"), 111.11, 100, 200, 0.03));
  // //! [pegbest_order_submission]

  // //! [pegmid_order_submission]
  // m_pClient->placeOrder(m_orderId++, ContractSamples::IBKRATSContract(), OrderSamples::PegMidOrder("BUY",
  // stringToDecimal("100"), 111.11, 100, 0.02, 0.025));
  // //! [pegmid_order_submission]

  // m_state = ST_ORDEROPERATIONS_ACK;
}

void CBOEEngine::setConnectOptions(const std::string& connectOptions) { m_pClient->setConnectOptions(connectOptions); }

void CBOEEngine::pnlOperation() {
  //! [reqpnl]
  m_pClient->reqPnL(7001, "DUD00029", "");
  //! [reqpnl]

  std::this_thread::sleep_for(std::chrono::seconds(2));

  //! [cancelpnl]
  m_pClient->cancelPnL(7001);
  //! [cancelpnl]

  m_state = ST_PNL_ACK;
}

void CBOEEngine::pnlSingleOperation() {
  //! [reqpnlsingle]
  m_pClient->reqPnLSingle(7002, "DUD00029", "", 268084);
  //! [reqpnlsingle]

  std::this_thread::sleep_for(std::chrono::seconds(2));

  //! [cancelpnlsingle]
  m_pClient->cancelPnLSingle(7002);
  //! [cancelpnlsingle]

  m_state = ST_PNLSINGLE_ACK;
}

void CBOEEngine::tickDataOperation() {}
void CBOEEngine::tickOptionComputationOperation() {}
void CBOEEngine::delayedTickDataOperation() {}
void CBOEEngine::marketDepthOperations() {}
void CBOEEngine::realTimeBars() {}
void CBOEEngine::marketDataType() {}
void CBOEEngine::historicalDataRequests() {}
void CBOEEngine::optionsOperations() {}

void CBOEEngine::accountOperations() {
  /*** Requesting managed accounts***/
  //! [reqmanagedaccts]
  m_pClient->reqManagedAccts();
  //! [reqmanagedaccts]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  /*** Requesting accounts' summary ***/
  //! [reqaaccountsummary]
  m_pClient->reqAccountSummary(9001, "All", AccountSummaryTags::getAllTags());
  //! [reqaaccountsummary]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //! [reqaaccountsummaryledger]
  m_pClient->reqAccountSummary(9002, "All", "$LEDGER");
  //! [reqaaccountsummaryledger]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //! [reqaaccountsummaryledgercurrency]
  m_pClient->reqAccountSummary(9003, "All", "$LEDGER:EUR");
  //! [reqaaccountsummaryledgercurrency]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //! [reqaaccountsummaryledgerall]
  m_pClient->reqAccountSummary(9004, "All", "$LEDGER:ALL");
  //! [reqaaccountsummaryledgerall]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //! [cancelaaccountsummary]
  m_pClient->cancelAccountSummary(9001);
  m_pClient->cancelAccountSummary(9002);
  m_pClient->cancelAccountSummary(9003);
  m_pClient->cancelAccountSummary(9004);
  //! [cancelaaccountsummary]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  /*** Subscribing to an account's information. Only one at a time! ***/
  //! [reqaaccountupdates]
  m_pClient->reqAccountUpdates(true, "U150462");
  //! [reqaaccountupdates]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //! [cancelaaccountupdates]
  m_pClient->reqAccountUpdates(false, "U150462");
  //! [cancelaaccountupdates]
  std::this_thread::sleep_for(std::chrono::seconds(2));

  //! [reqaaccountupdatesmulti]
  m_pClient->reqAccountUpdatesMulti(9002, "U150462", "EUstocks", true);
  //! [reqaaccountupdatesmulti]
  std::this_thread::sleep_for(std::chrono::seconds(2));

  /*** Requesting all accounts' positions. ***/
  //! [reqpositions]
  m_pClient->reqPositions();
  //! [reqpositions]
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //! [cancelpositions]
  m_pClient->cancelPositions();
  //! [cancelpositions]

  //! [reqpositionsmulti]
  m_pClient->reqPositionsMulti(9003, "U150462", "EUstocks");
  //! [reqpositionsmulti]

  //! [userinfo]
  m_pClient->reqUserInfo(0);
  //! [userinfo]

  m_state = ST_ACCOUNTOPERATIONS_ACK;
}

void CBOEEngine::ocaSamples() {}
void CBOEEngine::conditionSamples() {}
void CBOEEngine::bracketSample() {}
void CBOEEngine::hedgeSample() {}
void CBOEEngine::contractOperations() {}
void CBOEEngine::marketScanners() {}
void CBOEEngine::fundamentals() {}
void CBOEEngine::bulletins() {}
void CBOEEngine::testAlgoSamples() {}
void CBOEEngine::financialAdvisorOrderSamples() {}
void CBOEEngine::financialAdvisorOperations() {}
void CBOEEngine::testDisplayGroups() {}
void CBOEEngine::miscelaneous() {}
void CBOEEngine::reqFamilyCodes() {}
void CBOEEngine::reqMatchingSymbols() {}
void CBOEEngine::reqMktDepthExchanges() {}
void CBOEEngine::reqNewsTicks() {}
void CBOEEngine::reqSmartComponents() {}
void CBOEEngine::reqNewsProviders() {}
void CBOEEngine::reqNewsArticle() {}
void CBOEEngine::reqHistoricalNews() {}
void CBOEEngine::reqHeadTimestamp() {}
void CBOEEngine::reqHistogramData() {}
void CBOEEngine::rerouteCFDOperations() {}
void CBOEEngine::marketRuleOperations() {}
void CBOEEngine::continuousFuturesOperations() {}
void CBOEEngine::reqHistoricalTicks() {}
void CBOEEngine::reqTickByTickData() {}
void CBOEEngine::whatIfSamples() {}
void CBOEEngine::ibkratsSample() {}
void CBOEEngine::wshCalendarOperations() {}
void CBOEEngine::reqCurrentTime() {}

}  // namespace CBOE
}  // namespace HFSAT