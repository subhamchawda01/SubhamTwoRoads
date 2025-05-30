/**
   \file Tools/mkt_trade_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717

 */

#include "baseinfra/Tools/common_smv_source.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "tradeengine/CommonInitializer/common_initializer.hpp"
#include <inttypes.h>

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225


class TestBook : public HFSAT::Thread, public HFSAT::SecurityMarketViewChangeListener, public HFSAT::FastTradeListener {
  HFSAT::SecurityMarketView *this_smv_;
  HFSAT::Watch &watch_;
  std::unique_ptr<HFSAT::BulkFileWriter> file_writer_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  bool binary_log_;
  int mode_;
  int start_unix_time_;
  int end_unix_time_;
  int pause_mfm_;
  int max_levels_;
  bool stuck_for_go_;
  bool day_over_;
  int traded_volume_;
  int timeout_msecs_;
  int last_msecs_recorded_;
  int count_type_;
  int update_so_far_;
  bool duplicate_entry;

 public:
  TestBook(HFSAT::SecurityMarketView *_this_smv_, HFSAT::Watch &_watch_, const int r_max_levels_, const int r_mode_,
           const int start_unix_time_, const int end_unix_time_, const int _timeout_msecs_, const int _count_type_,
           std::string _this_shortcode_, bool _binary_log_)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        binary_log_(_binary_log_),
        mode_(r_mode_),
        start_unix_time_(start_unix_time_),
        end_unix_time_(end_unix_time_),
        pause_mfm_(0),
        max_levels_(r_max_levels_),
        stuck_for_go_(false),
        day_over_(false),
        traded_volume_(0),
        timeout_msecs_(_timeout_msecs_),
        last_msecs_recorded_(-86400000),
        count_type_(_count_type_),
        update_so_far_(0),
        duplicate_entry(false)

  {
    if ( binary_log_ )
      file_writer_ = std::make_unique<HFSAT::BulkFileWriter>(_this_shortcode_.c_str(), 4 * 1024, std::ofstream::binary | std::ofstream::app | std::ofstream::ate);
    // std::cerr << start_unix_time_ << " " << end_unix_time_ << "\n" ;
    // printf ( "\033[3;1H" );	// move to 3rd line //Rahul
    // printf ( "%2s %5s %3s %11s %7s X %7s %11s %3s %5s %2s\n", "BL", "BS", "BO", "BP", "BIP", "AIP", "AP", "AO", "AS",
    // "AL" );
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if ((start_unix_time_ == 0 || ((int)watch_.msecs_from_midnight() > start_unix_time_ &&
                                   (int)watch_.msecs_from_midnight() < end_unix_time_)) &&
        ((count_type_ == 0 && (watch_.msecs_from_midnight() >= last_msecs_recorded_ + timeout_msecs_)) ||
         (count_type_ == 1 && update_so_far_ > timeout_msecs_))) {
      // std::string secname_ = std::string( this_smv_->secname() ); std::replace( secname_.begin(), secname_.end(), '
      // ', '~');
      if (binary_log_) {
        if (!duplicate_entry) {
          HFSAT::GenericL1DataStruct l1update_struct_;
          this_smv_->UpdateL1Struct(l1update_struct_);
          l1update_struct_.time = watch_.tv();
          l1update_struct_.type = HFSAT::GenericL1DataType::L1_DELTA;
          l1update_struct_.side = _market_update_info_.tradetype;
          l1update_struct_.price = -1;
          l1update_struct_.size = -1;
          l1update_struct_.is_intermediate = false;
/*
        if (_market_update_info_.tradetype == HFSAT::kTradeTypeBuy) 
          std::cout << "OnMarketUpdate: " << 'B' << std::endl; 
        else if (_market_update_info_.tradetype == HFSAT::kTradeTypeSell) 
          std::cout << "OnMarketUpdate: " << 'S' << std::endl; 
        else if (_market_update_info_.tradetype == HFSAT::kTradeTypeNoInfo) 
          std::cout << "OnMarketUpdate: " << 'N' << std::endl; 
*/
          file_writer_->Write(&l1update_struct_, sizeof(l1update_struct_));
          file_writer_->CheckToFlushBuffer();
        }
        duplicate_entry = false;
      }
      else {
        std::cout << watch_.tv() << " "
                  << "OnMarketUpdate "
                  //  << secname_ << " "
                  << this_smv_->MarketUpdateInfoToString() << std::endl;
      }
      last_msecs_recorded_ = watch_.msecs_from_midnight();
      update_so_far_ = 0;
    }
    update_so_far_++;
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if ((start_unix_time_ == 0 || ((int)watch_.msecs_from_midnight() > start_unix_time_ &&
                                   (int)watch_.msecs_from_midnight() < end_unix_time_)) &&
        ((count_type_ == 0 && (watch_.msecs_from_midnight() >= last_msecs_recorded_ + timeout_msecs_)) ||
         (count_type_ == 1))) {
      if (binary_log_) {
        HFSAT::GenericL1DataStruct l1update_struct_;
        this_smv_->UpdateL1Struct(l1update_struct_);
        duplicate_entry = true;
        l1update_struct_.time = watch_.tv();
        l1update_struct_.type = HFSAT::GenericL1DataType::L1_TRADE;
        l1update_struct_.side = _trade_print_info_.buysell_;
        l1update_struct_.price = _trade_print_info_.trade_price_;
        l1update_struct_.size = _trade_print_info_.size_traded_;
        l1update_struct_.is_intermediate = _trade_print_info_.is_intermediate_;
/*
        if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeBuy) 
          std::cout << "OnTradePrint: " << 'B' << std::endl; 
        else if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeSell) 
          std::cout << "OnTradePrint: " << 'S' << std::endl; 
        else if (_trade_print_info_.buysell_ == HFSAT::kTradeTypeNoInfo) 
          std::cout << "OnTradePrint: " << 'N' << std::endl; 
*/
        file_writer_->Write(&l1update_struct_, sizeof(l1update_struct_));
        file_writer_->CheckToFlushBuffer();
      }
      else {
        std::cout << watch_.tv() << " "
                  << "OnTradePrint " << _trade_print_info_.ToString() << " " << this_smv_->MarketUpdateInfoToString()
                  << std::endl;
      }
      last_msecs_recorded_ = watch_.msecs_from_midnight();
    }
    // Do not call trade implied OnMarketUpdate () s
    // OnMarketUpdate ( _security_id_, _market_update_info_ );
  }

