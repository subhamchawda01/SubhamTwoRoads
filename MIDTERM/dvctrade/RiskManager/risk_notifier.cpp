#include "dvctrade/RiskManager/risk_notifier.hpp"

namespace HFSAT {

RiskNotifier* RiskNotifier::notifier_ = nullptr;

RiskNotifier::RiskNotifier(DebugLogger& dbglogger, const std::vector<std::string>& sec_list_vec)
    : packet_num_(0),
      slack_manager_(new HFSAT::SlackManager(RISKMANGER)),
      dbglogger_(dbglogger),
      sec_list_vec_(sec_list_vec),
      in_recovery(false),
      recovered(false) {
  // Connect to the central risk server at ny11
  tcp_socket_.Connect(GetServerIpToConnect(), SERVER_PORT);
  if (!tcp_socket_.IsOpen()) {
    dbglogger_ << "Could not connect to Central risk server 10.23.74.51:" << SERVER_PORT << "! Exiting!\n";
    exit(0);
  }
  trading_date_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  /// get broadcast ip/port info from network info manager
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  HFSAT::DataInfo combined_control_recv_data_info_ = network_account_info_manager_.GetCombControlDataInfo();

  /// create broadcast socket
  mcast_sock_ = new HFSAT::MulticastSenderSocket(
      combined_control_recv_data_info_.bcast_ip_, combined_control_recv_data_info_.bcast_port_,
      HFSAT::NetworkAccountInterfaceManager::instance().GetInterfaceForApp(HFSAT::k_Control));

  /// create struct to dump_ors_files, populate it and send it along
  comb_control_request_ = new HFSAT::CombinedControlMessage();
  // fetch and store hostname
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  hostname_ = hostname;

  strcpy(comb_control_request_->location_, hostname);
  comb_control_request_->message_code_ = kCmdControlMessageCodeDumpMdsFiles;
  // dump only ORS broadcast files
  comb_control_request_->generic_combined_control_msg_.dump_mds_files_.dump_only_ors_files_ = true;
  LoadSACITagMap();

  std::stringstream ss;
  ss << "/spare/local/logs/risk_logs/risk_monitor." << HFSAT::DateTime::GetCurrentIsoDateLocal();
  trades_file_.open(ss.str().c_str(), std::ofstream::out | std::ofstream::app);
}

RiskNotifier::~RiskNotifier() {
  // Stop listening to new updates
  tcp_socket_.Close();
  trades_file_.close();
}

RiskNotifier& RiskNotifier::setInstance(DebugLogger& dbglogger, const std::vector<std::string>& sec_list_vec) {
  if (notifier_ == nullptr) {
    notifier_ = new RiskNotifier(dbglogger, sec_list_vec);
  }
  return *notifier_;
}

RiskNotifier& RiskNotifier::getInstance() {
  if (notifier_ == nullptr) {
    std::cerr << "RiskNotifier getInstance called before setInstance. Exiting." << std::endl;
    exit(-1);
  }
  return *notifier_;
}

// returns the ip of running risk manager server
std::string RiskNotifier::GetServerIpToConnect() {
  HFSAT::TradingLocation_t curr_location_ = HFSAT::TradingLocationUtils::GetTradingLocationFromHostname();
  if (curr_location_ == kTLocHK)
    return "10.23.199.51";
  else
    return "10.23.74.51";
}

// used by queries to connect to the risk monitor client
std::string RiskNotifier::GetClientIpToNotifySaciMapping() { return "localhost"; }

bool RiskNotifier::InformMappingViaTcpToNy11(QueryTag& query_tag) {
  // Inform the same mapping to Ny11 server on PNL_SERVER_PORT
  HFSAT::TCPClientSocket saci_socket_to_ny11;
  saci_socket_to_ny11.Connect(GetServerIpToConnect(), PNL_SERVER_PORT);
  if (!saci_socket_to_ny11.IsOpen()) {
    std::cerr << "Could not inform via TCP on ny11, as server unreachable" << std::endl;
    return false;
  }

  int write_len = saci_socket_to_ny11.WriteN(sizeof(query_tag), &query_tag);
  if (write_len < (int)sizeof(query_tag)) {
    std::cerr << "Could not inform Risk Monitor Slave about the SACI-tag pair to NY11 Risk monitor server: "
              << write_len << ", " << errno << ", " << strerror(errno) << std::endl;
    return false;
  }
  saci_socket_to_ny11.Close();
  return true;
}

// Notify TCP server of the SACI-tag map (to be called by the query- TCP client)
bool RiskNotifier::InformMappingViaTcp(QueryTag& query_tag) {
  HFSAT::TCPClientSocket saci_socket;
  saci_socket.Connect(GetClientIpToNotifySaciMapping(), QUERY_LISTENER_PORT);
  if (!saci_socket.IsOpen()) {
    std::cerr << "Could not inform via TCP, as server unreachable" << std::endl;
    return false;
  }

  int write_len = saci_socket.WriteN(sizeof(query_tag), &query_tag);
  if (write_len < (int)sizeof(query_tag)) {
    std::cerr << "Could not inform Risk Monitor Slave about the SACI-tag pair: " << write_len << ", " << errno << ", "
              << strerror(errno) << std::endl;
    return false;
  }
  saci_socket.Close();
  return true;
}

void RiskNotifier::DumpAllRiskMappingOnSigInit() {
  // Dump SACI-tag mapping to file (with #define SACI_RECORD_FILE locking)
  int fd = AcquireFileLock();
  for (auto entry : saci_tags_map_) {
    QueryTag query_tag(entry.first, saci_queryid_map_[entry.first], entry.second, saci_start_time_map_[entry.first],
                       saci_end_time_map_[entry.first]);
    DumpSACITagMapping(query_tag, fd);
  }
  ReleaseFileLock(fd);
}

// Acquire lock on SACI-tag map file, and return fd
int RiskNotifier::AcquireFileLock(int append) {
  int fd = open(SACI_RECORD_FILE, O_CREAT | append | O_RDWR, 0777);
  if (fd <= 0) {
    std::cerr << "Could not open file " << SACI_RECORD_FILE << " for SACI tracking!" << std::endl;
  }
  int locked = -1, retries = 0;
  while (true) {
    // take exclusive lock before writing
    locked = flock(fd, LOCK_EX | LOCK_NB);
    if (locked == 0 || retries++ >= 4) {
      break;
    }
    sleep(1);  // sleep and retry
  }
  if (locked != 0) {
    std::cerr << "Could not lock file " << SACI_RECORD_FILE << ", even after retries, for SACI tracking!" << std::endl;
  }
  return fd;
}

// To dump SACI-tag maps to a file
void RiskNotifier::DumpSACITagMapping(QueryTag& query_tag, int given_fd) {
  // Acquire lock if fd not provided
  int fd = given_fd;
  if (fd <= 0) {
    fd = AcquireFileLock(O_APPEND);
  }
  // Prepare the string and write to file
  std::stringstream ss;
  ss << query_tag.query_id_ << " " << query_tag.saci_ << " " << query_tag.tags_ << " " << query_tag.utc_start_time_
     << " " << query_tag.utc_end_time_ << "\n";
  write(fd, ss.str().c_str(), ss.str().length());
  if (given_fd <= 0) {
    // Release lock only when created inside this function (otherwise leave it upto the parent)
    ReleaseFileLock(fd);
  }
}
// To load SACI-tag maps from a file
void RiskNotifier::LoadSACITagMap() {
  // Acquire lock if fd not provided
  std::ifstream fin(SACI_RECORD_FILE);
  int saci, query_id, utc_start_time, utc_end_time;
  std::string tags;
  while (fin.good()) {
    fin >> query_id >> saci >> tags >> utc_start_time >> utc_end_time;
    if (saci <= 0 || query_id <= 0 || tags.length() <= 0) break;
    QueryTag query_tag = QueryTag(saci, query_id, tags, utc_start_time, utc_end_time);
    AddSACIMapping(query_tag);
    InformMappingViaTcpToNy11(query_tag);
  }
  fin.close();
}

void RiskNotifier::LoadPastTradesFromOrsBinFile() {
  // before reading ors bcast files, dump all buffers via control cmd
  mcast_sock_->WriteN(sizeof(HFSAT::CombinedControlMessage), comb_control_request_);

  for (auto& shc_itr : sec_list_vec_) {
    // read past trades for this shortcode using ors bcast logs
    ReadORSLoggedDataForTrades(shc_itr);
  }

  // recovery complete. set the flags
  dbglogger_ << "Ending risk monitor recovery \n";
  dbglogger_.DumpCurrentBuffer();
  in_recovery = false;
  recovered = true;
  return;
}

void RiskNotifier::ReadORSLoggedDataForTrades(std::string shc) {
  std::string ors_logged_data_file = GetORSLoggedDataFilePath(shc);
  dbglogger_ << "Loading past trades: " << shc << " " << ors_logged_data_file << "\n";
  HFSAT::BulkFileReader bulk_file_reader_;
  bulk_file_reader_.open(ors_logged_data_file.c_str());
  HFSAT::MDS_MSG::GenericMDSMessage generic_msg;
  HFSAT::GenericORSReplyStructLive reply_struct;
  if (bulk_file_reader_.is_open()) {
    while (true) {
      size_t read_length_ =
          bulk_file_reader_.read(reinterpret_cast<char*>(&generic_msg), sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
      if (read_length_ < sizeof(HFSAT::MDS_MSG::GenericMDSMessage)) break;
      if (generic_msg.mds_msg_exch_ != HFSAT::MDS_MSG::ORS_REPLY) {
        dbglogger_ << "Only supports ORS generic logged data. Exiting. Exchange type read from  "
                   << ors_logged_data_file << " is " << (int)generic_msg.mds_msg_exch_ << "\n";
        dbglogger_.DumpCurrentBuffer();
        exit(-1);
      }
      reply_struct = generic_msg.generic_data_.ors_reply_data_;
      if (reply_struct.orr_type_ != HFSAT::kORRType_Exec) continue;  // process only executions
      saci_to_last_sams_map[reply_struct.server_assigned_client_id_] = reply_struct.server_assigned_message_sequence_;
      int size = reply_struct.size_executed_;
      if (reply_struct.buysell_ == HFSAT::kTradeTypeBuy)
        shortcode_pos_map_[shc] += size;
      else
        shortcode_pos_map_[shc] -= size;
      std::stringstream ss;
      ss << reply_struct.time_set_by_server_;
      dbglogger_ << "Past trade: " << shc << " " << reply_struct.server_assigned_client_id_ << " "
                 << (int)reply_struct.buysell_ << " " << size << " " << reply_struct.price_ << " "
                 << shortcode_pos_map_[shc] << " " << reply_struct.server_assigned_order_sequence_ << " "
                 << reply_struct.server_assigned_message_sequence_ << "\n";
      dbglogger_.DumpCurrentBuffer();
      OnOrderExecuted(reply_struct.server_assigned_client_id_, shc, reply_struct.buysell_, size, reply_struct.price_,
                      shortcode_pos_map_[shc], reply_struct.server_assigned_order_sequence_, ss.str(),
                      reply_struct.server_assigned_message_sequence_);
    }
    dbglogger_ << "Recovered trades till " << reply_struct.server_assigned_message_sequence_ << " for shc: " << shc
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  } else {
    dbglogger_ << "Couldn't recover ors trades for shc " << shc << " file " << ors_logged_data_file << "doesn't exist"
               << "\n";
    dbglogger_.DumpCurrentBuffer();
  }
}
std::string RiskNotifier::GetORSLoggedDataFilePath(std::string shc) {
  std::string file_path = "";
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(trading_date_);
  std::stringstream ss;
  ss << "/spare/local/MDSlogs/GENERIC/ORS_" << HFSAT::ExchangeSymbolManager::GetExchSymbol(shc) << "_" << trading_date_;
  file_path = ss.str();

  return file_path;
}
// Issue getflat messages to all queries associated with the given tag
void RiskNotifier::IssueGetFlat(GetFlatStruct& get_flat) {
  bool getflat = get_flat.getflat_;
  std::string tag = get_flat.tag_;

  dbglogger_ << "Num of queryids for tag " << tag << " : " << tag_queryvec_map_[tag].size() << "\n";
  dbglogger_.DumpCurrentBuffer();
  std::stringstream ss;
  ss << "Sending " << (getflat ? "GetFlat" : "StartTrading") << " @ " << hostname_ << " to: " << tag << " : ";
  size_t idx = 0;
  for (; idx < tag_queryvec_map_[tag].size(); idx++) {
    ss << tag_queryvec_map_[tag][idx] << ", ";
    // send multiple times for redundancy
    // TODO: Enable this
    /*for (size_t ctr = 0; ctr < 5; ctr++) {
      mcast_sock_->WriteN(sizeof(HFSAT::GenericControlRequestStruct), gcrs_);
    }*/
  }
  // If atleast one query got getflat
  if (idx > 0) {
    ss << std::endl;
    dbglogger_ << ss.str() << "\n";
    dbglogger_.DumpCurrentBuffer();
    // slack_manager_->sendNotification(ss.str()); //Disable this slack: makes channel ugly
  }
}

double RiskNotifier::GetN2D(std::string& shortcode, int yyyymmdd, double cur_price) {
  // Simple for non-DI contracts
  if (shortcode.substr(0, 3) != "DI1") {
    return HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode, yyyymmdd);
  }

  // For DI contracts
  if (di_reserves_map_.find(shortcode) == di_reserves_map_.end()) {
    di_reserves_map_[shortcode] = SecurityDefinitions::GetDIReserves(yyyymmdd, shortcode);
  }
  if (cur_price == 0.0) {
    // just to avoid inf values when cur_price is 0.0, we return invalid value
    return 1;
  }

  double unit_price = 0;
  double term = double(di_reserves_map_[shortcode] / 252.0);
  if (term > 0.000) {
    unit_price = 100000 / std::pow((cur_price / 100 + 1), term);
  }
  return -(unit_price * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD) / cur_price);
}
// Called on mktsizewprice changes
void RiskNotifier::OnMktPriceChange(std::string shortcode, double mkt_price) {
  // No need to process mkt px change if position is 0
  last_mkt_price_[shortcode] = mkt_price;
  int yyyymmdd = HFSAT::DateTime::GetCurrentIsoDateLocal();
  double n2d = GetN2D(shortcode, yyyymmdd, mkt_price);

  // Get current hhmm time
  time_t cur_time = time(NULL);
  cur_time /= 60;
  int mm = cur_time % 60;
  cur_time /= 60;
  int hhmm = (cur_time % 24) * 100 + mm;  // current time in hhmm

  for (auto shc_itr : positions_[shortcode]) {
    std::string tag = shc_itr.first;

    // Ignore if current position for this tag is 0 or end time for tag is over
    // If start_time > than end_time(AS hours), process update if its > start_time OR < end_time (Why??)
    if (shc_itr.second == 0)
      continue;
    else if (tag_start_time_map_[tag] > tag_end_time_map_[tag] &&
             (hhmm < tag_start_time_map_[tag] && hhmm > tag_end_time_map_[tag]))
      continue;
    else if (hhmm > tag_end_time_map_[tag])
      continue;
    // Update PNL for this tag-shc pair
    double commiss = BaseCommish::GetCommission(shortcode, yyyymmdd, mkt_price, abs(shc_itr.second));
    unrealized_pnls_[shortcode][tag] = shc_itr.second * mkt_price * n2d - commiss;
    NotifyPNLUpdate(shortcode, tag);
  }
}

