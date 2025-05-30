#include "bmf_fpga_decoder.hpp"

namespace BMF_FPGA {

BMFFpgaDecoder::BMFFpgaDecoder()
    : fpga_cstr(new FPGA_MDS::BMFFPGACommonStruct()),
      mdsLogger("BMF_FPGA"),
      mode_(HFSAT::kLogger),
      shm_writer_(nullptr),
      is_cached_trade_valid_(false),
      clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
      using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()) {
  for (int i = 0; i < 16; i++) {
    powers.push_back(pow(10.0, i));
    neg_powers.push_back(pow(10.0, -i));
  }
}

void BMFFpgaDecoder::Initialize(HFSAT::FastMdConsumerMode_t mode) {
  mode_ = mode;
  switch (mode_) {
    case HFSAT::kLogger:
      mdsLogger.run();
      break;
    case HFSAT::kProShm:
      shm_writer_ = new SHM::ShmWriter<FPGA_MDS::BMFFPGACommonStruct>(SHM_KEY_BMF_FPGA, BMF_FPGA_SHM_QUEUE_SIZE);
      break;
    // Using Raw as hybrid mode as a hack
    case HFSAT::kRaw:
      shm_writer_ = new SHM::ShmWriter<FPGA_MDS::BMFFPGACommonStruct>(SHM_KEY_BMF_FPGA, BMF_FPGA_SHM_QUEUE_SIZE);
      mdsLogger.run();
      break;
    default:
      std::cerr << "Exiting...Invalid mode provided " << (int)mode_ << std::endl;
      exit(0);
      break;
  }
}

void BMFFpgaDecoder::SetTime() {
  if (true == using_simulated_clocksource_) {
    fpga_cstr->time_ = clock_source_.GetTimeOfDay();
  } else {
    gettimeofday(&(fpga_cstr->time_), NULL);
  }
}

void BMFFpgaDecoder::FlushFpgaData() {
  switch (mode_) {
    case HFSAT::kLogger:
      mdsLogger.log(*fpga_cstr);
      break;
    case HFSAT::kProShm:
      shm_writer_->Write(fpga_cstr);
      break;
    case HFSAT::kRaw:
      shm_writer_->Write(fpga_cstr);
      mdsLogger.log(*fpga_cstr);
      break;
    default:
      break;
  }
}

void BMFFpgaDecoder::ProcessReferenceData(SiliconUmdf::EventAPI::Events_t& events_) {
  std::cout << "Processing reference data on Channel: " << events_.sumdfChannelId << std::endl;
  for (auto inst_iter = events_.instrumentList->begin(); inst_iter != events_.instrumentList->end(); ++inst_iter) {
    BMFFpgaRefStruct* ref_data_ptr = new BMFFpgaRefStruct;
    ref_data_ptr->secid = inst_iter->first;
    strcpy(ref_data_ptr->secname, inst_iter->second.info.Symbol);
    char groupname[20];
    strcpy(groupname, inst_iter->second.info.SecurityGroup);
    ref_data_ptr->group_id = groupname[0] * 256 + groupname[1];
    ref_data_ptr->channel_id = events_.sumdfChannelId;
    ref_data_ptr->trading_status = events_.instrumentStatus.SecurityTradingStatus;
    ref_data_ptr->is_following_group_ = true;
    secid_refdata_map_[ref_data_ptr->secid] = ref_data_ptr;

    std::cout << "RefData: " << ref_data_ptr->ToString() << std::endl;
  }
}
void BMFFpgaDecoder::PrintReferenceData() {
  for (auto ref_data_it : secid_refdata_map_) {
    std::cout << ref_data_it.second->ToString() << std::endl;
  }
}

void BMFFpgaDecoder::SetOpenFlag(uint64_t sec_id) {
  if (secid_refdata_map_[sec_id]->trading_status == 17) {
    fpga_cstr->is_closed_ = false;
  } else {
    fpga_cstr->is_closed_ = true;
  }
}

void BMFFpgaDecoder::FlushAggregatedTrade(uint64_t sec_id) {
  if (is_cached_trade_valid_) {
    fpga_cstr->is_closed_ = false;
    FlushFpgaData();
    is_cached_trade_valid_ = false;
  }
}

