/**
    \file Ntp_md_processor.hpp

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
#include "infracore/lwfixfast/bmf_md_processor.hpp"
#include "dvccode/CDef/defines.hpp"

namespace BMF_MD_PROCESSOR {

BmfMDProcessor::BmfMDProcessor(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                               HFSAT::FastMdConsumerMode_t mode_, bool recovery_flag_)
    : mode(mode_),
      recovery_flag(recovery_flag_),
      sock(sock_),
      mdsLogger("BMF"),
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
      id2OpenPrice_(),
      id2SettlementPrice_(),
      id2ClosingPrice_(),
      recovery_(),
      ntpCstrQ(),
      waitQMap() {
  if (mode == HFSAT::kModeMax) return;  // dummy arg
  if (mode_ == HFSAT::kLogger) mdsLogger.run();
  if (mode == HFSAT::kReference) {
    HFSAT::FileUtils::MkdirEnclosing(DEF_BMF_REFLOC_);  // makes the directory if it does not exist
    rfile.open(DEF_BMF_REFLOC_, std::ofstream::out);
    if (!rfile.is_open()) {
      fprintf(stderr, "Cannot open file %s for logging address data\n", DEF_BMF_REFLOC_);
      exit(-1);
    }
    rfile << "##Numeric_Id\tInternal_Ref\t\tChannel\t\tGroup_Code" << endl;
  }
  if (mode != HFSAT::kReference) {
    dfile.open(DEF_BMF_REFLOC_, std::ofstream::in);
    if (!dfile.is_open()) {
      fprintf(stderr, "Cannot open file %s for reading reference data\n", DEF_BMF_REFLOC_);
      exit(-1);
    }
    char line[1024];
    int implicit_sec_id_ =
        0;  // used when securityId is string in the ref file (this is always the case for equities data)
    while (!dfile.eof()) {
      memset(line, 0, sizeof(line));
      dfile.getline(line, sizeof(line));
      if (strlen(line) == 0 || line[0] == '#')  /// comments etc
        continue;
      char* secIdStr = strtok(line, "\n\t ");
      int num_id = atoi(secIdStr);
      if (num_id == 0) num_id = ++implicit_sec_id_;
      char* strname = (char*)calloc(20, sizeof(char));
      strncpy(strname, strtok(NULL, "\n\t "), 20);
      if ((mode_ == HFSAT::kRaw || mode_ == HFSAT::kComShm) &&
          ntpLiveSource->getSecurityNameIndexer().GetIdFromSecname(strname) < 0) {
        continue;
      }
      idmap_[num_id] = strname;
      idchnmap_[num_id] = atoi(strtok(NULL, "\n\t "));

      // This additional map was needed because the Security ID in a NTP Market refresh is not
      // necessarily an integer code. It may be a string.
      // In CME it is always an integer.

      sec_to_id_map_[secIdStr] = num_id;
      rpt_seq_map_[num_id] = 0;
    }

    dfile.close();
  }
}

BmfMDProcessor* BmfMDProcessor::ins = NULL;

BmfMDProcessor& BmfMDProcessor::SetInstance(HFSAT::MulticastSenderSocket* sock_, HFSAT::NewNTPRawLiveDataSource* rls,
                                            HFSAT::FastMdConsumerMode_t mode, bool recovery_flag) {
  if (ins == NULL) ins = new BmfMDProcessor(sock_, rls, mode, recovery_flag);
  return *ins;
}

void BmfMDProcessor::cleanup() {
  if (rfile.is_open()) {
    rfile.close();
    std::cerr << "closed reference file\n";
  }
  if (mdsLogger.isRunning()) mdsLogger.stop();

  if (NULL != ins) {
    delete ins;
    ins = NULL;
  }
}

bool BmfMDProcessor::filterQCond(char* conds, int len) {
  for (int i = 0; i < len; ++i)
    if (conds[i] == 'C' || conds[i] == 'K') return true;
  return false;
}

void BmfMDProcessor::updateSeqno(uint32_t secid, uint32_t seqno) {
  if (seqno <= rpt_seq_map_[secid]) return;

  // in case of old seqno ( should only happen if snapshot leads
  // incremental message should not be sent .. move all bcast to
  // common function and put check there
  if ((!recovery_[secid]) && (seqno > rpt_seq_map_[secid] + 1)) {
    if (recovery_flag) {
      fprintf(stderr, "dropped packet %u %u \n", rpt_seq_map_[secid], seqno);
      // further processing
      recovery_[secid] = true;
      /// inform inputprocessor of drop
      ntpLiveSource->startRecovery(idchnmap_[secid]);
      struct timeval tv;
      gettimeofday(&tv, NULL);
      fprintf(stderr, "Starting recovery for %s (%u) at seqno %u at time %ld \n", idmap_[secid].c_str(), secid, seqno,
              tv.tv_sec);
    } else {  // TODO remove this block of code when this logging seems unnecessary
      // Log the losses, despite recovery flag being off
      struct timeval tv;
      gettimeofday(&tv, NULL);
      fprintf(stderr, "Possible recovery for %s (%u) at seqno %u at time %ld \n", idmap_[secid].c_str(), secid, seqno,
              tv.tv_sec);
    }
  }

  rpt_seq_map_[secid] = seqno;  // update seqnum no matter what
}

void BmfMDProcessor::flushTradeQueue() {
  if (!old_trd_valid) return;
  assert(old_trd_cstr_->msg_ == NTP_MDS::NTP_TRADE);

  gettimeofday(&(old_trd_cstr_->time_), NULL);

  //   for debug
  //    fprintf(stderr, "%s", old_trd_cstr_->ToString().c_str());
  //    old_trd_valid = false;
  //    return;

  switch (mode) {
    case HFSAT::kMcast:
      sock->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_trd_cstr_);
      break;
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

void BmfMDProcessor::dispatchTrade(uint64_t sec_id_) {
  // flush any book messages we might have
  flushQuoteQueue(false);  // non intermediate

  assert(cstr->msg_ == NTP_MDS::NTP_TRADE);
  if (old_trd_valid)  // dispatch the old message.
  {
    if (old_trd_sec_id == sec_id_ && old_trd_cstr_->data_.ntp_trds_.trd_px_ == cstr->data_.ntp_trds_.trd_px_
        /*&& old_trd_cstr_->data_.ntp_trds_.agg_side_ == cstr->data_.ntp_trds_.agg_side_*/) {
      old_trd_cstr_->data_.ntp_trds_.tot_qty_ = cstr->data_.ntp_trds_.tot_qty_;   // copy
      old_trd_cstr_->data_.ntp_trds_.trd_qty_ += cstr->data_.ntp_trds_.trd_qty_;  // increment
      old_trd_cstr_->data_.ntp_trds_.is_last_ = cstr->data_.ntp_trds_.is_last_;
      return;
    }
    flushTradeQueue();
  }
  old_trd_sec_id = sec_id_;
  memcpy(old_trd_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_trd_valid = true;
}