void RiskNotifier::OnOrderExecuted(int saci, std::string shortcode, TradeType_t side, int order_size_executed,
                                   double price, int global_pos, int saos, std::string timestamp, int sams) {
  // Filter trades already processed during recovery
  if (!in_recovery && saci_to_last_sams_map.find(saci) != saci_to_last_sams_map.end() &&
      saci_to_last_sams_map[saci] >= sams) {
    dbglogger_ << "Already processed this trade during recovery: " << saci << " " << shortcode << " " << (int)side
               << " " << order_size_executed << " " << price << " " << global_pos << " " << saos << " " << timestamp
               << " " << sams << "\n";
    return;
  } else  // This ensure we are able to filter duplicate trades, if any
    saci_to_last_sams_map[saci] = sams;

  // Handling erroneous initial global position from ORS
  int size = order_size_executed;
  if (old_global_pos_.find(shortcode) != old_global_pos_.end()) {
    // We use difference in global position to find order size executed (as order_size_executed is the total size
    // executed of order till now)
    if (side == HFSAT::kTradeTypeBuy) {
      size = global_pos - old_global_pos_[shortcode];
    } else {
      size = old_global_pos_[shortcode] - global_pos;
    }
  }
  old_global_pos_[shortcode] = global_pos;
  if (size <= 0) {
    dbglogger_ << "ERROR: Invalid trade size calculated: " << saci << " " << shortcode << " " << (int)side << " "
               << order_size_executed << " " << price << " " << global_pos << " " << saos << " " << timestamp << "\n";
    return;
  }
  // fetch tags
  std::string tags = shortcode;
  if (saci_tags_map_.find(saci) != saci_tags_map_.end()) {
    tags += ": " + saci_tags_map_[saci];
  } else {
    dbglogger_ << "No idea about saci. Untagged query or console trade " << saci << " tags for : " << (int)side << " "
               << price << " " << size << " " << global_pos << "\n";
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    struct tm* tm = localtime(&current_time.tv_sec);
    char time_str[20];  // the current time in utc now
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);

    // based on this time, assign auto tags to this saci
    int hhmmss_time_now = atoi(time_str);
    if (hhmmss_time_now > 1300 && hhmmss_time_now < 2100)  // US hours in UTC
    {
      tags += ":GLOBAL:US";
      dbglogger_ << "Assigned auto tag: GLOBAL:US \n";
    } else if (hhmmss_time_now >= 630 && hhmmss_time_now <= 1300) {  // EU hours in UTC
      tags += ":GLOBAL:EU";
      dbglogger_ << "Assigned auto tag: GLOBAL:EU \n";
    } else {
      tags += ":GLOBAL:AS";
      dbglogger_ << "Assigned auto tag: GLOBAL:AS \n";
    }
  }
  std::vector<std::string> tags_vec;
  GetTagsFromString(tags, tags_vec);
  // TCP write the trade info
  int yyyymmdd = HFSAT::DateTime::GetCurrentIsoDateLocal();
  // Update pnl values for all the concerned tags
  if (last_mkt_price_.find(shortcode) == last_mkt_price_.end()) last_mkt_price_[shortcode] = price;
  // Compute N2D
  double n2d = GetN2D(shortcode, yyyymmdd, last_mkt_price_[shortcode]);
  double n2dt = GetN2D(shortcode, yyyymmdd, price);
  double trade_comm = BaseCommish::GetCommission(shortcode, yyyymmdd, price, size);
  dbglogger_ << "Trade: " << shortcode << " " << (int)side << " " << size << " " << price << " " << global_pos << " "
             << saci << " " << tags << " " << trade_comm << " " << saos << " " << last_mkt_price_[shortcode] << "\n";
  dbglogger_.DumpCurrentBuffer();

  for (size_t ctr = 0; ctr < tags_vec.size(); ctr++) {
    std::string tag = tags_vec[ctr];
    last_seen_saos_[tag][shortcode] = saos;

    // Update pos, vol, realized and unrealized pnl
    volume_[shortcode][tag] += size;
    if (side == kTradeTypeBuy) {
      positions_[shortcode][tag] += size;
      realized_pnls_[shortcode][tag] -= size * price * n2dt;
    } else {
      positions_[shortcode][tag] -= size;
      realized_pnls_[shortcode][tag] += size * price * n2dt;
    }

    // Incorporate commissions as well
    realized_pnls_[shortcode][tag] -= trade_comm;
    unrealized_pnls_[shortcode][tag] = positions_[shortcode][tag] * last_mkt_price_[shortcode] * n2d;
    // Incorporate commissions as well (assuming close-out at trade price)
    double commiss = 0.0;
    if (positions_[shortcode][tag] != 0) {
      commiss = BaseCommish::GetCommission(shortcode, yyyymmdd, price, abs(positions_[shortcode][tag]));
      unrealized_pnls_[shortcode][tag] -= commiss;
    }

    // Inform server about this pnl change
    NotifyPNLUpdate(shortcode, tags_vec[ctr], "trade");
    dbglogger_ << tag.c_str() << " " << realized_pnls_[shortcode][tag] + unrealized_pnls_[shortcode][tag] << " "
               << realized_pnls_[shortcode][tag] << " " << unrealized_pnls_[shortcode][tag] << " "
               << positions_[shortcode][tag] << "\n";

    // Log trade info first
    trades_file_ << timestamp << " " << shortcode << " " << side << " " << size << " " << price << " " << global_pos
                 << " " << saci << " " << tag << " " << saos << " "
                 << realized_pnls_[shortcode][tag] + unrealized_pnls_[shortcode][tag] << " "
                 << realized_pnls_[shortcode][tag] << " " << unrealized_pnls_[shortcode][tag] << " "
                 << positions_[shortcode][tag] << " " << last_mkt_price_[shortcode] << " " << trade_comm << " "
                 << commiss << " " << n2dt << " " << n2d << std::endl;
    trades_file_.flush();
  }
}

