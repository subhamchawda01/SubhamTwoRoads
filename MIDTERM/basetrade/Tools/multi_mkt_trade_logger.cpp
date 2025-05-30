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
#include "dvccode/CDef/ttime.hpp"
#include <inttypes.h>

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

class TestBook : public HFSAT::Thread, public HFSAT::SecurityMarketViewChangeListener, public HFSAT::FastTradeListener {
  HFSAT::SecurityMarketViewPtrVec &this_smv_;
  HFSAT::Watch &watch_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
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
 public:
  TestBook(HFSAT::SecurityMarketViewPtrVec &_this_smv_, HFSAT::Watch &_watch_, const int r_max_levels_, const int r_mode_,
           const int start_unix_time_, const int end_unix_time_, const int _timeout_msecs_, const int _count_type_)
      : this_smv_(_this_smv_),
        watch_(_watch_),
        mode_(r_mode_),
        start_unix_time_(start_unix_time_ * 1000),
        end_unix_time_(end_unix_time_ * 1000),
        pause_mfm_(0),
        max_levels_(r_max_levels_),
        stuck_for_go_(false),
        day_over_(false),
        traded_volume_(0),
        timeout_msecs_(_timeout_msecs_),
        last_msecs_recorded_(-86400000),
        count_type_(_count_type_),
        update_so_far_(0)
  {
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
      std::cout << watch_.tv() << " "
                  << "OnMarketUpdate " << _security_id_ << " ";
       int i=0;
       for( auto & itr : this_smv_ ){
   	HFSAT::SecurityMarketView *p_smv_ = itr;
        std::cout << ((p_smv_->market_update_info_.security_id_ == _security_id_) ? (i==0?"*":"^") : "");
        std::cout << p_smv_->MarketUpdateInfoToString()<< " ";
	i++;
	i%=2;
       }
       std::cout<<std::endl;
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
	int i=0;
        std::cout << watch_.tv() << " "
                  << "OnTradePrint " << _security_id_ << " " << _trade_print_info_.ToString() << " ";
  	for( auto & itr : this_smv_ ){
          HFSAT::SecurityMarketView *p_smv_ = itr;
          std::cout << ((p_smv_->market_update_info_.security_id_ == _security_id_) ? (i==0?"*":"^") : "");
 	  std::cout << p_smv_->MarketUpdateInfoToString() << " ";
	  i++;
          i%=2;
        }
        std::cout << std::endl;
        last_msecs_recorded_ = watch_.msecs_from_midnight();
    }
    // Do not call trade implied OnMarketUpdate () s
    // OnMarketUpdate ( _security_id_, _market_update_info_ );
  }

  void OnFastTradeUpdate(const unsigned int t_security_id_, HFSAT::TradeType_t t_aggressive_side,
                         double t_last_trade_price_) {
    if ((start_unix_time_ == 0 || ((int)watch_.msecs_from_midnight() > start_unix_time_ &&
                                   (int)watch_.msecs_from_midnight() < end_unix_time_)) &&
        ((count_type_ == 0 && (watch_.msecs_from_midnight() >= last_msecs_recorded_ + timeout_msecs_)) ||
         (count_type_ == 1))) {
      char t_side = ((t_aggressive_side == HFSAT::kTradeTypeBuy) ? 'B' : 'S');
      std::cout << watch_.tv() << " "
                << "OnFastTradeUpdate [ " << t_side << " @ " << t_last_trade_price_ << " ] ";
      for( auto & itr : this_smv_ ){
          HFSAT::SecurityMarketView *p_smv_ = itr;
          std::cout<<" "<< p_smv_->MarketUpdateInfoToString();
      }
      std::cout << std::endl;
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
   // if (this_smv_->IsError()) {
      // 	printf ( "ERROR \n" );
      // 	exit ( 0 );
   // }
  }

  void ShowBook() {
   for( auto & itr : this_smv_ ){
          HFSAT::SecurityMarketView *p_smv_ = itr;

    // printf ( "\033[1;1H" );	// move to 1st line
    // printf ( "Time: %s | %s\n", watch_.time_string( ) , watch_.tv ( ) . ToString ( ) . c_str ( ) );
    // printf ( "Secname: %s   Traded Volue: %d\n", p_smv_->secname(), traded_volume_ );
    // printf ( "\033[4;1H" );	// move to 4th line
    int m_m_levels = std::min(max_levels_, std::min(p_smv_->NumBidLevels(), p_smv_->NumAskLevels()));
    for (int t_level_ = 0; t_level_ < m_m_levels; t_level_++) {
      switch (p_smv_->secname()[0]) {
        case 'Z':
        case 'U': {
          printf("%2d %5d %3d %11.7f %7d X %7d %11.7f %3d %5d %2d\n", p_smv_->bid_int_price_level(t_level_),
                 p_smv_->bid_size(t_level_), p_smv_->bid_order(t_level_), p_smv_->bid_price(t_level_),
                 p_smv_->bid_int_price(t_level_), p_smv_->ask_int_price(t_level_), p_smv_->ask_price(t_level_),
                 p_smv_->ask_order(t_level_), p_smv_->ask_size(t_level_),
                 p_smv_->ask_int_price_level(t_level_));
        } break;
        default: {
          printf("%2d %5d %3d %11.7f %7d X %7d %11.7f %3d %5d %2d\n", p_smv_->bid_int_price_level(t_level_),
                 p_smv_->bid_size(t_level_), p_smv_->bid_order(t_level_), p_smv_->bid_price(t_level_),
                 p_smv_->bid_int_price(t_level_), p_smv_->ask_int_price(t_level_), p_smv_->ask_price(t_level_),
                 p_smv_->ask_order(t_level_), p_smv_->ask_size(t_level_),
                 p_smv_->ask_int_price_level(t_level_));
        } break;
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < p_smv_->NumBidLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, p_smv_->NumBidLevels()); t_level_++) {
        switch (p_smv_->secname()[0]) {
          case 'Z':
          case 'U': {
            printf("%2d %5d %3d %11.7f %7d X %7s %11s %3s %5s %2s \n", p_smv_->bid_int_price_level(t_level_),
                   p_smv_->bid_size(t_level_), p_smv_->bid_order(t_level_), p_smv_->bid_price(t_level_),
                   p_smv_->bid_int_price(t_level_), "-", "-", "-", "-", "-");
          } break;
          default: {
            printf("%2d %5d %3d %11.7f %7d X %7s %11s %3s %5s %2s \n", p_smv_->bid_int_price_level(t_level_),
                   p_smv_->bid_size(t_level_), p_smv_->bid_order(t_level_), p_smv_->bid_price(t_level_),
                   p_smv_->bid_int_price(t_level_), "-", "-", "-", "-", "-");
          } break;
        }
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < p_smv_->NumAskLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, p_smv_->NumAskLevels()); t_level_++) {
        switch (p_smv_->secname()[0]) {
          case 'Z':
          case 'U': {
            printf("%2s %5s %3s %11s %7s X %7d %11.7f %3d %5d %2d \n", "-", "-", "-", "-", "-",
                   p_smv_->ask_int_price(t_level_), p_smv_->ask_price(t_level_), p_smv_->ask_order(t_level_),
                   p_smv_->ask_size(t_level_), p_smv_->ask_int_price_level(t_level_));
          } break;
          default: {
            printf("%2s %5s %3s %11s %7s X %7d %11.7f %3d %5d %2d \n", "-", "-", "-", "-", "-",
                   p_smv_->ask_int_price(t_level_), p_smv_->ask_price(t_level_), p_smv_->ask_order(t_level_),
                   p_smv_->ask_size(t_level_), p_smv_->ask_int_price_level(t_level_));
          } break;
        }
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
              << " 1. $EXEC SIM SHORTCODE TRADINGDATE NTP/NTP_ORD/BMF_EQ/QUINCY/HKOMD/HKOMD_PF/HKOMD_CPF/CHIXORD"
              << '\n' << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels NTP/NTP_ORD/BMF_EQ/QUINCY" << '\n'
              << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels defmode=0 NTP/NTP_ORD/BMF_EQ/QUINCY" << '\n'
              << "    $EXEC SIM SHORTCODE TRADINGDATE PL" << '\n' << " 2. $EXEC LIVE SHORTCODE NTP/NTP_ORD" << '\n'
              << "    $EXEC LIVE SHORTCODE max_levels NTP/NTP_ORD/BMF_EQ" << '\n'
              << "    $EXEC LIVE SHORTCODE max_levels NETWORKINFOFILENAME=\"SAME_NETWORK_INFO\" NTP/NTP_ORD/BMF_EQ"
              << '\n';
    exit(0);
  }

  bool isNTP = (argc >= 4 && strcmp(argv[argc - 1], "NTP") == 0);
  bool isNTPORD = (argc >= 4 && strcmp(argv[argc - 1], "NTP_ORD") == 0);
  bool isBMFEq = (argc >= 4 && strcmp(argv[argc - 1], "BMF_EQ") == 0);
  bool isPL = (argc >= 4 && strcmp(argv[argc - 1], "PL") == 0);
  bool isQuincy = (argc >= 4 && strcmp(argv[argc - 1], "QUINCY") == 0);
  bool isOMD = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD") == 0);
  bool isOMD_PF = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD_PF") == 0);
  bool isOMD_CPF = (argc >= 4 && strcmp(argv[argc - 1], "HKOMD_CPF") == 0);
  bool isCHIXORD = (argc >= 4 && strcmp(argv[argc - 1], "CHIXORD") == 0);
  bool isCMEOBF = (argc >= 4 && strcmp(argv[argc - 1], "CMEOBF") == 0);

  // book building modification purpose
  bool is_TMX_OF = (argc >= 4 && strcmp(argv[argc - 1], "OF") == 0);

  bool set_book_type_ = false;

  if (isNTP || isNTPORD || isBMFEq || isQuincy || isOMD || isOMD_PF || isOMD_CPF || isCHIXORD || isCMEOBF ||
      is_TMX_OF) {
    --argc;
    set_book_type_ = true;
  }

  bool livetrading_ = (strcmp(argv[1], "LIVE") == 0);
  std::string _this_shortcode_ = argv[2];
  int tradingdate_ = 0;
  int max_levels_ = 1;  // 15;
  int defmode_ = 0;

  // bool non_self_book_enabled_ = true;

  int start_unix_time_ = 0;
  int end_unix_time_ = 0;
  int timeout_msecs_ = 0;
  int count_type_ = 0;
  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");

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
    if (argc >= 6) {
      if (strcmp(argv[5], "SAME_NETWORK_INFO") != 0) {
        network_account_info_filename_ = argv[5];
      }
    }
  } else {
    if (argc < 4) {
      std::cerr
          << "expecting :\n"
          << " 1. $EXEC SIM SHORTCODE TRADINGDATE NTP/NTP_ORD/BMF_EQ " << '\n'
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
    }
    if (argc >= 6) {
      defmode_ = atoi(argv[5]);
    }
    if (argc >= 8) {
      if (strcmp(argv[6], "DEF") != 0) {
        start_unix_time_ =
            HFSAT::GetSecondsFromHHMM(HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[6] + 4), argv[6]));
      }
      if (strcmp(argv[6], "DEF") != 0) {
        end_unix_time_ =
            HFSAT::GetSecondsFromHHMM(HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[7] + 4), argv[7]));
      }
    }
    if (start_unix_time_ > end_unix_time_) {
      start_unix_time_ -= 86400;
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

  std::vector<std::string> shortcode_list_;
//  shortcode_list_.push_back(_this_shortcode_);i
//HFSAT::PerishableStringTokenizer st_(argv[2], );
HFSAT::PerishableStringTokenizer::StringSplit(argv[2], ',', shortcode_list_);

//std::cout<<"List of shortcodes::\n";
//for(auto it=shortcode_list_.begin(); it != shortcode_list_.end(); it++){
//  std::cout<<*it<<"\n";
//}


  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonSMVSource *common_smv_source = new CommonSMVSource(shortcode_list_, tradingdate_, livetrading_);

  // Set the required book type and network info filename
  if (set_book_type_) {
    common_smv_source->SetBookType(isNTP, isNTPORD, isBMFEq, isPL, isQuincy, true, isOMD_PF, isOMD_CPF, false,
                                   isCHIXORD, isCMEOBF);
  }
  common_smv_source->SetNetworkAccountInfoFilename(network_account_info_filename_);

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  HFSAT::Watch &watch_ = common_smv_source->getWatch();

// Get the smv and watch after creating the source   
 HFSAT::SecurityMarketViewPtrVec & smv_map_ = common_smv_source->getSMVMap();
 TestBook * new_book = new TestBook(smv_map_, watch_, max_levels_, defmode_, start_unix_time_, end_unix_time_, timeout_msecs_, count_type_);
 new_book->run();
 for( auto & itr : smv_map_ ){      
   HFSAT::SecurityMarketView *p_smv_ = itr;
   //TestBook * new_book = new TestBook(p_smv_, watch_, max_levels_, defmode_, start_unix_time_, end_unix_time_, timeout_msecs_, count_type_);
   //new_book->run();
   p_smv_->subscribe_price_type(new_book, HFSAT::kPriceTypeMktSizeWPrice);
   if (max_levels_ > 1) {
    p_smv_->subscribe_L2(new_book);
   }
   p_smv_->subscribe_to_fast_trade_updates(new_book); 
 }   
 common_smv_source->Run();  

  // --------------------------------- API use over------------------------------------------------------

  if ((defmode_ == 2) || (defmode_ == 1)) {
    // printf ( "Day Over. Enter to exit " );
    fflush(stdout);
  }

//  test_book_.stop();
  return 0;
}
