/**
    \file ntp_md_queue.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#pragma once

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "dvccode/CDef/email_utils.hpp"

// a class to store ntp_commonstruct
// functionality to sort based on seq no

struct Comparator {
  bool operator()(const NTP_MDS::NTPCommonStruct& a, const NTP_MDS::NTPCommonStruct& b) {
    return (a.data_.ntp_ordr_.seqno_ < b.data_.ntp_ordr_.seqno_);
  }
};

class NtpCstrQ {
#define Ntp_CSTR_Q_SIZE 1600000
  int capacity;
  NTP_MDS::NTPCommonStruct* list;
  int curr_indx;

  uint32_t last_rpt_seq;
  uint32_t min_rpt_seq;
  uint32_t max_rpt_seq;

  uint64_t sec_id;  // only for logging purpose! //TODO remove it later

  void double_capacity() {
    NTP_MDS::NTPCommonStruct* new_list =
        (NTP_MDS::NTPCommonStruct*)calloc(std::min(capacity * 2, Ntp_CSTR_Q_SIZE), sizeof(NTP_MDS::NTPCommonStruct));
    for (int i = 0; new_list == NULL && i < 3; ++i) {  // repeat at most thrice if calloc fails
      new_list =
          (NTP_MDS::NTPCommonStruct*)calloc(std::min(capacity * 2, Ntp_CSTR_Q_SIZE), sizeof(NTP_MDS::NTPCommonStruct));
    }
    if (new_list == NULL) {
      std::cerr << "failed to allocate memory in ntp_md_queue.hpp for new list of size "
                << (std::min(capacity * 2, Ntp_CSTR_Q_SIZE)) << "hence exiting\n";
      exit(1);
    }
    memcpy(new_list, list, capacity * sizeof(NTP_MDS::NTPCommonStruct));
    free(list);
    capacity = std::min(capacity * 2, Ntp_CSTR_Q_SIZE);
    list = new_list;
  }

 public:
  NtpCstrQ(uint64_t sec_id_ = -1, int capacity_ = Ntp_CSTR_Q_SIZE)
      : capacity(capacity_),
        list((NTP_MDS::NTPCommonStruct*)calloc(capacity, sizeof(NTP_MDS::NTPCommonStruct))),
        curr_indx(0),
        last_rpt_seq(0),
        min_rpt_seq(-1),
        max_rpt_seq(0),
        sec_id(sec_id_) {}

  uint32_t getMinSeq() {
    if (curr_indx == 0) return 0;
    return min_rpt_seq;
  }

  uint64_t getMaxSeq() {
    if (curr_indx == 0) return 0;
    return max_rpt_seq;
  }

  void updateSeqNum(uint32_t seqnum) {
    // already seen seq-num. should not happen unless we are listening to 2 both primary and secondary feeds
    if (seqnum <= last_rpt_seq) return;

    // The purpose of the 2nd comparison is to ensure that we correctly process the first message in the wait queue
    // note that 2nd comparison is made with last_rpt_seq and not curr_indx > 0
    // this is because updateSeqNum can be called without add being called, as the received message might be filtered
    // for example due to bad trade or bad quote condition. A non zero last_rpt_seq means it is valid
    if (seqnum > last_rpt_seq + 1 && last_rpt_seq != 0) {
      fprintf(stderr, "non contiguous rpt seq in wait q for incremental data inst: %ull, last_seq: %d, curr_seq: %d \n",
              (unsigned)sec_id, last_rpt_seq, seqnum);
      // discard all previous queues messages
      clear();  // since we have already lost some message, simply clear all the past ones
    }
    last_rpt_seq = seqnum;
    min_rpt_seq = std::min(min_rpt_seq, last_rpt_seq);
    max_rpt_seq = std::max(max_rpt_seq, last_rpt_seq);
  }

  bool add(NTP_MDS::NTPCommonStruct* cstr) {
    memcpy(list + curr_indx, cstr, sizeof(NTP_MDS::NTPCommonStruct));
    ++curr_indx;
    if (curr_indx == capacity) {
      if (curr_indx < Ntp_CSTR_Q_SIZE) {
        double_capacity();
        fprintf(stderr, "doubling waitQ capacity for sec %u to %d\n", (unsigned)sec_id, capacity);
      } else {
        fprintf(stderr, "queue size of ntp_cstr reached limit for sec %u\n", (unsigned)sec_id);
        HFSAT::Email m;
        m.setSubject("NTP_MD_Q reached limit");
        m.addSender("nseall@tworoads.co.in");
        m.addRecepient("nseall@tworoads.co.in");
        m.content_stream << "queue size of ntp_cstr reached limit for sec " << sec_id << ".. Max size is "
                         << Ntp_CSTR_Q_SIZE << ".. Program will exit\n";
        m.sendMail();
        exit(1);
        return false;
      }
    }
    return true;
  }

  void clear() {
    curr_indx = 0;
    min_rpt_seq = -1;
    max_rpt_seq = 0;
    last_rpt_seq = 0;
  }

  int getSize() { return curr_indx; }

  NTP_MDS::NTPCommonStruct* getSorted() {
    // std::sort(list, list+curr_indx, Comparator());
    return list;
  }

  NTP_MDS::NTPCommonStruct* getList() { return list; }
};
