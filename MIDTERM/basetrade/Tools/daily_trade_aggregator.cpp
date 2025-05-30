/**
   \file Tools/mds_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/bmf_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ice_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/rts_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eobi_price_feed_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/book_init_utils.hpp"

template <class T>
class MDSLogReader {
 public:
  // static int getTradeVol(const T& msg, bool& isTrade );
  uint32_t static getTradeVol(const EUREX_MDS::EUREXCommonStruct& msg, bool& isTrade) {
    isTrade = msg.msg_ == EUREX_MDS::EUREX_TRADE;
    if (!isTrade) return 0;
    return msg.data_.eurex_trds_.trd_qty_;
  }

  uint32_t static getTradeVol(const CME_MDS::CMECommonStruct& msg, bool& isTrade) {
    isTrade = msg.msg_ == CME_MDS::CME_TRADE;
    if (!isTrade) return 0;
    return msg.data_.cme_trds_.trd_qty_;
  }

  uint32_t static getTradeVol(const BMF_MDS::BMFCommonStruct& msg, bool& isTrade) {
    isTrade = msg.msg_ == BMF_MDS::BMF_TRADE;
    if (!isTrade) return 0;
    return msg.data_.bmf_trds_.trd_qty_;
  }

  uint32_t static getTradeVol(const NTP_MDS::NTPCommonStruct& msg, bool& isTrade) {
    isTrade = msg.msg_ == NTP_MDS::NTP_TRADE;
    if (!isTrade) return 0;
    return msg.data_.ntp_trds_.trd_qty_;
  }

  uint32_t static getTradeVol(const TMX_MDS::TMXCommonStruct& msg, bool& isTrade) {
    isTrade = msg.msg_ == TMX_MDS::TMX_TRADE;
    if (!isTrade) return 0;
    return msg.data_.tmx_trds_.trd_qty_;
  }

  uint32_t static getTradeVol(const LIFFE_MDS::LIFFECommonStruct& msg, bool& isTrade) {
    isTrade = msg.msg_ == LIFFE_MDS::LIFFE_TRADE;
    if (!isTrade) return 0;
    return msg.data_.liffe_trds_.trd_qty_;
  }

  uint32_t static getTradeVol(const ICE_MDS::ICECommonStruct& msg, bool& isTrade) {
    isTrade = msg.msg_ == ICE_MDS::ICE_TRADE;
    if (!isTrade) return 0;
    return msg.data_.ice_trds_.size_;
  }

  uint32_t static getTradeVol(const HKEX_MDS::HKEXCommonStruct& next_event_, bool& isTrade) {
    if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
      isTrade = true;
      return next_event_.data_.hkex_trds_.trd_qty_;
    }
    return 0;
  }

  uint32_t static getTradeVol(const OSE_MDS::OSECommonStruct& next_event_, bool& isTrade) {
    if (next_event_.msg_ == OSE_MDS::OSE_TRADE) {
      isTrade = true;
      return next_event_.data_.ose_trds_.trd_qty_;
    }
    return 0;
  }

  uint32_t static getTradeVol(const OSE_MDS::OSEPLCommonStruct& next_event_, bool& isTrade) {
    if (next_event_.get_buy_sell_trade() == OSE_MDS::kL1TRADE) {
      isTrade = true;
      return next_event_.size;
    }
    return 0;
  }

  uint32_t static getTradeVol(const OSE_MDS::OSEPriceFeedCommonStruct& next_event_, bool& isTrade) {
    if (next_event_.get_buy_sell_trade() == OSE_MDS::kL1TRADE) {
      isTrade = true;
      return next_event_.size;
    }
    return 0;
  }

  uint32_t static getTradeVol(const RTS_MDS::RTSCommonStruct& next_event_, bool& isTrade) {
    if (next_event_.msg_ == RTS_MDS::RTS_TRADE) {
      isTrade = true;
      return next_event_.data_.rts_trds_.trd_qty_;
    }
    return 0;
  }

  uint32_t static getTradeVol(const MICEX_MDS::MICEXCommonStruct& next_event_, bool& isTrade) {
    if (next_event_.msg_ == MICEX_MDS::MICEX_TRADE) {
      isTrade = true;
      return next_event_.data_.micex_trds_.trd_qty_;
    }
    return 0;
  }

  uint32_t static getTradeVol(const CSM_MDS::CSMCommonStruct& next_event_, bool& isTrade) {
    if (next_event_.msg_ == CSM_MDS::CSM_TRADE) {
      isTrade = true;
      return next_event_.data_.csm_trds_.trd_qty_;
    }
    return 0;
  }

  uint32_t static getTradeVol(const HKOMD_MDS::HKOMDPFCommonStruct& next_event_, bool& isTrade) {
    if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_TRADE) {
      isTrade = true;
      return next_event_.data_.trade_.quantity_;
    }
    return 0;
  }

  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_, HFSAT::BulkFileWriter& writer_, const int& st,
                             const int& et) {
    T next_event_;
    int lastsec = 0;
    int last_vol = 0;

    if (bulk_file_reader_.is_open() && writer_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
        if (available_len_ < sizeof(next_event_)) {
          break;
        }
        int time_since_midnight = next_event_.time_.tv_sec % 86400;
        if (et < time_since_midnight || time_since_midnight < st)  // time filter
        {
          continue;
        }

        bool isTrade = false;
        int trd_vol = getTradeVol(next_event_, isTrade);
        if (!isTrade) continue;

        if (lastsec != time_since_midnight) {
          if (lastsec > 0) {
            writer_ << lastsec << " " << trd_vol << "\n";
            writer_.CheckToFlushBuffer();
          }
          last_vol = trd_vol;
          lastsec = time_since_midnight;
        } else {
          last_vol += trd_vol;  // aggregate for second
        }
      }
      bulk_file_reader_.close();
      if (lastsec > 0) writer_ << lastsec << " " << last_vol << "\n";

      writer_.CheckToFlushBuffer();
      writer_.Close();
    }
  }
};

// all specific template definitions end

void ParseCommandLineParams(const int argc, const char** argv, std::string& shortcode_, int& input_date_,
                            int& begin_secs_from_midnight_, int& end_secs_from_midnight_, std::string& out_file_name,
                            std::string& input_file_name_) {
  // expect :
  // 1. $0 shortcode date_YYYYMMDD out_file_name [ start_tm HHMM ] [ end_tm HHMM ]
  if (argc < 4) {
    std::cerr
        << "Usage: " << argv[0]
        << " shortcode input_date_YYYYMMDD out_file_name [ start_tm_utc_hhmm ] [ end_tm_utc_hhmm ] [ input_file_name ]"
        << std::endl;
    exit(0);
  } else {
    shortcode_ = argv[1];
    input_date_ = atoi(argv[2]);
    out_file_name = argv[3];

    if (argc > 4) {
      begin_secs_from_midnight_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
    }
    if (argc > 5) {
      end_secs_from_midnight_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[5]) % 100) * 60;
    }
    if (argc > 6) {
      input_file_name_ = std::string(argv[6]);
    }
  }
}

/// input arguments : input_date
int main(int argc, char** argv) {
  std::string shortcode_ = "";
  int input_date_ = 20110101;
  int begin_secs_from_midnight_ = 0;
  int end_secs_from_midnight_ = 24 * 60 * 60;
  std::string out_file_name = "";
  std::string input_file_name_ = "";
  ParseCommandLineParams(argc, (const char**)argv, shortcode_, input_date_, begin_secs_from_midnight_,
                         end_secs_from_midnight_, out_file_name, input_file_name_);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(input_date_);
  const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

  HFSAT::TradingLocation_t trading_location_file_read_;
  HFSAT::BulkFileReader reader;
  HFSAT::BulkFileWriter writer;
  writer.Open(out_file_name);

  HFSAT::ExchSource_t exch = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, input_date_);
  trading_location_file_read_ =
      HFSAT::TradingLocationUtils::GetTradingLocationExch(exch);  // initialize to primary location
  if ((exch == HFSAT::kExchSourceHONGKONG && trading_location_file_read_ == HFSAT::kTLocHK &&
       (input_date_ >= 20141204)) ||
      ((exch == HFSAT::kExchSourceHONGKONG) && (trading_location_file_read_ == HFSAT::kTLocJPY) &&
       (input_date_ >= 20150121))) {
    exch = HFSAT::kExchSourceHKOMDCPF;
  }

  if (input_file_name_ == "") {
    if (exch == HFSAT::kExchSourceEUREX) {
      if (HFSAT::UseEOBIData(trading_location_file_read_, input_date_, shortcode_)) {
        input_file_name_ = HFSAT::EOBIPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                               trading_location_file_read_);
      } else {
        input_file_name_ =
            HFSAT::EUREXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
      }
    } else if (exch == HFSAT::kExchSourceCME) {
      input_file_name_ = HFSAT::CommonLoggedMessageFileNamer::GetName(exch, t_exchange_symbol_, input_date_,
                                                                      trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceBMF) {
      input_file_name_ =
          HFSAT::NTPLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceBMFEQ) {
      input_file_name_ = HFSAT::NTPLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                   trading_location_file_read_, false, true);
    } else if (exch == HFSAT::kExchSourceTMX) {
      input_file_name_ =
          HFSAT::TMXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceLIFFE) {
      input_file_name_ =
          HFSAT::LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceICE) {
      input_file_name_ =
          HFSAT::ICELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceHONGKONG) {
      input_file_name_ =
          HFSAT::HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceHKOMDCPF || exch == HFSAT::kExchSourceHKOMDPF) {
      input_file_name_ =
          HFSAT::HKOMDCPFLoggedMessagefileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceJPY) {
      input_file_name_ = HFSAT::OSEPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_,
                                                                            trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceRTS) {
      input_file_name_ =
          HFSAT::RTSLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceMICEX) {
      input_file_name_ =
          HFSAT::MICEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    } else if (exch == HFSAT::kExchSourceCFE) {
      input_file_name_ =
          HFSAT::CFELoggedMessageFileNamer::GetName(t_exchange_symbol_, input_date_, trading_location_file_read_);
    }
  }

  reader.open(input_file_name_);

  if (exch == HFSAT::kExchSourceEUREX) {
    MDSLogReader<EUREX_MDS::EUREXCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                               end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceCME) {
    MDSLogReader<CME_MDS::CMECommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                           end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceBMF) {
    MDSLogReader<NTP_MDS::NTPCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                           end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceBMFEQ) {
    MDSLogReader<NTP_MDS::NTPCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                           end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceTMX) {
    MDSLogReader<TMX_MDS::TMXCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                           end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceLIFFE) {
    MDSLogReader<LIFFE_MDS::LIFFECommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                               end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceICE) {
    MDSLogReader<ICE_MDS::ICECommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                           end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceHONGKONG) {
    MDSLogReader<HKEX_MDS::HKEXCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                             end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceHKOMDCPF || exch == HFSAT::kExchSourceHKOMDPF) {
    MDSLogReader<HKOMD_MDS::HKOMDPFCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                                 end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceJPY) {
    MDSLogReader<OSE_MDS::OSEPriceFeedCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                                    end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceRTS) {
    MDSLogReader<RTS_MDS::RTSCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                           end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceMICEX) {
    MDSLogReader<MICEX_MDS::MICEXCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                               end_secs_from_midnight_);
  } else if (exch == HFSAT::kExchSourceCFE) {
    MDSLogReader<CSM_MDS::CSMCommonStruct>::ReadMDSStructs(reader, writer, begin_secs_from_midnight_,
                                                           end_secs_from_midnight_);
  }

  return 0;
}