  inline void OnIndexUpdate(const unsigned int t_security_id_, double const t_spot_price_){
    if (!binary_log_) 
      std::cout << watch_.tv() << ""
  	        << " OnSpotIndexPrice " << t_spot_price_ << std::endl;
  }

  void OnFastTradeUpdate(const unsigned int t_security_id_, HFSAT::TradeType_t t_aggressive_side,
                         double t_last_trade_price_) {
    if ((start_unix_time_ == 0 || ((int)watch_.msecs_from_midnight() > start_unix_time_ &&
                                   (int)watch_.msecs_from_midnight() < end_unix_time_)) &&
        ((count_type_ == 0 && (watch_.msecs_from_midnight() >= last_msecs_recorded_ + timeout_msecs_)) ||
         (count_type_ == 1))) {
      char t_side = ((t_aggressive_side == HFSAT::kTradeTypeBuy) ? 'B' : 'S');
      std::cout << watch_.tv() << " "
                << "OnFastTradeUpdate [ " << t_side << " @ " << t_last_trade_price_ << " ] "
                << this_smv_->MarketUpdateInfoToString() << std::endl;
      last_msecs_recorded_ = watch_.msecs_from_midnight();
    }
  }

  void thread_main() {
    while (!day_over_) {
      sleep(1);
    }
  }

  void DayOver() { day_over_ = true; }

  void CheckBook() {
    if (this_smv_->IsError()) {
      // 	printf ( "ERROR \n" );
      // 	exit ( 0 );
    }
  }

