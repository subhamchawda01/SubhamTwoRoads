/**
   \file Puma_md_processor.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <fstream>
#include "infracore/lwfixfast/puma_md_processor.hpp"
#include "dvccode/CDef/refdata_locator.hpp"
#include "dvccode/CDef/defines.hpp"

namespace PUMA_MD_PROCESSOR {

// Makes sure that there is only one instance of PumaMDProcessor
PumaMDProcessor& PumaMDProcessor::GetInstance() { return SetInstance(NULL, NULL, HFSAT::kModeMax, true, true); }

void PumaMDProcessor::InitMcastMode(HFSAT::MulticastSenderSocket* sock, HFSAT::NewPumaRawLiveDataSource* rls,
                                    bool recovery_flag, int bmf_feed_type) {
  SetInstance(sock, rls, HFSAT::kMcast, recovery_flag, bmf_feed_type);
}

void PumaMDProcessor::InitRefMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type,
                                  bool is_full_reference_mode) {
  SetInstance(NULL, rls, HFSAT::kReference, recovery_flag, bmf_feed_type, is_full_reference_mode);
}

void PumaMDProcessor::InitRawMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type) {
  SetInstance(NULL, rls, HFSAT::kRaw, recovery_flag, bmf_feed_type);
}

void PumaMDProcessor::InitLoggerMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type) {
  SetInstance(NULL, rls, HFSAT::kLogger, recovery_flag, bmf_feed_type);
}

void PumaMDProcessor::InitProShmMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type) {
  SetInstance(NULL, rls, HFSAT::kProShm, recovery_flag, bmf_feed_type);
}

void PumaMDProcessor::InitComShmMode(HFSAT::NewPumaRawLiveDataSource* rls, bool recovery_flag, int bmf_feed_type) {
  SetInstance(NULL, rls, HFSAT::kComShm, recovery_flag, bmf_feed_type);
}

PumaMDProcessor::PumaMDProcessor(HFSAT::MulticastSenderSocket* sock, HFSAT::NewPumaRawLiveDataSource* rls,
                                 HFSAT::FastMdConsumerMode_t mode, bool recovery_flag, int bmf_feed_type,
                                 bool is_full_reference_mode)
    : recovery_flag_(recovery_flag),
      sock_(sock),
      is_full_reference_mode_(is_full_reference_mode),
      mdsLogger(bmf_feed_type == 1 ? "PUMA" : (bmf_feed_type == 2) ? "NTP" : "NTP_ORD"),
      pumaLiveSource(rls),
      cstr(new NTP_MDS::NTPCommonStruct()),
      old_cstr_(new NTP_MDS::NTPCommonStruct()),
      is_old_valid(false),
      old_sec_id(-1),
      old_trd_cstr_(new NTP_MDS::NTPCommonStruct()),
      old_trd_valid(false),
      old_trd_sec_id(-1),
      bmf_feed_type_(bmf_feed_type),
      product_seen(),
      security_trading_status_(),
      security_group_trading_status_(),
      tradingdate_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
      mode_(mode),
      mds_shm_interface_(nullptr),
      generic_mds_message_(new HFSAT::MDS_MSG::GenericMDSMessage()),
      rpt_seq_map_(),
      num_recovery_securities_(0) {
  md_time_.tv_sec = 0;
  gettimeofday(&recovery_start_time_, NULL);

  if (mode_ == HFSAT::kModeMax) {
    return;  // dummy arg
  }

  if (mode_ == HFSAT::kLogger) {
    mdsLogger.run();  // multithread
  }

  if (mode_ == HFSAT::kReference) {
    OpenRefFiles();
  }

  if (mode_ == HFSAT::kComShm) {
    mds_shm_interface_ = &HFSAT::Utils::MDSShmInterface::GetUniqueInstance();
  }

  if (mode_ != HFSAT::kReference) {
    LoadRefData();
    if (bmf_feed_type == 1) OpenSettlementFile();
  }

  if (mode_ == HFSAT::kProShm) {
    // TODO: change the key from -1 to something meaningful
    shm_writer_ = new SHM::ShmWriter<NTP_MDS::NTPCommonStruct>(-1, PUMA_RAW_SHM_QUEUE_SIZE);
  }

  // Start recovery for all instruments
  StartAllRecovery();

  // PrintGroups();
}

PumaMDProcessor* PumaMDProcessor::ins = NULL;

void PumaMDProcessor::PrintGroups() {
  for (auto& ref_pair : ref_data_) {
    PUMARefStruct* ref = ref_pair.second;
    std::cerr << "Group: " << ref->group_id << " secname: " << ref->secname << std::endl;
  }
}

// Open ref files to write ref data in Reference mode
void PumaMDProcessor::OpenRefFiles() {
  // Make the directory if it does not exist
  HFSAT::FileUtils::MkdirEnclosing(DEF_PUMA_REFLOC_);
  std::string file;

  switch (bmf_feed_type_) {
    case 1:
      file = DEF_PUMA_REFLOC_;
      break;
    case 2:
      file = DEF_NTP_REFLOC_;
      break;
    case 3:
      file = DEF_NTP_ORD_REFLOC_;
      break;
  }

  if (is_full_reference_mode_ && bmf_feed_type_ == 1) {
    file = DEF_PUMA_FULL_REFLOC_;
  }

  rfile.open(file, std::ofstream::out);

  if (!rfile.is_open()) {
    fprintf(stderr, "Cannot open file %s for logging address data\n", file.c_str());
    exit(-1);
  }
  rfile << "#Numeric_Id\tInternal_Ref\t\tChannel\t\tGroup_Code" << std::endl;
}

// Load ref data from ref files
void PumaMDProcessor::LoadRefData() {
  std::string ref_file;
  switch (bmf_feed_type_) {
    case 1:
      ref_file = DEF_PUMA_REFLOC_;
      break;
    case 2:
      ref_file = DEF_NTP_REFLOC_;
      break;
    case 3:
      ref_file = DEF_NTP_ORD_REFLOC_;
      break;
  }
  if (HFSAT::kLogger == mode_ && bmf_feed_type_ == 1) ref_file = DEF_PUMA_FULL_REFLOC_;

  dfile.open(ref_file.c_str(), std::ofstream::in);

  if (!dfile.is_open()) {
    fprintf(stderr, "Cannot open file %s for reading reference data\n", ref_file.c_str());
    exit(-1);
  }

  char line[1024];
  while (!dfile.eof()) {
    memset(line, 0, sizeof(line));
    dfile.getline(line, sizeof(line));

    // Skip empty lines and comments
    if (strlen(line) == 0 || line[0] == '#') {
      continue;
    }

    uint64_t num_id = atoll(strtok(line, "\n\t "));
    char* strname = (char*)calloc(20, sizeof(char));
    strncpy(strname, strtok(NULL, "\n\t "), 20);

    if (strlen(strname) > 11) {
      // std::cout << "Ignoring ( 12+ chars ) Secname : " << strname << std::endl;
      continue;
    }

    if ((mode_ == HFSAT::kRaw || mode_ == HFSAT::kComShm) &&
        pumaLiveSource->getSecurityNameIndexer().GetIdFromSecname(strname) < 0) {
      continue;
    }

    PUMARefStruct* ref = new PUMARefStruct();
    ref_data_[num_id] = ref;

    idmap_[num_id] = strname;
    sec_to_id_map_[strname] = num_id;
    ref->secname = strname;
    ref->channel_no = atoi(strtok(NULL, "\n\t "));
    ref->rpt_seq = 0;
    ref->recovery = false;
    ref->id = num_id;
    ref->trading_status = NTP_SEC_TRADING_STATUS_CLOSE;
    ref->is_following_group = true;

    char groupname[20];
    strncpy(groupname, strtok(NULL, "\n\t "), 5);
    int group_id = groupname[0] * 256 + groupname[1];
    ref->group_id = group_id;
    rpt_seq_map_[num_id] = 0;
  }

  dfile.close();
}

// Open settlement file in write mode
void PumaMDProcessor::OpenSettlementFile() {
  // opening prices
  std::stringstream st;

  int yesterday = HFSAT::DateTime::CalcPrevWeekDay(tradingdate_);
  st << "/spare/local/files/BMF/puma-settlement_prices_" << yesterday << ".txt";
  std::string today_filename = st.str();

  // Create the directory if it does not exist already
  HFSAT::FileUtils::MkdirEnclosing(today_filename);
  yesterday_settlement_file_.open(today_filename.c_str(), std::ofstream::out | std::ofstream::app);

  if (!yesterday_settlement_file_.is_open()) {
    fprintf(stderr, "Cannot open file %s for logging address data\n", st.str().c_str());
    exit(-1);
  }
  yesterday_settlement_file_ << "#Numeric_Id\tInternal_Ref\t\tGroup_Code" << std::endl;
}

// Start Recovery for all securities
// This is to get the initial trading state - generally called only at the start
// We do not recover in between
void PumaMDProcessor::StartAllRecovery() {
  for (auto ref_pair : ref_data_) {
    StartRecovery(ref_pair.first);
  }
}

// Start recovery for specific instrument
void PumaMDProcessor::StartRecovery(uint64_t sec_id) {
  // Return if the sec_id is not present in the ref_data
  if (ref_data_.find(sec_id) == ref_data_.end()) {
    return;
  }

  PUMARefStruct* ref = ref_data_[sec_id];

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
  if (pumaLiveSource) {
    pumaLiveSource->startRecovery(ref->channel_no);
  }

  // Add to the channel_recovery set to avoid calling recovery for this channel again
  channel_recovery_.insert(ref->channel_no);
}

// End recovery for the specified instrument
void PumaMDProcessor::EndRecovery(uint64_t sec_id) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  PUMARefStruct* ref = ref_data_[sec_id];

  // Return if this isn't in recovery

  if (!ref->recovery) {
    return;
  }

  ref->recovery = false;
  num_recovery_securities_--;
  recovery_[sec_id] = false;

  // If all the securities have recovered, end recovery from all channels at once
  if (num_recovery_securities_ <= 0) {
    EndAllRecovery();
  }
}

// End recovery for all products
void PumaMDProcessor::EndAllRecovery() {
  for (auto ref_pair : ref_data_) {
    ref_pair.second->recovery = false;
    recovery_[ref_pair.first] = false;
  }

  std::cout << "Ending recovery for all products. NumSecsLeft: " << num_recovery_securities_ << std::endl;

  num_recovery_securities_ = 0;

  for (auto channel : channel_recovery_) {
    if (pumaLiveSource) {
      pumaLiveSource->endRecovery(channel);
    }
  }

  // Empty channel recovery set, so that it can be reused for recovery again
  channel_recovery_.clear();
}

PumaMDProcessor& PumaMDProcessor::SetInstance(HFSAT::MulticastSenderSocket* sock, HFSAT::NewPumaRawLiveDataSource* rls,
                                              HFSAT::FastMdConsumerMode_t mode, bool recovery_flag, int bmf_feed_type,
                                              bool is_full_reference_mode) {
  if (ins == NULL) {
    ins = new PumaMDProcessor(sock, rls, mode, recovery_flag, bmf_feed_type, is_full_reference_mode);
  }
  return *ins;
}

void PumaMDProcessor::cleanup() {
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

bool PumaMDProcessor::UpdateSeqno(uint64_t sec_id, uint32_t seq_no) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  PUMARefStruct* ref = ref_data_[sec_id];

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

void PumaMDProcessor::flushTradeQueue() {
  if (!old_trd_valid) {
    return;
  }

  assert((PUMA_MDS::msgType)old_trd_cstr_->msg_ == PUMA_MDS::PUMA_TRADE);

  gettimeofday(&(old_trd_cstr_->time_), NULL);

  switch (mode_) {
    case HFSAT::kMcast: {
      sock_->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_trd_cstr_);
      break;
    }

    case HFSAT::kComShm: {
      switch (bmf_feed_type_) {
        case 1:
          generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::BMF_EQ;
          break;
        default:
          generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NTP;
          break;
      }

      memcpy((void*)&generic_mds_message_->generic_data_.bmf_eq_data_, (void*)old_trd_cstr_,
             sizeof(NTP_MDS::NTPCommonStruct));
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_);
      if (pumaLiveSource->IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
      }
      break;
    }

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

bool PumaMDProcessor::filterQCond(char* conds, int len) {
  for (int i = 0; i < len; ++i)
    if (conds[i] == 'C' || conds[i] == 'K') return true;
  return false;
}

void PumaMDProcessor::updateSeqno(uint64_t secid, uint32_t seqno) {
  if (seqno <= rpt_seq_map_[secid]) {
    return;
  }

  // in case of old seqno (should only happen if snapshot leads
  // rpt_seq_map_[secid] ==0 means we should always start with recovery so that we can update the SecTradingStatus
  if (!recovery_[secid] && (seqno > rpt_seq_map_[secid] + 1 || rpt_seq_map_[secid] == 0)) {
    if (recovery_flag_ || rpt_seq_map_[secid] == 0)  // Always recover at start
    {
      fprintf(stderr, "dropped packet %u %u \n", rpt_seq_map_[secid], seqno);
      // further processing
      recovery_[secid] = true;
      struct timeval tv;
      gettimeofday(&tv, NULL);
      fprintf(stderr, "Starting recovery for %lu at seqno %u at time %lu \n", secid, seqno, tv.tv_sec);
      /// inform inputprocessor of drop
      StartRecovery(secid);
    } else {  // TODO remove this block of code when this logging seems unnecessary
      // Log the losses, despite recovery flag being off
      struct timeval tv;
      gettimeofday(&tv, NULL);
      fprintf(stderr, "Possible recovery for %lu at seqno %u at time %lu \n", secid, seqno, tv.tv_sec);
    }
  }
  rpt_seq_map_[secid] = seqno;
}
void PumaMDProcessor::dispatchTrade(uint64_t sec_id) {
  // Flush any book messages we might have
  // false -> non intermediate
  flushQuoteQueue(false);

  assert((PUMA_MDS::msgType)cstr->msg_ == PUMA_MDS::PUMA_TRADE);

  // dispatch the old message.
  if (old_trd_valid) {
    switch (bmf_feed_type_) {
      case 3:
        if (old_trd_sec_id == sec_id && old_trd_cstr_->data_.ntp_trds_.trd_px_ == cstr->data_.ntp_trds_.trd_px_
            /*&& old_trd_cstr_->data_.ntp_trds_.agg_side_ == cstr->data_.ntp_trds_.agg_side_*/) {
          old_trd_cstr_->data_.ntp_trds_.tot_qty_ = cstr->data_.ntp_trds_.tot_qty_;   // copy
          old_trd_cstr_->data_.ntp_trds_.trd_qty_ += cstr->data_.ntp_trds_.trd_qty_;  // increment
          return;
        }
        break;
      default:
        if (old_trd_sec_id == sec_id && old_trd_cstr_->data_.ntp_trds_.trd_px_ == cstr->data_.ntp_trds_.trd_px_) {
          old_trd_cstr_->data_.ntp_trds_.tot_qty_ = cstr->data_.ntp_trds_.tot_qty_;   // copy
          old_trd_cstr_->data_.ntp_trds_.trd_qty_ += cstr->data_.ntp_trds_.trd_qty_;  // increment
          return;
        }
        break;
    }

    flushTradeQueue();
  }

  old_trd_sec_id = sec_id;
  memcpy(old_trd_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_trd_valid = true;
}