void BMFFpgaDecoder::AggregateTrades(SiliconUmdf::EventAPI::Events_t& events_, unsigned int index, uint64_t sec_id) {
  int64_t mantissa = (*events_.MDEntries_PUMA_2_0.MDEntries)[index].trade.MDEntryPxMant;
  int exp = (*events_.MDEntries_PUMA_2_0.MDEntries)[index].trade.MDEntryPxExp;

  double price = (exp >= 0) ? mantissa * powers[exp] : mantissa * neg_powers[-exp];

  uint64_t size = (*events_.MDEntries_PUMA_2_0.MDEntries)[index].trade.MDEntrySize;

  uint64_t rpt_seq_num = (*events_.MDEntries_PUMA_2_0.MDEntries)[index].trade.RptSeq;

  if (!is_cached_trade_valid_) {
    fpga_cstr->data_.fpga_trds_.rpt_seq_num = rpt_seq_num;
    fpga_cstr->data_.fpga_trds_.size = size;
    fpga_cstr->data_.fpga_trds_.price = price;
    is_cached_trade_valid_ = true;
  } else {
    // If price is same aggregate
    if (fabs(fpga_cstr->data_.fpga_trds_.price - price) < 0.00001) {
      fpga_cstr->data_.fpga_trds_.size += size;
    } else {
      FlushAggregatedTrade(sec_id);
      fpga_cstr->data_.fpga_trds_.rpt_seq_num = rpt_seq_num;
      fpga_cstr->data_.fpga_trds_.size = size;
      fpga_cstr->data_.fpga_trds_.price = price;
      is_cached_trade_valid_ = true;
    }
  }
}

void BMFFpgaDecoder::ProcessMDEntries(SiliconUmdf::EventAPI::Events_t& events_) {
#if BMF_FPGA_DEBUG_MODE
  std::cout << "Processing MD Entries " << std::endl;
#endif

  SiliconUmdf::EventAPI::Instrument_t* instrument = events_.bookAndInstrumentInfo;

  uint64_t sec_id = instrument->info.SecurityID;

  fpga_cstr->msg_ = FPGA_MDS::BMF_FPGA_TRADE;
  SetTime();
  strcpy(fpga_cstr->data_.fpga_dels_.contract_, instrument->info.Symbol);

  for (unsigned int index = 0; index < events_.MDEntries_PUMA_2_0.NoMDEntries; ++index) {
    // Only processing md_entries of type trade
    if ((*events_.MDEntries_PUMA_2_0.MDEntries)[index].MDEntryType == '2') {
      // Ignoring crossed trades
      if ((*events_.MDEntries_PUMA_2_0.MDEntries)[index].trade.TradeCondition[0] == 'X') {
        continue;
      }

      // Ignoring trade bust
      if ((*events_.MDEntries_PUMA_2_0.MDEntries)[index].MDUpdateAction == 2) {
        continue;
      }

#if BMF_FPGA_DEBUG_MODE
      std::cout << "Processing FPGA Trade messages " << std::endl;
      std::cout << "Exponent = " << ((*events_.MDEntries_PUMA_2_0.MDEntries)[index].trade.MDEntryPxMant) << std::endl;
      std::cout << "Mantissa = " << (*events_.MDEntries_PUMA_2_0.MDEntries)[index].trade.MDEntryPxExp << std::endl;
#endif

      AggregateTrades(events_, index, sec_id);

#if BMF_FPGA_DEBUG_MODE
      std::cout << fpga_cstr->ToString() << std::endl;
#endif
    }
  }

  FlushAggregatedTrade(sec_id);

#if BMF_FPGA_DEBUG_MODE
  std::cout << "Processed MD Entries " << std::endl;
#endif
}

