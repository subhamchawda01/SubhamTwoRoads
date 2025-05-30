/**
    \file lwfixfast/ntp_ord_md_processor.hpp

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
#include "infracore/lwfixfast/ntp_ord_md_processor.hpp"
#include "dvccode/CDef/defines.hpp"

namespace NTP_ORD_MD_PROCESSOR {

NtpOrdMdProcessor::NtpOrdMdProcessor(HFSAT::MulticastSenderSocket *sock_, HFSAT::NewNTPRawLiveDataSource *rls,
                                     HFSAT::FastMdConsumerMode_t mode_,
                                     bool recovery_flag_)
    : recovery_flag(recovery_flag_),  // always recover, since we need to update sec_trading_status at startup
      sock(sock_),
      mdsLogger("NTP_ORD"),
      ntpLiveSource(rls),
      cstr(new NTP_MDS::NTPCommonStruct()),
      old_cstr_(new NTP_MDS::NTPCommonStruct()),
      is_old_valid(false),
      old_sec_id(-1),
      old_trd_cstr_(new NTP_MDS::NTPCommonStruct()),
      old_trd_valid(false),
      old_trd_sec_id(-1),
      product_seen(),
      idmap_(),
      idchnmap_(),
      rpt_seq_map_(),
      recovery_(),
      security_trading_status_(),
      tradingdate_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
      last_midnight_sec_(HFSAT::DateTime::GetTimeMidnightUTC(tradingdate_)),
      mode(mode_),
      key(SHM_KEY_NTP_RAW),
      shmid(-1),
      ntp_shm_queue_(NULL),
      ntp_shm_queue_pointer_(NULL),
      count(0),
      last_write_seq_num_(0),
      mds_shm_interface_(nullptr),
      generic_mds_message_(new HFSAT::MDS_MSG::GenericMDSMessage()),
      sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance())

{
  if (mode == HFSAT::kModeMax) return;  // dummy arg
  if (mode_ == HFSAT::kLogger) mdsLogger.run();
  if (mode_ == HFSAT::kComShm) {
    mds_shm_interface_ = &HFSAT::Utils::MDSShmInterface::GetUniqueInstance();
  }

  md_time_.tv_sec = 0;

  if (mode == HFSAT::kReference) {
    HFSAT::FileUtils::MkdirEnclosing(DEF_NTP_ORD_REFLOC_);  // makes the directory if it does not exist
    rfile.open(DEF_NTP_ORD_REFLOC_, std::ofstream::out);
    if (!rfile.is_open()) {
      fprintf(stderr, "Cannot open file %s for logging address data\n", DEF_NTP_ORD_REFLOC_);
      exit(-1);
    }
    rfile << "##Numeric_Id\tInternal_Ref\t\tChannel\t\tGroup_Code" << endl;
  } else {
    dfile.open(DEF_NTP_ORD_REFLOC_, std::ofstream::in);

    if (!dfile.is_open()) {
      fprintf(stderr, "Cannot open file %s for reading reference data\n", DEF_NTP_ORD_REFLOC_);
      exit(-1);
    }

    char line[1024];
    while (!dfile.eof()) {
      memset(line, 0, sizeof(line));
      dfile.getline(line, sizeof(line));
      if (strlen(line) == 0 || line[0] == '#')  /// comments etc
        continue;

      uint64_t num_id = strtoul(strtok(line, "\n\t "), NULL, 0);
      char *strname = (char *)calloc(20, sizeof(char));
      strncpy(strname, strtok(NULL, "\n\t "), 20);

      if (mode_ == HFSAT::kRaw && ntpLiveSource->getSecurityNameIndexer().GetIdFromSecname(strname) < 0) continue;
      idmap_[num_id] = strname;
      idchnmap_[num_id] = atoi(strtok(NULL, "\n\t "));

      // This additional map was needed because the Security ID in a NTP Market refresh is not
      // necessarily an integer code. It may be a string.
      // In CME it is always an integer.

      sec_to_id_map_[strname] = num_id;
      rpt_seq_map_[num_id] = 0;
    }

    dfile.close();
  }

  if (mode == HFSAT::kProShm) {
    if ((shmid = shmget(key, (size_t)(NTP_RAW_SHM_QUEUE_SIZE * (sizeof(NTP_MDS::NTPCommonStruct)) + sizeof(int)),
                        IPC_CREAT | 0666)) < 0) {
      std::cout << "Size of segment = " << NTP_RAW_SHM_QUEUE_SIZE * sizeof(NTP_MDS::NTPCommonStruct)
                << " bytes key = " << key << std::endl;
      printf("Failed to shmget error = %s\n", strerror(errno));

      if (errno == EINVAL)
        printf("Invalid segment size specified\n");
      else if (errno == EEXIST)
        printf("Segment exists, cannot create it\n");
      else if (errno == EIDRM)
        printf("Segment is marked for deletion or was removed\n");
      else if (errno == ENOENT)
        printf("Segment does not exist\n");
      else if (errno == EACCES)
        printf("Permission denied\n");
      else if (errno == ENOMEM)
        printf("Not enough memory to create segment\n");

      exit(1);
    }

    if ((ntp_shm_queue_ = (volatile NTP_MDS::NTPCommonStruct *)shmat(shmid, NULL, 0)) ==
        (volatile NTP_MDS::NTPCommonStruct *)-1) {
      perror("shmat");
      exit(0);
    }

    if (shmctl(shmid, IPC_STAT, &shm_ds) == -1) {
      perror("shmctl");
      exit(1);
    }

    memset((void *)ntp_shm_queue_, 0, (NTP_RAW_SHM_QUEUE_SIZE * (sizeof(NTP_MDS::NTPCommonStruct)) + sizeof(int)));
    ntp_shm_queue_pointer_ = ntp_shm_queue_;

    shm_queue_index_ = (volatile int *)(ntp_shm_queue_ + NTP_RAW_SHM_QUEUE_SIZE);
  }
}

bool NtpOrdMdProcessor::filterQCond(char *conds, int len) {
  for (int i = 0; i < len; ++i)
    if (conds[i] == 'C' || conds[i] == 'K') return true;
  return false;
}

void NtpOrdMdProcessor::updateSeqno(uint64_t secid, uint32_t seqno) {
  if (seqno <= rpt_seq_map_[secid]) {
    return;
  }

  // in case of old seqno (should only happen if snapshot leads
  // rpt_seq_map_[secid] ==0 means we should always start with recovery so that we can update the SecTradingStatus
  if (!recovery_[secid] && (seqno > rpt_seq_map_[secid] + 1 || rpt_seq_map_[secid] == 0)) {
    if (recovery_flag || rpt_seq_map_[secid] == 0)  // Always recover at start
    {
      fprintf(stderr, "dropped packet %u %u \n", rpt_seq_map_[secid], seqno);
      // further processing
      recovery_[secid] = true;
      /// inform inputprocessor of drop
      ntpLiveSource->startRecovery(idchnmap_[secid]);
      struct timeval tv;
      gettimeofday(&tv, NULL);
      fprintf(stderr, "Starting recovery for %lu at seqno %u at time %lu \n", secid, seqno, tv.tv_sec);
    } else {  // TODO remove this block of code when this logging seems unnecessary
      // Log the losses, despite recovery flag being off
      struct timeval tv;
      gettimeofday(&tv, NULL);
      fprintf(stderr, "Possible recovery for %lu at seqno %u at time %lu \n", secid, seqno, tv.tv_sec);
    }
  }
  rpt_seq_map_[secid] = seqno;
}

void NtpOrdMdProcessor::flushTradeQueue() {
  if (!old_trd_valid) return;
  assert(old_trd_cstr_->msg_ == NTP_MDS::NTP_TRADE);

  gettimeofday(&(old_trd_cstr_->time_), NULL);

  switch (mode) {
    case HFSAT::kMcast:
      sock->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_trd_cstr_);
      break;

    case HFSAT::kComShm: {
      generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NTP;
      memcpy((void *)&(generic_mds_message_->generic_data_).ntp_data_, (void *)old_trd_cstr_,
             sizeof(NTP_MDS::NTPCommonStruct));
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_);

    } break;

    case HFSAT::kProShm: {
      memcpy((void *)ntp_shm_queue_pointer_, (void *)old_trd_cstr_, sizeof(NTP_MDS::NTPCommonStruct));

      *shm_queue_index_ = last_write_seq_num_;

      last_write_seq_num_ = (last_write_seq_num_ + 1) & (NTP_RAW_SHM_QUEUE_SIZE - 1);

      count++;

      if (last_write_seq_num_ == 0) {
        count = 0;

        ntp_shm_queue_pointer_ = ntp_shm_queue_;  // reset back to start of data

        //  shm_queue_index_ = (int*) (ntp_shm_queue_ + NTP_RAW_SHM_QUEUE_SIZE);

      } else {
        ntp_shm_queue_pointer_++;
      }

    } break;

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

void NtpOrdMdProcessor::dispatchTrade(uint64_t sec_id_) {
  // flush any book messages we might have
  flushQuoteQueue(false);  // non intermediate

  assert(cstr->msg_ == NTP_MDS::NTP_TRADE);
  if (old_trd_valid)  // dispatch the old message.
  {
    if (old_trd_sec_id == sec_id_ && old_trd_cstr_->data_.ntp_trds_.trd_px_ == cstr->data_.ntp_trds_.trd_px_
        /*&& old_trd_cstr_->data_.ntp_trds_.agg_side_ == cstr->data_.ntp_trds_.agg_side_*/) {
      old_trd_cstr_->data_.ntp_trds_.tot_qty_ = cstr->data_.ntp_trds_.tot_qty_;   // copy
      old_trd_cstr_->data_.ntp_trds_.trd_qty_ += cstr->data_.ntp_trds_.trd_qty_;  // increment
      return;
    }
    flushTradeQueue();
  }
  old_trd_sec_id = sec_id_;
  memcpy(old_trd_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_trd_valid = true;

  // Log individual trade messages for order feed, instead of logging cumulative trade sizes.
  // This is important for current order level sim market maker.
  flushTradeQueue();
}