// Prepare a vector of strings (tags) from the combo string
void RiskNotifier::GetTagsFromString(std::string tags, std::vector<std::string>& tag_vec) {
  std::replace(tags.begin(), tags.end(), ':', ' ');
  std::string temp;
  std::stringstream ss(tags);
  while (ss >> temp) {
    tag_vec.push_back(temp);
  }
}
void RiskNotifier::NotifyPNLUpdate(std::string& shc, std::string& tag, std::string update_type) {
  // Apply cutoff of 20 $ for notification
  // in case of mkt update and cutoff <20$, dont notifying the server
  if (fabs(old_vals[tag][shc] - unrealized_pnls_[shc][tag] - realized_pnls_[shc][tag]) < 20 &&
      update_type == "mkt_update") {
    return;
  }
  old_vals[tag][shc] = unrealized_pnls_[shc][tag] + realized_pnls_[shc][tag];
  strcpy(new_update_.tag_, tag.c_str());
  strcpy(new_update_.shortcode_, shc.c_str());
  new_update_.update_type_ = UPDATE_VALUES;
  new_update_.unrealized_pnl_ = unrealized_pnls_[shc][tag];
  new_update_.realized_pnl_ = realized_pnls_[shc][tag];
  new_update_.position_ = positions_[shc][tag];
  new_update_.volume_ = volume_[shc][tag];
  dbglogger_ << new_update_.ToString();

  NotifyServer(new_update_);
}