  void ShowBook() {
    // printf ( "\033[1;1H" );	// move to 1st line
    // printf ( "Time: %s | %s\n", watch_.time_string( ) , watch_.tv ( ) . ToString ( ) . c_str ( ) );
    // printf ( "Secname: %s   Traded Volue: %d\n", this_smv_->secname(), traded_volume_ );
    // printf ( "\033[4;1H" );	// move to 4th line
    int m_m_levels = std::min(max_levels_, std::min(this_smv_->NumBidLevels(), this_smv_->NumAskLevels()));
    for (int t_level_ = 0; t_level_ < m_m_levels; t_level_++) {
      switch (this_smv_->secname()[0]) {
        case 'Z':
        case 'U': {
          printf("%2d %5d %3d %11.7f %7d X %7d %11.7f %3d %5d %2d\n", this_smv_->bid_int_price_level(t_level_),
                 this_smv_->bid_size(t_level_), this_smv_->bid_order(t_level_), this_smv_->bid_price(t_level_),
                 this_smv_->bid_int_price(t_level_), this_smv_->ask_int_price(t_level_), this_smv_->ask_price(t_level_),
                 this_smv_->ask_order(t_level_), this_smv_->ask_size(t_level_),
                 this_smv_->ask_int_price_level(t_level_));
        } break;
        default: {
          printf("%2d %5d %3d %11.7f %7d X %7d %11.7f %3d %5d %2d\n", this_smv_->bid_int_price_level(t_level_),
                 this_smv_->bid_size(t_level_), this_smv_->bid_order(t_level_), this_smv_->bid_price(t_level_),
                 this_smv_->bid_int_price(t_level_), this_smv_->ask_int_price(t_level_), this_smv_->ask_price(t_level_),
                 this_smv_->ask_order(t_level_), this_smv_->ask_size(t_level_),
                 this_smv_->ask_int_price_level(t_level_));
        } break;
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < this_smv_->NumBidLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, this_smv_->NumBidLevels()); t_level_++) {
        switch (this_smv_->secname()[0]) {
          case 'Z':
          case 'U': {
            printf("%2d %5d %3d %11.7f %7d X %7s %11s %3s %5s %2s \n", this_smv_->bid_int_price_level(t_level_),
                   this_smv_->bid_size(t_level_), this_smv_->bid_order(t_level_), this_smv_->bid_price(t_level_),
                   this_smv_->bid_int_price(t_level_), "-", "-", "-", "-", "-");
          } break;
          default: {
            printf("%2d %5d %3d %11.7f %7d X %7s %11s %3s %5s %2s \n", this_smv_->bid_int_price_level(t_level_),
                   this_smv_->bid_size(t_level_), this_smv_->bid_order(t_level_), this_smv_->bid_price(t_level_),
                   this_smv_->bid_int_price(t_level_), "-", "-", "-", "-", "-");
          } break;
        }
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < this_smv_->NumAskLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, this_smv_->NumAskLevels()); t_level_++) {
        switch (this_smv_->secname()[0]) {
          case 'Z':
          case 'U': {
            printf("%2s %5s %3s %11s %7s X %7d %11.7f %3d %5d %2d \n", "-", "-", "-", "-", "-",
                   this_smv_->ask_int_price(t_level_), this_smv_->ask_price(t_level_), this_smv_->ask_order(t_level_),
                   this_smv_->ask_size(t_level_), this_smv_->ask_int_price_level(t_level_));
          } break;
          default: {
            printf("%2s %5s %3s %11s %7s X %7d %11.7f %3d %5d %2d \n", "-", "-", "-", "-", "-",
                   this_smv_->ask_int_price(t_level_), this_smv_->ask_price(t_level_), this_smv_->ask_order(t_level_),
                   this_smv_->ask_size(t_level_), this_smv_->ask_int_price_level(t_level_));
          } break;
        }
      }
    }
  }

  void PrintCmdStr() {
    // printf ( "\033[%d;1H", (max_levels_+4) );	// move to max_levels_ + 4th line
    // if ( mode_ != 2 )
    //   printf ( "Enter command: " );
    // else
    //   printf ( "Enter time to go to: " );
  }

  void AskForUserInput() {
    if (mode_ == 0) {
      sleep(1);
      return;
    }

    PrintCmdStr();

    const int kCmdLineBufferLen = 256;
    char readline_buffer_[kCmdLineBufferLen];
    std::cin.getline(readline_buffer_, kCmdLineBufferLen);
    // TODO process cmd

    HFSAT::PerishableStringTokenizer st_(readline_buffer_, kCmdLineBufferLen);
    const std::vector<const char *> &tokens_ = st_.GetTokens();
    if (mode_ == 1) {
      stuck_for_go_ = false;
    } else {
      if (mode_ == 2) {
        if (tokens_.size() > 0) {
          pause_mfm_ = atoi(tokens_[0]);
          stuck_for_go_ = false;
        }
      }
    }
  }
};