void PumaMDProcessor::flushQuoteQueue(bool intermediate) {
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

  if (bmf_feed_type_ == 3) {
    old_cstr_->data_.ntp_ordr_.intermediate_ = intermediate;
  } else {
    old_cstr_->data_.ntp_dels_.intermediate_ = intermediate;
  }

  if (md_time_.tv_sec == 0) {
    gettimeofday(&md_time_, NULL);
  }

  old_cstr_->time_ = md_time_;

  if (!intermediate) {
    md_time_.tv_sec = 0;
  }

  switch (mode_) {
    case HFSAT::kMcast: {
      sock_->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_cstr_);
      break;
    }

    case HFSAT::kComShm: {
      switch (bmf_feed_type_) {
        case 1:
          generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::BMF_EQ;
          break;
        default:
          generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NTP;
          break;
      }
      memcpy((void*)&generic_mds_message_->generic_data_.bmf_eq_data_, (void*)old_cstr_,
             sizeof(NTP_MDS::NTPCommonStruct));
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_);
      if (pumaLiveSource->IsLocalData()) {
        HFSAT::CpucycleProfiler::GetUniqueInstance().End(4);
      }
      break;
    }

    case HFSAT::kProShm: {
      shm_writer_->Write(old_cstr_);
      break;
    }

    case HFSAT::kLogger: {
      mdsLogger.log(*old_cstr_);
      break;
    }
    case HFSAT::kReference:
    case HFSAT::kModeMax:
    default:
      break;
  }
  is_old_valid = false;
}