void BmfMDProcessor::dispatchQuote(NTP_MDS::NTPCommonStruct* ntp_cstr) {
  //    for debug
  //    fprintf(stderr, "%s", ntp_cstr->ToString().c_str());
  //    return;

  switch (mode) {
    case HFSAT::kMcast:
      sock->WriteN(sizeof(NTP_MDS::NTPCommonStruct), ntp_cstr);
      break;
    case HFSAT::kLogger:
      mdsLogger.log(*ntp_cstr);
      break;
    case HFSAT::kReference:
    case HFSAT::kModeMax:
    default:
      break;
  }
}
void BmfMDProcessor::flushQuoteQueue(bool intermediate) {
  if (!is_old_valid) return;

  old_cstr_->data_.ntp_ordr_.intermediate_ = intermediate;

  if (!intermediate) {
    gettimeofday(&(old_cstr_->time_), NULL);
  }

  //    for debug
  //    fprintf(stderr, "%s", old_cstr_->ToString().c_str());
  //    is_old_valid = false;
  //    return;

  switch (mode) {
    case HFSAT::kMcast:
      sock->WriteN(sizeof(NTP_MDS::NTPCommonStruct), old_cstr_);
      break;
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

void BmfMDProcessor::dispatchQuote(uint64_t sec_id_) {
  // flush any trade messages we might have
  flushTradeQueue();
  flushQuoteQueue(sec_id_ == old_sec_id);  // previous message had same sec id as this one it was intermediate, else not
  memcpy(old_cstr_, cstr, sizeof(NTP_MDS::NTPCommonStruct));
  old_sec_id = sec_id_;
  is_old_valid = true;
}

void BmfMDProcessor::dumpSecurityDefinition(const std::string& secId, const std::string& symbol) {
  rfile << secId << "\t\t" << symbol << "\t\t" << ntpLiveSource->getCurrChannel() << "\n";
}

void BmfMDProcessor::endRecovery(int channel_id_) { ntpLiveSource->endRecovery(channel_id_); }

NtpCstrQ* BmfMDProcessor::getWaitQ(uint64_t secId) {
  if (waitQMap.find(secId) == waitQMap.end()) {
    waitQMap[secId] = new NtpCstrQ(secId, 100);
  }
  return waitQMap[secId];
}

void BmfMDProcessor::clearWaitQ(uint64_t secId) {
  NtpCstrQ* q = getWaitQ(secId);
  int sz = q->getSize();
  if (sz == 0) return;
  fprintf(stderr, "flushing wait queue for security %d between seqnum (%d, %d). max_rpt_seq %d\n", (unsigned)secId,
          (unsigned)q->getMinSeq(), (unsigned)q->getMaxSeq(), (unsigned)max_rpt_seq);
  NTP_MDS::NTPCommonStruct* list = q->getList();
  for (int i = 0; i < sz; ++i) {
    if ((list + i)->msg_ == NTP_MDS::NTP_TRADE) {
      if ((list + i)->data_.ntp_trds_.seqno_ <= max_rpt_seq) continue;
      memcpy(cstr, list + i, sizeof(NTP_MDS::NTPCommonStruct));
      dispatchTrade(secId);
    } else if ((list + i)->msg_ == NTP_MDS::NTP_ORDER) {
      if ((list + i)->data_.ntp_ordr_.seqno_ <= max_rpt_seq) continue;
      memcpy(cstr, list + i, sizeof(NTP_MDS::NTPCommonStruct));
      dispatchQuote(secId);
    }
  }
  rpt_seq_map_[secId] = q->getMaxSeq();
  q->clear();
  flushQuoteQueue(false);
  flushTradeQueue();
}

void BmfMDProcessor::seqReset() { ntpLiveSource->seqReset(); }
}