int main(int argc, char **argv) {
  // Assume that we get the filename of a file that only has all logged EBS data pertaining to one symbol.
  // Load up the File Source

  if (argc < 3) {
    std::cerr << "expecting :\n"
              << " 1. $EXEC SIM SHORTCODE TRADINGDATE NTP/NTP_ORD/BMF_EQ/QUINCY/HKOMD/HKOMD_PF/HKOMD_CPF/CHIXORD/BINARY/NSE_L1/BSE_L1"
              << '\n' << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels NTP/NTP_ORD/BMF_EQ/QUINCY" << '\n'
              << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels defmode=0 NTP/NTP_ORD/BMF_EQ/QUINCY" << '\n'
              << "    $EXEC SIM SHORTCODE TRADINGDATE PL" << '\n' << " 2. $EXEC LIVE SHORTCODE NTP/NTP_ORD" << '\n'
              << "    $EXEC LIVE SHORTCODE max_levels NTP/NTP_ORD/BMF_EQ" << '\n'
              << "    $EXEC LIVE SHORTCODE max_levels NETWORKINFOFILENAME=\"SAME_NETWORK_INFO\" NTP/NTP_ORD/BMF_EQ"
              << '\n';
    exit(0);
  }

  HFSAT::TradingLocation_t dep_trading_location_ = HFSAT::kTLocNSE;
  bool isNSEL1 = (argc >= 4 && strcmp(argv[argc - 1], "NSE_L1") == 0);
  bool isBSEL1 = (argc >= 4 && strcmp(argv[argc - 1], "BSE_L1") == 0);
  bool isPL = (argc >= 4 && strcmp(argv[argc - 1], "PL") == 0);
  bool isBSE = (argc >= 4 && strncmp(argv[2], "BSE_", 4) == 0);
  if(isBSE || isBSEL1) {
    dep_trading_location_ = HFSAT::kTLocBSE;
  }

  bool livetrading_ = (strcmp(argv[1], "LIVE") == 0);
  std::string _this_shortcode_ = argv[2];
  bool binary_log = false;

  int tradingdate_ = 0;
  int max_levels_ = 1;  // 15;
  int defmode_ = 0;

  // bool non_self_book_enabled_ = true;

  int start_utc_hhmm_ = 330;
  int end_utc_hhmm_ = 1000;
  int timeout_msecs_ = 0;
  int count_type_ = 0;
  HFSAT::DebugLogger dbglogger_(1024000, 1);

  if (livetrading_) {
    if (argc < 3) {
      std::cerr << "expecting :\n"
                << "    $EXEC LIVE SHORTCODE NTP/NTP_ORD/BMF_EQ" << '\n'
                << "    $EXEC LIVE SHORTCODE max_levels NTP/NTP_ORD/BMF_EQ" << '\n'
                << "    $EXEC LIVE SHORTCODE max_levels defmode=0 NTP/NTP_ORD/BMF_EQ" << '\n'
                << "    $EXEC LIVE SHORTCODE max_levels defmode=0 NETWORKINFOFILENAME=\"SAME_NETWORK_INFO\" "
                   "NTP/NTP_ORD/BMF_EQ" << '\n';
      exit(0);
    }

    tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();

    if (argc >= 4) {
      if (!isPL) max_levels_ = std::min(15, atoi(argv[3]));
    }
    if (argc >= 5) {
      defmode_ = atoi(argv[4]);
    }
  } else {
    if (argc < 4) {
      std::cerr
          << "expecting :\n"
          << " 1. $EXEC SIM SHORTCODE TRADINGDATE NTP/NTP_ORD/BMF_EQ/BINARY/NSE_L1/BSE_L1 " << '\n'
          << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels NTP/NTP_ORD/BMF_EQ " << '\n'
          << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels defmode=0 NTP/NTP_ORD/BMF_EQ " << '\n'
          << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels defmode=0 NTP/NTP_ORD/BMF_EQ START_TIME END_TIME TIMEOUT "
          << '\n' << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels defmode=0 NTP/NTP_ORD/BMF_EQ START_TIME END_TIME "
          << '\n';
      exit(0);
    }
    tradingdate_ = atoi(argv[3]);
    if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
      std::cerr << tradingdate_ << " not logical date" << std::endl;
      exit(0);
    }
    if (argc >= 5) {  // change this OF,PF
      if (!isPL) max_levels_ = std::min(15, atoi(argv[4]));
      if (!strcmp(argv[4],"BINARY")) binary_log = true;
    }
    if (argc >= 6) {
      defmode_ = atoi(argv[5]);
    }
    if (argc >= 8) {
      if (strcmp(argv[6], "DEF") != 0) {
        start_utc_hhmm_ =
            HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[6] + 4), argv[6]);
      }
      if (strcmp(argv[6], "DEF") != 0) {
        end_utc_hhmm_ =
            HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[7] + 4), argv[7]);
      }
    }
    if (argc >= 9) {
      if (strncmp(argv[8], "EVT_", 4) == 0) {
        timeout_msecs_ = atoi(argv[8] + 4);
        count_type_ = 1;
      } else {
        timeout_msecs_ = atoi(argv[8]);
        count_type_ = 0;
      }
    }
    // std::cerr << "st: " << start_unix_time_ << " et: " << end_unix_time_ << " timout: " << timeout_msecs_
    //        << " dfmod: " << defmode_ << std::endl;
  }

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);
  if(isBSE || isBSEL1) {
    HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadBSESecurityDefinitions();
  } else {
    HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadNSESecurityDefinitions();
  }
  std::vector<std::string> shortcode_list_;
  shortcode_list_.push_back(_this_shortcode_);
  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonInitializer* common_intializer =
      new CommonInitializer(shortcode_list_, shortcode_list_, tradingdate_, dbglogger_, dep_trading_location_, livetrading_);
  common_intializer->SetStartEndTime(start_utc_hhmm_, end_utc_hhmm_);
  common_intializer->SetRuntimeID(999999);
  if (isNSEL1 || isBSEL1)
    common_intializer->Initialize(true);
  else
    common_intializer->Initialize();


  // Get the smv and watch after creating the source
  std::vector<HFSAT::SecurityMarketView*>& p_smv_vec_ = common_intializer->getSMVMap();
  HFSAT::Watch &watch_ = common_intializer->getWatch();
  HFSAT::SecurityMarketView* p_smv_ = p_smv_vec_[0];

  int trading_start_utc_mfm_ = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, start_utc_hhmm_, "UTC_");
  int trading_end_utc_mfm_ = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, end_utc_hhmm_, "UTC_");

  std::string exch_symbol;
  std::string datasource;

  if(isBSE || isBSEL1) {
    exch_symbol = HFSAT::BSESecurityDefinitions::GetExchSymbolBSE(_this_shortcode_);
    datasource = HFSAT::BSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exch_symbol) + "_" + std::to_string(tradingdate_);
  } else {
    exch_symbol = HFSAT::NSESecurityDefinitions::GetExchSymbolNSE(_this_shortcode_);
    datasource = HFSAT::NSESecurityDefinitions::ConvertExchSymboltoDataSourceName(exch_symbol) + "_" + std::to_string(tradingdate_);
  }

  TestBook test_book_(p_smv_, watch_, max_levels_, defmode_, trading_start_utc_mfm_, trading_end_utc_mfm_, timeout_msecs_,
                      count_type_, datasource, binary_log);
  test_book_.run();

  // Subscribe yourself to the smv
  p_smv_->subscribe_price_type(&test_book_, HFSAT::kPriceTypeMktSizeWPrice);
  p_smv_->subscribe_price_type(&test_book_, HFSAT::kPriceTypeSpotIndex);
  if (max_levels_ > 1) {
    p_smv_->subscribe_L2(&test_book_);
  }

  p_smv_->subscribe_to_fast_trade_updates(&test_book_);

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_intializer->Run();

  test_book_.DayOver();

  // --------------------------------- API use over------------------------------------------------------

  if ((defmode_ == 2) || (defmode_ == 1)) {
    // printf ( "Day Over. Enter to exit " );
    fflush(stdout);
  }

  test_book_.stop();
  return 0;
}