void NtpOrdMdProcessor::flushQuoteQueue(bool intermediate) {
  if (!is_old_valid) {
    md_time_.tv_sec = 0;
    return;
  }

  old_cstr_->data_.ntp_ordr_.intermediate_ = intermediate;

  if (md_time_.tv_sec == 0) {
    gettimeofday(&md_time_, NULL);
  }

  old_cstr_->time_ = md_time_;

  if (!intermediate) {
    md_time_.tv_sec = 0;
  }

  switch (mode) {
    case HFSAT::kMcast:
      sock->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_cstr_);
      break;
    case HFSAT::kComShm: {
      generic_mds_message_->mds_msg_exch_ = HFSAT::MDS_MSG::NTP;
      memcpy((void *)&(generic_mds_message_->generic_data_).ntp_data_, (void *)old_cstr_,
             sizeof(NTP_MDS::NTPCommonStruct));
      mds_shm_interface_->WriteGenericStruct(generic_mds_message_);

    } break;

    case HFSAT::kProShm: {
      memcpy((void *)ntp_shm_queue_pointer_, (void *)old_cstr_, sizeof(NTP_MDS::NTPCommonStruct));

      *shm_queue_index_ = last_write_seq_num_;

      last_write_seq_num_ = (last_write_seq_num_ + 1) & (NTP_RAW_SHM_QUEUE_SIZE - 1);

      count++;

      if (last_write_seq_num_ == 0) {
        count = 0;

        ntp_shm_queue_pointer_ = ntp_shm_queue_;  // reset back to start of data

        // shm_queue_index_ = (int*) (ntp_shm_queue_ + NTP_RAW_SHM_QUEUE_SIZE);

      } else {
        ntp_shm_queue_pointer_++;
      }

    } break;

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

void NtpOrdMdProcessor::dispatchQuote(uint64_t sec_id_) {
  // flush any trade messages we might have
  flushTradeQueue();
  flushQuoteQueue(sec_id_ == old_sec_id);  // previous message had same sec id as this one it was intermediate, else not
  memcpy(old_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_sec_id = sec_id_;
  is_old_valid = true;
}

void NtpOrdMdProcessor::dumpSecurityDefinition(uint64_t secId, const std::string &symbol, const std::string &secGroup) {
  rfile << secId << "\t\t" << symbol << "\t\t" << ntpLiveSource->getCurrChannel() << "\t\t" << secGroup << "\n"
        << std::flush;
}

void NtpOrdMdProcessor::endRecovery(int channel_id_) { ntpLiveSource->endRecovery(channel_id_); }

void NtpOrdMdProcessor::seqReset() { ntpLiveSource->seqReset(); }
}
