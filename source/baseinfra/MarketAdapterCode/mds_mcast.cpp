/**
   \file MarketAdapter/mds_mcast.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <string>

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "baseinfra/MarketAdapter/mds_mcast.hpp"

namespace HFSAT {

MDSMcast::MDSMcast(std::string ip, int port, std::string iface, int usecs_sleep)
    : usecs_sleep_(usecs_sleep), last_data_time_(0, 0) {
  socket_ = new HFSAT::MulticastSenderSocket(ip, port, iface);
}

void MDSMcast::SetTimestampAndMulticast(char* buffer, int final_len, HFSAT::MDS_MSG::MDSMessageExchType exch_type,
                                        void* ptr_to_price_level_update, int length_of_bytes, int exch_type_sz,
                                        int data_tv_sec, int data_tv_usec) {
  if (last_data_time_.tv_sec == 0) {
    memcpy(buffer, &exch_type, exch_type_sz);
    memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
    socket_->WriteN(final_len, buffer);
  } else {
    uint64_t diff = (data_tv_sec - last_data_time_.tv_sec) * 1000000 + (data_tv_usec - last_data_time_.tv_usec);
    int secs = diff / 1000000;
    int usecs = diff % 1000000;

    if (secs > 0) {
      sleep(secs);
    }
    usleep(usecs);

    memcpy(buffer, &exch_type, exch_type_sz);
    memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
    socket_->WriteN(final_len, buffer);
  }
  last_data_time_.tv_sec = data_tv_sec;
  last_data_time_.tv_usec = data_tv_usec;
}

void MDSMcast::OnPriceLevelUpdate(void* ptr_to_price_level_update, int length_of_bytes,
                                  HFSAT::MDS_MSG::MDSMessageExchType exch_type) {
  int exch_type_sz = sizeof(HFSAT::MDS_MSG::MDSMessageExchType);
  int final_len = length_of_bytes + exch_type_sz;
  char buffer[final_len];

  if (exch_type == HFSAT::MDS_MSG::NSE && usecs_sleep_ == -1) {
    NSE_MDS::NSETBTDataCommonStruct* data =
        reinterpret_cast<NSE_MDS::NSETBTDataCommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->source_time.tv_sec, data->source_time.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::NTP && usecs_sleep_ == -1) {
    NTP_MDS::NTPCommonStruct* data = reinterpret_cast<NTP_MDS::NTPCommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::ASX && usecs_sleep_ == -1) {
    ASX_MDS::ASXPFCommonStruct* data = reinterpret_cast<ASX_MDS::ASXPFCommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::ICE && usecs_sleep_ == -1) {
    ICE_MDS::ICECommonStruct* data = reinterpret_cast<ICE_MDS::ICECommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::EOBI_PF && usecs_sleep_ == -1) {
    EUREX_MDS::EUREXCommonStruct* data = reinterpret_cast<EUREX_MDS::EUREXCommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::CSM && usecs_sleep_ == -1) {
    CSM_MDS::CSMCommonStruct* data = reinterpret_cast<CSM_MDS::CSMCommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::LIFFE && usecs_sleep_ == -1) {
    LIFFE_MDS::LIFFECommonStruct* data = reinterpret_cast<LIFFE_MDS::LIFFECommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::SGX && usecs_sleep_ == -1) {
    SGX_MDS::SGXPFCommonStruct* data = reinterpret_cast<SGX_MDS::SGXPFCommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }
  if (exch_type == HFSAT::MDS_MSG::TMX_OBF && usecs_sleep_ == -1) {
    TMX_OBF_MDS::TMXPFCommonStruct* data = reinterpret_cast<TMX_OBF_MDS::TMXPFCommonStruct*>(ptr_to_price_level_update);
    SetTimestampAndMulticast(buffer, final_len, exch_type, ptr_to_price_level_update, length_of_bytes, exch_type_sz,
                             data->time_.tv_sec, data->time_.tv_usec);
  }

  if (exch_type == HFSAT::MDS_MSG::RTS && usecs_sleep_ == -1) {
    RTS_MDS::RTSCommonStruct* data = reinterpret_cast<RTS_MDS::RTSCommonStruct*>(ptr_to_price_level_update);

    if (last_data_time_.tv_sec == 0) {
      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    } else {
      uint64_t diff =
          (data->time_.tv_sec - last_data_time_.tv_sec) * 1000000 + (data->time_.tv_usec - last_data_time_.tv_usec);
      int secs = diff / 1000000;
      int usecs = diff % 1000000;

      if (secs > 0) {
        sleep(secs);
      }
      usleep(usecs);

      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    }
    last_data_time_.tv_sec = data->time_.tv_sec;
    last_data_time_.tv_usec = data->time_.tv_usec;
  }

  if (exch_type == HFSAT::MDS_MSG::MICEX && usecs_sleep_ == -1) {
    MICEX_MDS::MICEXCommonStruct* data = reinterpret_cast<MICEX_MDS::MICEXCommonStruct*>(ptr_to_price_level_update);

    if (last_data_time_.tv_sec == 0) {
      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    } else {
      uint64_t diff =
          (data->time_.tv_sec - last_data_time_.tv_sec) * 1000000 + (data->time_.tv_usec - last_data_time_.tv_usec);
      int secs = diff / 1000000;
      int usecs = diff % 1000000;

      if (secs > 0) {
        sleep(secs);
      }
      usleep(usecs);

      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    }
    last_data_time_.tv_sec = data->time_.tv_sec;
    last_data_time_.tv_usec = data->time_.tv_usec;
  }

  if (exch_type == HFSAT::MDS_MSG::CME && usecs_sleep_ == -1) {
    CME_MDS::CMECommonStruct* data = reinterpret_cast<CME_MDS::CMECommonStruct*>(ptr_to_price_level_update);

    if (last_data_time_.tv_sec == 0) {
      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    } else {
      uint64_t diff =
          (data->time_.tv_sec - last_data_time_.tv_sec) * 1000000 + (data->time_.tv_usec - last_data_time_.tv_usec);
      int secs = diff / 1000000;
      int usecs = diff % 1000000;

      if (secs > 0) {
        sleep(secs);
      }
      usleep(usecs);

      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    }
    last_data_time_.tv_sec = data->time_.tv_sec;
    last_data_time_.tv_usec = data->time_.tv_usec;
  }

  if (exch_type == HFSAT::MDS_MSG::HKOMDPF && usecs_sleep_ == -1) {
    HKOMD_MDS::HKOMDPFCommonStruct* data = reinterpret_cast<HKOMD_MDS::HKOMDPFCommonStruct*>(ptr_to_price_level_update);

    if (last_data_time_.tv_sec == 0) {
      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    } else {
      uint64_t diff =
          (data->time_.tv_sec - last_data_time_.tv_sec) * 1000000 + (data->time_.tv_usec - last_data_time_.tv_usec);
      int secs = diff / 1000000;
      int usecs = diff % 1000000;

      if (secs > 0) {
        sleep(secs);
      }
      usleep(usecs);

      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    }
    last_data_time_.tv_sec = data->time_.tv_sec;
    last_data_time_.tv_usec = data->time_.tv_usec;
  }

  if (exch_type == HFSAT::MDS_MSG::OSE_ITCH_PF && usecs_sleep_ == -1) {
    OSE_ITCH_MDS::OSEPFCommonStruct* data =
        reinterpret_cast<OSE_ITCH_MDS::OSEPFCommonStruct*>(ptr_to_price_level_update);

    if (last_data_time_.tv_sec == 0) {
      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    } else {
      uint64_t diff =
          (data->time_.tv_sec - last_data_time_.tv_sec) * 1000000 + (data->time_.tv_usec - last_data_time_.tv_usec);
      int secs = diff / 1000000;
      int usecs = diff % 1000000;

      if (secs > 0) {
        sleep(secs);
      }
      usleep(usecs);

      memcpy(buffer, &exch_type, exch_type_sz);
      memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
      socket_->WriteN(final_len, buffer);
    }
    last_data_time_.tv_sec = data->time_.tv_sec;
    last_data_time_.tv_usec = data->time_.tv_usec;
  }

  if (usecs_sleep_ >= 0) {
    memcpy(buffer, &exch_type, exch_type_sz);
    memcpy(buffer + exch_type_sz, ptr_to_price_level_update, length_of_bytes);
    socket_->WriteN(final_len, buffer);

    if (usecs_sleep_ > 0) {
      usleep(usecs_sleep_);
    }
  }
}
}