void PumaMDProcessor::dispatchQuote(uint64_t sec_id) {
  // flush any trade messages we might have
  flushTradeQueue();
  flushQuoteQueue(sec_id == old_sec_id);  // previous message had same sec id as this one it was intermediate, else not
  memcpy(old_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_sec_id = sec_id;
  is_old_valid = true;
}

void PumaMDProcessor::dumpSecurityDefinition(uint64_t secId, const std::string& symbol, const std::string& secGroup) {
  rfile << secId << "\t\t" << symbol << "\t\t" << pumaLiveSource->getCurrChannel() << "\t\t" << secGroup << "\n"
        << std::flush;
}

void PumaMDProcessor::seqReset() {
  if (pumaLiveSource) {
    pumaLiveSource->seqReset();
  }
}

void PumaMDProcessor::UpdateBookDelta(uint64_t sec_id, uint16_t level, uint16_t num_ords, double price, int32_t size,
                                      uint32_t seq_no, char entry_type, uint32_t update_action) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existance
  PUMARefStruct* ref = ref_data_[sec_id];

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

void PumaMDProcessor::UpdateTrade(uint64_t sec_id, int32_t size, uint64_t total_qty, uint32_t seq_no, double price,
                                  bool is_last, bool is_cross) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existance
  PUMARefStruct* ref = ref_data_[sec_id];

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

void PumaMDProcessor::UpdateOpeningPrice(uint64_t sec_id, double price, int32_t size, uint32_t seq_no,
                                         uint32_t open_close_flag) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existance
  PUMARefStruct* ref = ref_data_[sec_id];

  memset((void*)cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  cstr->msg_ = NTP_MDS::NTP_OPENPRICE;

  strcpy(cstr->data_.ntp_open.contract, ref->secname);

  cstr->data_.ntp_open.price = price;
  cstr->data_.ntp_open.size = size;
  cstr->data_.ntp_open.seq_no = seq_no;
  cstr->data_.ntp_open.theoretical = open_close_flag;

  dispatchQuote(sec_id);
}

void PumaMDProcessor::DumpClosePrice(uint64_t sec_id, double price) {}

void PumaMDProcessor::UpdateImbalance(uint64_t sec_id, int32_t size, uint32_t seq_no, char condition) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existance
  PUMARefStruct* ref = ref_data_[sec_id];

  memset((void*)cstr, 0, sizeof(NTP_MDS::NTPCommonStruct));

  cstr->msg_ = NTP_MDS::NTP_IMBALANCE;

  strcpy(cstr->data_.ntp_imbalance.contract, ref->secname);

  cstr->data_.ntp_imbalance.size = size;
  cstr->data_.ntp_imbalance.seq_no = seq_no;
  cstr->data_.ntp_imbalance.condition = condition;

  dispatchQuote(sec_id);
}

