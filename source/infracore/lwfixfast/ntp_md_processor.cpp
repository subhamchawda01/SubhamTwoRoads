/**
    \file lwfixfast/ntp_md_processor.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include <fstream>
#include "infracore/lwfixfast/ntp_md_processor.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/refdata_locator.hpp"

namespace NTP_MD_PROCESSOR {

/// Makes sure that there is only one instance of NtpMDProcessor
NtpMDProcessor& NtpMDProcessor::GetInstance() { return SetInstance(NULL, NULL, HFSAT::kModeMax, true); }

void NtpMDProcessor::InitMcastMode(HFSAT::MulticastSenderSocket* sock, HFSAT::NewNTPRawLiveDataSource* rls,
                                   bool recovery_flag) {
  SetInstance(sock, rls, HFSAT::kMcast, recovery_flag);
}

void NtpMDProcessor::InitRefMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
  SetInstance(NULL, rls, HFSAT::kReference, recovery_flag);
}

void NtpMDProcessor::InitRawMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
  SetInstance(NULL, rls, HFSAT::kRaw, recovery_flag);
}

void NtpMDProcessor::InitLoggerMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
  SetInstance(NULL, rls, HFSAT::kLogger, recovery_flag);
}

void NtpMDProcessor::InitProShmMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
  SetInstance(NULL, rls, HFSAT::kProShm, recovery_flag);
}

void NtpMDProcessor::InitComShmMode(HFSAT::NewNTPRawLiveDataSource* rls, bool recovery_flag) {
  SetInstance(NULL, rls, HFSAT::kComShm, recovery_flag);
}

NtpMDProcessor::NtpMDProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                               HFSAT::FastMdConsumerMode_t mode_,
                               bool recovery_flag_)
    : recovery_flag(recovery_flag_),  // always recover, since we need to update sec_trading_status at startup
      sock(sock_),
      ntpLiveSource(rls),
      cstr(new NTP_MDS::NTPCommonStruct()),
      old_cstr_(new NTP_MDS::NTPCommonStruct()),
      is_old_valid(false),
      old_sec_id(-1),
      old_trd_cstr_(new NTP_MDS::NTPCommonStruct()),
      old_trd_valid(false),
      old_trd_sec_id(-1),
      channel_recovery_(),
      num_recovery_securities_(0),
      mode(mode_),
      mds_shm_interface_(nullptr),
      generic_mds_message_(new HFSAT::MDS_MSG::GenericMDSMessage()),
      mdsLogger("BMF_PUMA"),
      product_seen(),
      ref_data_() {
  md_time_.tv_sec = 0;
  gettimeofday(&recovery_start_time_, NULL);

  if (mode == HFSAT::kModeMax) {
    return;  // dummy arg
  }

  if (mode == HFSAT::kLogger) {
    mdsLogger.run();
  }

  if (mode == HFSAT::kComShm) {
    mds_shm_interface_ = &HFSAT::Utils::MDSShmInterface::GetUniqueInstance();
  }

  if (mode == HFSAT::kReference) {
    OpenRefFiles();

  } else {
    LoadRefData();
  }

  if (mode == HFSAT::kProShm) {
    shm_writer_ = new SHM::ShmWriter<NTP_MDS::NTPCommonStruct>(SHM_KEY_NTP_RAW, NTP_RAW_SHM_QUEUE_SIZE);
  }

  // Start recovery for all instruments
  StartAllRecovery();
}

NtpMDProcessor* NtpMDProcessor::ins = NULL;

NtpMDProcessor& NtpMDProcessor::SetInstance(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                                            HFSAT::FastMdConsumerMode_t mode, bool recovery_flag) {
  if (ins == NULL) {
    ins = new NtpMDProcessor(sock_, rls, mode, recovery_flag);
  }

  return *ins;
}

void NtpMDProcessor::cleanup() {
  if (rfile.is_open()) {
    rfile.close();
    std::cerr << "closed reference file\n";
  }

  if (mdsLogger.isRunning()) {
    mdsLogger.stop();
  }

  if (NULL != cstr) {
    delete cstr;
    delete old_cstr_;
    delete old_trd_cstr_;
    delete generic_mds_message_;

    cstr = NULL;
    old_cstr_ = NULL;
    old_trd_cstr_ = NULL;
    generic_mds_message_ = NULL;
  }

  if (NULL != ins) {
    delete ins;
    ins = NULL;
  }
}

// Open ref files to write ref data in Reference mode
void NtpMDProcessor::OpenRefFiles() {
  HFSAT::FileUtils::MkdirEnclosing(DEF_NTP_REFLOC_);  // makes the directory if it does not exist
  rfile.open(DEF_NTP_REFLOC_, std::ofstream::out);

  if (!rfile.is_open()) {
    fprintf(stderr, "Cannot open file %s for logging address data\n", DEF_NTP_REFLOC_);
    exit(-1);
  }

  rfile << "##Numeric_Id\tInternal_Ref\t\tChannel\t\tGroup_Code" << std::endl;
}

// Load ref data from ref files
void NtpMDProcessor::LoadRefData() {
  std::string ref_file = DEF_NTP_REFLOC_;
  dfile.open(ref_file.c_str(), std::ofstream::in);

  if (!dfile.is_open()) {
    fprintf(stderr, "Cannot open file %s for reading reference data\n", ref_file.c_str());
    exit(-1);
  }

  char line[1024];
  while (!dfile.eof()) {
    memset(line, 0, sizeof(line));
    dfile.getline(line, sizeof(line));

    // Ignore empty lines and comments
    if (strlen(line) == 0 || line[0] == '#') {
      continue;
    }

    uint64_t num_id = atol(strtok(line, "\n\t "));
    char* strname = (char*)calloc(20, sizeof(char));
    strncpy(strname, strtok(NULL, "\n\t "), 20);

    if (mode == HFSAT::kRaw && ntpLiveSource->getSecurityNameIndexer().GetIdFromSecname(strname) < 0) {
      continue;
    }

    NTPRefStruct* ref = new NTPRefStruct();
    ref_data_[num_id] = ref;

    ref->secname = strname;
    ref->channel_no = atoi(strtok(NULL, "\n\t "));
    ref->rpt_seq = 0;
    ref->recovery = false;
    ref->id = num_id;
    ref->trading_status = NTP_SEC_TRADING_STATUS_CLOSE;

    char groupname[20];
    strncpy(groupname, strtok(NULL, "\n\t "), 5);
    int group_id = groupname[0] * 256 + groupname[1];
    ref->group_id = group_id;
  }

  dfile.close();
}

// Start Recovery for all securities
// This is to get the initial trading state - generally called only at the start
// We do not recover in between
void NtpMDProcessor::StartAllRecovery() {
  for (auto ref_pair : ref_data_) {
    StartRecovery(ref_pair.first);
  }
}

// Start recovery for specific instrument
void NtpMDProcessor::StartRecovery(uint64_t sec_id) {
  // Return if the sec_id is not present in the ref_data
  if (ref_data_.find(sec_id) == ref_data_.end()) {
    return;
  }

  NTPRefStruct* ref = ref_data_[sec_id];

  // Return if this instrument is already in recovery
  if (ref->recovery) {
    return;
  }

  // Set the recovery flag for this instrument
  ref->recovery = true;

  num_recovery_securities_++;

  // If channel is already in recovery, return from here
  if (channel_recovery_.find(ref->channel_no) != channel_recovery_.end()) {
    return;
  }

  // Start recovery on this channel
  if (ntpLiveSource) {
    ntpLiveSource->startRecovery(ref->channel_no);
  }

  // Add to the channel_recovery set to avoid calling recovery for this channel again
  channel_recovery_.insert(ref->channel_no);
}

// End recovery for the specified instrument
void NtpMDProcessor::EndRecovery(uint64_t sec_id) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  // Return if this isn't in recovery
  if (!ref->recovery) {
    return;
  }

  ref->recovery = false;
  num_recovery_securities_--;

  // If all the securities have recovered, end recovery from all channels at once
  if (num_recovery_securities_ <= 0) {
    EndAllRecovery();
  }
}

// End recovery for all products
void NtpMDProcessor::EndAllRecovery() {
  for (auto ref_pair : ref_data_) {
    ref_pair.second->recovery = false;
  }

  std::cout << "Ending recovery for all products. NumSecsLeft: " << num_recovery_securities_ << std::endl;

  num_recovery_securities_ = 0;

  for (auto channel : channel_recovery_) {
    if (ntpLiveSource) {
      ntpLiveSource->endRecovery(channel);
    }
  }

  // Empty channel recovery set, so that it can be reused for recovery again
  channel_recovery_.clear();
}

// Returns true if the recovery is going on
bool NtpMDProcessor::UpdateSeqno(uint64_t sec_id, uint64_t seq_no) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  if (ref->recovery) {
    return true;
  }

  // Check if the new seqno is more than the expected rpt_seq
  //  if (ref->rpt_seq !=0 && seq_no > ref->rpt_seq) {
  //
  //    // Start the recovery process
  //    std::cout << "Starting recovery for secid: " << sec_id
  //        << " secname: " << ref->secname
  //        << " NewSeqno: " << seq_no
  //        << " PrevSeqno: " << ref->rpt_seq << std::endl;
  //    ref->rpt_seq = 0;
  //
  //    gettimeofday(&recovery_start_time_, NULL);
  //    StartRecovery(sec_id);
  //    return true;
  //  }

  // Assign the next expected rpt_seq
  ref->rpt_seq = seq_no + 1;
  return false;
}

void NtpMDProcessor::flushTradeQueue() {
  if (!old_trd_valid) {
    return;
  }

  assert(old_trd_cstr_->msg_ == NTP_MDS::NTP_TRADE);

  gettimeofday(&(old_trd_cstr_->time_), NULL);

  switch (mode) {
    case HFSAT::kMcast: {
      sock->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_trd_cstr_);
      break;
    }

    case HFSAT::kComShm: {
      generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NTP;
      memcpy((void*)&(generic_mds_message_->generic_data_).ntp_data_, (void*)old_trd_cstr_,
             sizeof(NTP_MDS::NTPCommonStruct));
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_);
      if (ntpLiveSource->IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
      }

    } break;

    case HFSAT::kProShm: {
      shm_writer_->Write(old_trd_cstr_);
      break;
    }

    case HFSAT::kLogger:
      mdsLogger.log(*old_trd_cstr_);
      break;
    case HFSAT::kReference:
    case HFSAT::kModeMax:
    default:
      break;
  }

  old_trd_valid = false;
}

void NtpMDProcessor::dispatchTrade(uint64_t sec_id) {
  // flush any book messages we might have
  flushQuoteQueue(false);  // non intermediate

  assert(cstr->msg_ == NTP_MDS::NTP_TRADE);

  // Dispatch the old message.
  if (old_trd_valid) {
    if (old_trd_sec_id == sec_id && old_trd_cstr_->data_.ntp_trds_.trd_px_ == cstr->data_.ntp_trds_.trd_px_) {
      old_trd_cstr_->data_.ntp_trds_.tot_qty_ = cstr->data_.ntp_trds_.tot_qty_;   // copy
      old_trd_cstr_->data_.ntp_trds_.trd_qty_ += cstr->data_.ntp_trds_.trd_qty_;  // increment
      return;
    }

    flushTradeQueue();
  }

  old_trd_sec_id = sec_id;
  memcpy(old_trd_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_trd_valid = true;
}

void NtpMDProcessor::flushQuoteQueue(bool intermediate) {
  if (num_recovery_securities_ > 0) {
    timeval current_time_;
    gettimeofday(&current_time_, NULL);

    // Do not wait more than 1 minute for recovery
    if (current_time_.tv_sec - recovery_start_time_.tv_sec > 90) {
      EndAllRecovery();
    }
  }

  if (!is_old_valid) {
    md_time_.tv_sec = 0;
    return;
  }

  old_cstr_->data_.ntp_dels_.intermediate_ = intermediate;

  if (md_time_.tv_sec == 0) {
    gettimeofday(&md_time_, NULL);
  }

  old_cstr_->time_ = md_time_;

  if (!intermediate) {
    md_time_.tv_sec = 0;
  }

  switch (mode) {
    case HFSAT::kMcast: {
      sock->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_cstr_);
      break;
    }
    case HFSAT::kComShm: {
      generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NTP;
      memcpy((void*)&(generic_mds_message_->generic_data_).ntp_data_, (void*)old_cstr_,
             sizeof(NTP_MDS::NTPCommonStruct));
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_);
      if (ntpLiveSource->IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
      }
      break;
    }
    case HFSAT::kProShm: {
      shm_writer_->Write(old_cstr_);
      break;
    }

    case HFSAT::kLogger:
      mdsLogger.log(*old_cstr_);
      break;
    case HFSAT::kReference:
    case HFSAT::kModeMax:
    default:
      break;
  }
  is_old_valid = false;
}

void NtpMDProcessor::dispatchQuote(uint64_t sec_id) {
  // flush any trade messages we might have
  flushTradeQueue();
  flushQuoteQueue(sec_id == old_sec_id);  // previous message had same sec id as this one it was intermediate, else not
  memcpy(old_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_sec_id = sec_id;
  is_old_valid = true;
}

void NtpMDProcessor::DumpSecurityDefinition(uint64_t secId, const std::string& symbol, const std::string& secGroup) {
  rfile << secId << "\t\t" << symbol << "\t\t" << ntpLiveSource->getCurrChannel() << "\t\t" << secGroup << "\n"
        << std::flush;
}

void NtpMDProcessor::endRecovery(int channel_id_) { ntpLiveSource->endRecovery(channel_id_); }

void NtpMDProcessor::seqReset() { ntpLiveSource->seqReset(); }

// Update the NTP market data struct with the parameters extracted from the
// fast message
void NtpMDProcessor::UpdateBookDelta(uint64_t sec_id, uint16_t level, uint16_t num_ords, double price, int32_t size,
                                     uint32_t seq_no, char entry_type, uint32_t update_action, uint16_t buyer,
                                     uint16_t seller) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  memset((void*)cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  cstr->msg_ = NTP_MDS::NTP_DELTA;

  strcpy(cstr->data_.ntp_dels_.contract_, ref->secname);

  cstr->data_.ntp_dels_.level_ = level;
  cstr->data_.ntp_dels_.num_ords_ = num_ords;
  cstr->data_.ntp_dels_.price_ = price;
  cstr->data_.ntp_dels_.size_ = size;
  cstr->data_.ntp_dels_.seqno_ = seq_no;
  cstr->data_.ntp_dels_.type_ = entry_type;
  cstr->data_.ntp_dels_.action_ = update_action;
  cstr->data_.ntp_dels_.flags[1] = ref->trading_status;

  dispatchQuote(sec_id);
}

// Update the NTP market data struct with the parameters extracted from the
// fast message
void NtpMDProcessor::UpdateTrade(uint64_t sec_id, int32_t size, uint64_t total_qty, uint32_t seq_no, double price,
                                 bool is_last, bool is_cross) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  memset((void*)cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  cstr->msg_ = NTP_MDS::NTP_TRADE;

  strcpy(cstr->data_.ntp_dels_.contract_, ref->secname);

  cstr->data_.ntp_trds_.trd_qty_ = size;
  cstr->data_.ntp_trds_.tot_qty_ = total_qty;
  cstr->data_.ntp_trds_.seqno_ = seq_no;
  cstr->data_.ntp_trds_.trd_px_ = price;
  cstr->data_.ntp_trds_.is_last_ = is_last;

  if (is_cross) {
    cstr->data_.ntp_trds_.flags_[0] = 'X';
  }
  cstr->data_.ntp_trds_.flags_[1] = ref->trading_status;

  dispatchTrade(sec_id);
}

void NtpMDProcessor::UpdateOpeningPrice(uint64_t sec_id, double price, int32_t size, uint32_t seq_no,
                                        uint32_t open_close_flag) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  memset((void*)cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  cstr->msg_ = NTP_MDS::NTP_OPENPRICE;

  strcpy(cstr->data_.ntp_open.contract, ref->secname);

  cstr->data_.ntp_open.price = price;
  cstr->data_.ntp_open.size = size;
  cstr->data_.ntp_open.seq_no = seq_no;
  cstr->data_.ntp_open.theoretical = open_close_flag;

  dispatchQuote(sec_id);
}

void NtpMDProcessor::UpdateImbalance(uint64_t sec_id, int32_t size, uint32_t seq_no, char condition) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  memset((void*)cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  cstr->msg_ = NTP_MDS::NTP_IMBALANCE;

  strcpy(cstr->data_.ntp_imbalance.contract, ref->secname);

  cstr->data_.ntp_imbalance.size = size;
  cstr->data_.ntp_imbalance.seq_no = seq_no;
  cstr->data_.ntp_imbalance.condition = condition;

  dispatchQuote(sec_id);
}

void NtpMDProcessor::UpdateSecurityTradingStatus(uint64_t sec_id, uint32_t trading_status, uint64_t open_time,
                                                 uint64_t transact_time, uint32_t trading_event, uint32_t rpt_seq) {
  // Return from here if trading_status is not valid
  if (!NTP_MDS::IsStatusValid(trading_status)) {
    return;
  }

  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  // If it's in recovery, end recovery
  if (ref->recovery) {
    EndRecovery(sec_id);
  }

  // If there is not change in the trading status, ignore this
  if (ref->trading_status == trading_status) {
    return;
  }

  ref->trading_status = trading_status;

  memset((void*)cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  cstr->msg_ = NTP_MDS::NTP_STATUS;

  strcpy(cstr->data_.ntp_status.contract, ref->secname);

  cstr->data_.ntp_status.security_status = trading_status;
  cstr->data_.ntp_status.trading_event = trading_event;
  cstr->data_.ntp_status.open_time = open_time;
  cstr->data_.ntp_status.transact_time = transact_time;

  dispatchQuote(sec_id);
}

void NtpMDProcessor::UpdateGroupTradingStatusRefresh(uint64_t sec_id, uint32_t trading_status, uint64_t open_time,
                                                     uint64_t transact_time, uint32_t trading_event, uint32_t rpt_seq) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  NTPRefStruct* ref = ref_data_[sec_id];

  UpdateGroupTradingStatusInc(ref->group_id, trading_status, open_time, transact_time, trading_event, rpt_seq);
}

void NtpMDProcessor::UpdateGroupTradingStatusInc(int group_id, uint32_t trading_status, uint64_t open_time,
                                                 uint64_t transact_time, uint32_t trading_event, uint32_t rpt_seq) {
  // Return from here if trading_status is not valid
  if (!NTP_MDS::IsStatusValid(trading_status)) {
    return;
  }

  for (auto ref_pair : ref_data_) {
    NTPRefStruct* temp_ref = ref_pair.second;

    if (temp_ref->group_id == group_id) {
      UpdateSecurityTradingStatus(temp_ref->id, trading_status, open_time, transact_time, trading_event, rpt_seq);
    }
  }
}

bool NtpMDProcessor::IsSecurityInTrdingPhase(uint64_t sec_id) {
  if (ref_data_.find(sec_id) == ref_data_.end()) {
    return false;
  }

  NTPRefStruct* ref = ref_data_[sec_id];
  return ref->trading_status == NTP_SEC_TRADING_STATUS_OPEN;
}

bool NtpMDProcessor::IsProcessingSecurity(const uint64_t& sec_id) {
  if (ref_data_.find(sec_id) == ref_data_.end()) {
    return false;
  }

  return true;
}
}