bool RiskNotifier::ReconnectToServer() {
  int retries = 10;
  while (retries--) {
    tcp_socket_.Close();                                       // close the exiting socket
    tcp_socket_ = new HFSAT::TCPClientSocket();                // create a new instance
    tcp_socket_.Connect(GetServerIpToConnect(), SERVER_PORT);  // connect again
    if (!tcp_socket_.IsOpen()) {
      dbglogger_ << "Could not connect to Central risk server 10.23.74.51" << SERVER_PORT << " > ! Reconnect attempt "
                 << 11 - retries << " \n ";
    } else {
      dbglogger_ << "Reconnected to Central risk server 10.23.74.51" << SERVER_PORT << " !  attempts " << 11 - retries
                 << " \n ";
      return true;
    }
  }
  dbglogger_ << "Could not connect to Central risk server 10.23.74.51" << SERVER_PORT << "  ! Tried 10 attempts "
             << " Exiting \n ";
  dbglogger_.DumpCurrentBuffer();
  exit(-1);
}

void RiskNotifier::NotifyServer(RiskUpdateStruct& new_update) {
  int write_len = tcp_socket_.WriteN(sizeof(new_update), &new_update);
  if (write_len < (int)sizeof(new_update)) {
    // Error
    dbglogger_ << "Could not notify central risk monitor! Exiting!!! " << strerror(errno) << ", "
               << ", Expected_Write_Len: " << sizeof(new_update) << ", Write_Len: " << write_len << "\n";
    if (ReconnectToServer()) {
      // after reconnect, try to send this packet again
      write_len = tcp_socket_.WriteN(sizeof(new_update), &new_update);
      if (write_len < (int)sizeof(new_update)) {
        dbglogger_ << "Failed to notifyserver even after reconnect. Lets try next time. \n";
        dbglogger_.DumpCurrentBuffer();
      }
    }
  }
}

// push all executions received during recovery
void RiskNotifier::PushToRecoveryBuffer(ExecutionMsg* exec_msg) {
  dbglogger_ << "Buffered Execs: " << exec_msg->ToString();
  dbglogger_.DumpCurrentBuffer();
  recovery_buffer.push_back(exec_msg);
}

// Applies all buffered msgs after recovery is complete
// Check if sams for a saci is <= that remembered during recovery(saci_to_last_sams_map)
// ignore this exec as already processed
void RiskNotifier::ApplyRecoveryBuffer() {
  for (auto buffered_exec : recovery_buffer) {
    if (saci_to_last_sams_map.find(buffered_exec->saci_) == saci_to_last_sams_map.end() ||
        saci_to_last_sams_map[buffered_exec->saci_] < buffered_exec->sams_)
      dbglogger_ << "Applying Buffer Execs: " << buffered_exec->ToString();
    dbglogger_.DumpCurrentBuffer();
    OnOrderExecuted(buffered_exec->saci_, buffered_exec->shortcode_, buffered_exec->side_,
                    buffered_exec->order_size_executed_, buffered_exec->price_, buffered_exec->global_pos_,
                    buffered_exec->saos_, buffered_exec->timestamp_, buffered_exec->sams_);
  }
}
}