void BMFFpgaDecoder::ProcessBook(SiliconUmdf::EventAPI::Events_t& events_) {
  SiliconUmdf::EventAPI::Instrument_t* instrument = events_.bookAndInstrumentInfo;

#if BMF_FPGA_DEBUG_MODE
  std::cout << "----- Processing FPGA Book Entries -------" << std::endl;
  std::cout << "Symbol : " << instrument->info.Symbol << " BidSize: " << instrument->book->bid.size()
            << " AskSize: " << instrument->book->offer.size() << " MarketDepth: " << instrument->info.MarketDepth
            << std::endl;
#endif

  if (instrument->book == NULL) {
    std::cerr << "Book is NULL for SecurityID = " << instrument->info.SecurityID << std::endl;
    return;
  }

  bool is_empty = (instrument->book->bid.size() == 0) && (instrument->book->offer.size() == 0);
  if (is_empty) {
    return;
  }

  SetTime();
  strcpy(fpga_cstr->data_.fpga_dels_.contract_, instrument->info.Symbol);
  fpga_cstr->msg_ = FPGA_MDS::BMF_FPGA_BOOK;

  for (int index = 0, bid_index = -1; index < (int)instrument->book->bid.size() && bid_index < BMF_FPGA_MAX_BOOK_SIZE;
       ++index) {
    double current_bid_price =
        (instrument->book->bid[index]->MDEntryPxExp >= 0)
            ? instrument->book->bid[index]->MDEntryPxMant * powers[instrument->book->bid[index]->MDEntryPxExp]
            : instrument->book->bid[index]->MDEntryPxMant * neg_powers[-(instrument->book->bid[index]->MDEntryPxExp)];

    ++bid_index;
    FPGA_MDS::BMFFPGABookEntry& book_entry = fpga_cstr->data_.fpga_dels_.bids[bid_index];
    book_entry.size = instrument->book->bid[index]->MDEntrySize;
    book_entry.number_of_orders = instrument->book->bid[index]->NumberOfOrders;
    book_entry.price = current_bid_price;
    fpga_cstr->data_.fpga_dels_.bid_size = bid_index + 1;
  }

  for (int index = 0, ask_index = -1; index < (int)instrument->book->offer.size() && ask_index < BMF_FPGA_MAX_BOOK_SIZE;
       ++index) {
    double current_ask_price =
        (instrument->book->offer[index]->MDEntryPxExp >= 0)
            ? instrument->book->offer[index]->MDEntryPxMant * powers[instrument->book->offer[index]->MDEntryPxExp]
            : instrument->book->offer[index]->MDEntryPxMant *
                  neg_powers[abs(instrument->book->offer[index]->MDEntryPxExp)];

    ++ask_index;
    FPGA_MDS::BMFFPGABookEntry& book_entry = fpga_cstr->data_.fpga_dels_.offers[ask_index];
    book_entry.size = instrument->book->offer[index]->MDEntrySize;
    book_entry.number_of_orders = instrument->book->offer[index]->NumberOfOrders;
    book_entry.price = current_ask_price;
    fpga_cstr->data_.fpga_dels_.ask_size = ask_index + 1;
  }

  SetOpenFlag(instrument->info.SecurityID);

  FlushFpgaData();
#if BMF_FPGA_DEBUG_MODE
  std::cout << fpga_cstr->ToString() << std::endl;
  std::cout << "------ Processed FPGA Book ------ " << std::endl;
#endif
}

void BMFFpgaDecoder::ProcessInstrumentStatus(SiliconUmdf::EventAPI::Events_t& events_) {
#if BMF_FPGA_DEBUG_MODE
  std::cout << "Processing FPGA Trading Status " << std::endl;
#endif
  fpga_cstr->msg_ = FPGA_MDS::BMF_FPGA_TRADING_STATUS;
  SetTime();

  uint32_t trading_status = events_.instrumentStatus.SecurityTradingStatus;
  uint32_t sec_trading_event = events_.instrumentStatus.SecurityTradingEvent;
  uint64_t sec_id = events_.bookAndInstrumentInfo->info.SecurityID;

  strcpy(fpga_cstr->data_.fpga_dels_.contract_, events_.bookAndInstrumentInfo->info.Symbol);
  fpga_cstr->data_.fpga_status_.status = trading_status;

  if (secid_refdata_map_.find(sec_id) != secid_refdata_map_.end()) {
    secid_refdata_map_[sec_id]->trading_status = trading_status;
    // Not following group status
    if (sec_trading_event == 101) {
      secid_refdata_map_[sec_id]->is_following_group_ = false;
    } else if (sec_trading_event == 102) {
      secid_refdata_map_[sec_id]->is_following_group_ = true;
    }
  }
  FlushFpgaData();
#if BMF_FPGA_DEBUG_MODE
  std::cout << fpga_cstr->ToString() << std::endl;
  std::cout << "------ Processed FPGA Trading Status  ------ " << std::endl;
#endif
}

void BMFFpgaDecoder::ProcessGroupStatus(SiliconUmdf::EventAPI::Events_t& events_) {
#if BMF_FPGA_DEBUG_MODE
  std::cout << "Processing FPGA Group Trading Status " << std::endl;
#endif

  char* group = events_.groupStatus.SecurityGroup;
  uint64_t group_id = group[0] * 256 + group[1];

  uint32_t trading_status = events_.groupStatus.TradingSessionSubID;

  fpga_cstr->msg_ = FPGA_MDS::BMF_FPGA_TRADING_STATUS;
  SetTime();
  fpga_cstr->data_.fpga_status_.status = trading_status;

  for (auto ref_pair : secid_refdata_map_) {
    BMFFpgaRefStruct* temp_ref = ref_pair.second;

    if (!temp_ref->is_following_group_) continue;

    if (temp_ref->group_id == group_id) {
      std::cout << "GroupID: " << group_id << " " << temp_ref->secname << " SecurityID: " << temp_ref->secid
                << " TradingStatus: " << trading_status << std::endl;

      temp_ref->trading_status = trading_status;
      strcpy(fpga_cstr->data_.fpga_dels_.contract_, temp_ref->secname);
      FlushFpgaData();
    }
  }

#if BMF_FPGA_DEBUG_MODE
  std::cout << fpga_cstr->ToString() << std::endl;
  std::cout << "------ Processed FPGA Group Trading Status  ------ " << std::endl;
#endif
}
}