void PumaMDProcessor::UpdateSecurityTradingStatus(uint64_t sec_id, uint32_t trading_status, uint64_t open_time,
                                                  uint64_t transact_time, uint32_t trading_event, uint32_t rpt_seq) {
  // Return from here if trading_status is not valid
  if (!NTP_MDS::IsStatusValid(trading_status)) {
    return;
  }

  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existance
  PUMARefStruct* ref = ref_data_[sec_id];

  // If it's in recovery, end recovery

  if (ref->recovery) {
    EndRecovery(sec_id);
  }

  if (trading_event == 101) {
    ref->is_following_group = false;
  } else if (trading_event == 102) {
    ref->is_following_group = true;
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

void PumaMDProcessor::UpdateGroupTradingStatusRefresh(uint64_t sec_id, uint32_t trading_status, uint64_t open_time,
                                                      uint64_t transact_time, uint32_t trading_event,
                                                      uint32_t rpt_seq) {
  // It is assumed that this sec_id is already present in ref_data_ map
  // Hence not checking for existence
  PUMARefStruct* ref = ref_data_[sec_id];
  UpdateGroupTradingStatusInc(ref->group_id, trading_status, open_time, transact_time, trading_event, rpt_seq);
}

void PumaMDProcessor::UpdateGroupTradingStatusInc(int group_id, uint32_t trading_status, uint64_t open_time,
                                                  uint64_t transact_time, uint32_t trading_event, uint32_t rpt_seq) {
  // Return from here if trading_status is not valid
  if (!NTP_MDS::IsStatusValid(trading_status)) {
    return;
  }

  for (auto ref_pair : ref_data_) {
    PUMARefStruct* temp_ref = ref_pair.second;

    if (!temp_ref->is_following_group) continue;

    if (temp_ref->group_id == group_id) {
      std::cerr << "gid: " << group_id << " " << temp_ref->secname << " sid: " << temp_ref->id
                << " status: " << trading_status << " opentime: " << open_time << " transacttime: " << transact_time
                << "\n";

      UpdateSecurityTradingStatus(temp_ref->id, trading_status, open_time, transact_time, trading_event, rpt_seq);
    }
  }
}
}
