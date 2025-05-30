/**
   \file CommonDataStructures/simple_security_symbol_indexer.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   India
   +91 80 4190 3551
*/
#include "baseinfra/Tools/common_smv_source.hpp"

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

class TestBook : public HFSAT::SecurityMarketViewChangeListener, public HFSAT::SpreadMarketViewListener {
  std::vector<HFSAT::SecurityMarketView *> &this_smv_vec_;
  HFSAT::Watch &watch_;

  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  int mode_;
  int pause_mfm_;
  int max_levels_;
  bool stuck_for_go_;
  bool day_over_;
  unsigned long long traded_volume_;
  double last_traded_px_;

  int start_unix_time_;
  int end_unix_time_;

  std::vector<int> this_size_vec_;

 public:
  TestBook(std::vector<HFSAT::SecurityMarketView *> &_this_smv_vec_, HFSAT::Watch &_watch_, const int r_max_levels_,
           const int r_mode_, std::vector<int> _this_size_vec_, const int t_start_unix_time_ = 0,
           const int t_end_unix_time_ = 0)
      : this_smv_vec_(_this_smv_vec_),
        watch_(_watch_),
        mode_(r_mode_),
        pause_mfm_(0),
        max_levels_(r_max_levels_),
        stuck_for_go_(false),
        day_over_(false),
        traded_volume_(0),
        last_traded_px_(0.0),
        start_unix_time_(t_start_unix_time_),
        end_unix_time_(t_end_unix_time_),
        this_size_vec_(_this_size_vec_) {
    printf("\033[3;1H");  // move to 3rd line //Rahul

    printf("%4s %10s %6s %22s %14s X %14s %22s %6s %10s %4s\n", "BL", "BS", "BO", "BP", "BIP", "AIP", "AP", "AO", "AS",
           "AL");
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_) {
    if ((start_unix_time_ > 0 && watch_.tv().tv_sec < start_unix_time_) ||
        (end_unix_time_ > 0 && watch_.tv().tv_sec > end_unix_time_)) {
      return;
    }

    DefLoop(_security_id_);
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                           const HFSAT::MarketUpdateInfo &_market_update_info_) {
    traded_volume_ += _trade_print_info_.size_traded_;
    last_traded_px_ = _trade_print_info_.trade_price_;

    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void DefLoop(const unsigned int _security_id_) {
    switch (mode_) {
      case 0: {
        ShowBook(_security_id_);
      } break;
      case 1: {
        ShowBook(_security_id_);
        stuck_for_go_ = true;
        AskForUserInput();
        while (stuck_for_go_) {
          sleep(1);
        }
      } break;
      case 2: {
        if ((pause_mfm_ <= 0) || (watch_.msecs_from_midnight() > pause_mfm_)) {
          /// after reaching specified time it should permit event
          /// by event lookup like mode 1
          if (pause_mfm_ > 0) {
            mode_ = 1;
            printf("\033[%d;1H", (max_levels_ + 4));
            printf("                                                                ");
            return;
          }
          ShowBook(_security_id_);
          stuck_for_go_ = true;
          AskForUserInput();
          while (stuck_for_go_) {
            sleep(1);
          }
        } else {
          ShowBook(_security_id_);
        }
      } break;
      case 3: {
        CheckBook(_security_id_);
      } break;
      default: { } break; }
  }

  void CheckBook(const unsigned int _security_id_) {
    if (this_smv_vec_[_security_id_]->IsError()) {
      // 	printf ( "ERROR \n" );
      // 	exit ( 0 );
    }
  }

  void ShowBook(const unsigned int _security_id_) {
    HFSAT::SecurityMarketView this_smv_ = *this_smv_vec_[_security_id_];
    if ((start_unix_time_ > 0 && watch_.tv().tv_sec < start_unix_time_) ||
        (end_unix_time_ > 0 && watch_.tv().tv_sec > end_unix_time_)) {
      return;
    }

    printf("\033[1;1H");  // move to 1st line
    printf("Time: %s | %s\n", watch_.time_string(), watch_.tv().ToString().c_str());
    printf("Secname: %s   Traded Volume: %llu Last Price: %f\n", this_smv_.secname(), traded_volume_, last_traded_px_);
    printf("\033[4;1H");  // move to 4th line

    if (this_size_vec_.size() == 2) {
      // SPREAD
      if (this_smv_vec_[0]->NumBidLevels() > 0 && this_smv_vec_[0]->NumAskLevels() > 0 &&
          this_smv_vec_[1]->NumBidLevels() > 0 && this_smv_vec_[1]->NumAskLevels() > 0) {
        // just best variables
        printf("BEST %10d %6d %22.9f %14d X %14d %22.9f %6d %10d \n",
               std::min((this_smv_vec_[0]->bestbid_size()) / this_size_vec_[0],
                        (this_smv_vec_[1]->bestask_size()) / this_size_vec_[1]),
               std::min(this_smv_vec_[0]->bestbid_ordercount(), this_smv_vec_[1]->bestask_ordercount()),
               (this_smv_vec_[0]->bestbid_price() * this_size_vec_[0] -
                this_smv_vec_[1]->bestask_price() * this_size_vec_[1]),
               (this_smv_vec_[0]->bestbid_int_price() * this_size_vec_[0] -
                this_smv_vec_[1]->bestask_int_price() * this_size_vec_[1]),
               (this_smv_vec_[0]->bestask_int_price() * this_size_vec_[0] -
                this_smv_vec_[1]->bestbid_int_price() * this_size_vec_[1]),
               (this_smv_vec_[0]->bestask_price() * this_size_vec_[0] -
                this_smv_vec_[1]->bestbid_price() * this_size_vec_[1]),
               std::min(this_smv_vec_[0]->bestask_ordercount(), this_smv_vec_[1]->bestbid_ordercount()),
               std::min(this_smv_vec_[0]->bestask_size() / this_size_vec_[0],
                        this_smv_vec_[1]->bestbid_size() / this_size_vec_[1]));
      }
      return;
    }

    if (this_size_vec_.size() == 3) {
      // FLY
      if (this_smv_vec_[0]->NumBidLevels() > 0 && this_smv_vec_[0]->NumAskLevels() > 0 &&
          this_smv_vec_[1]->NumBidLevels() > 0 && this_smv_vec_[1]->NumAskLevels() > 0 &&
          this_smv_vec_[2]->NumBidLevels() > 0 && this_smv_vec_[2]->NumAskLevels() > 0) {
        // just best variables
        printf("BEST %10d %6d %22.9f %14d X %14d %22.9f %6d %10d \n",
               std::min(std::min(this_smv_vec_[0]->bestbid_size() / this_size_vec_[0],
                                 this_smv_vec_[1]->bestask_size() / this_size_vec_[0]),
                        this_smv_vec_[1]->bestbid_size() / this_size_vec_[2]),
               std::min(std::min(this_smv_vec_[0]->bestbid_ordercount(), this_smv_vec_[1]->bestask_ordercount() / 2),
                        this_smv_vec_[2]->bestbid_ordercount()),
               (this_smv_vec_[0]->bestbid_price() * this_size_vec_[0] -
                this_size_vec_[1] * this_smv_vec_[1]->bestask_price() +
                this_smv_vec_[2]->bestbid_price() * this_size_vec_[2]),
               (this_smv_vec_[0]->bestbid_int_price() * this_size_vec_[0] -
                this_size_vec_[1] * this_smv_vec_[1]->bestask_int_price() +
                this_smv_vec_[2]->bestbid_int_price() * this_size_vec_[2]),
               (this_smv_vec_[0]->bestask_int_price() * this_size_vec_[0] -
                this_size_vec_[1] * this_smv_vec_[1]->bestbid_int_price() +
                this_smv_vec_[2]->bestask_int_price() * this_size_vec_[2]),
               (this_smv_vec_[0]->bestask_price() * this_size_vec_[0] -
                this_size_vec_[1] * this_smv_vec_[1]->bestbid_price() +
                this_smv_vec_[2]->bestask_price() * this_size_vec_[2]),
               std::min(std::min(this_smv_vec_[0]->bestask_ordercount(), this_smv_vec_[1]->bestbid_ordercount() / 2),
                        this_smv_vec_[2]->bestask_ordercount()),
               std::min(std::min(this_smv_vec_[0]->bestask_size() / this_size_vec_[0],
                                 this_smv_vec_[1]->bestbid_size() / this_size_vec_[1]),
                        this_smv_vec_[2]->bestask_size() / this_size_vec_[2]));
      }
      return;
    }

    int m_m_levels = std::min(max_levels_, std::min(this_smv_.NumBidLevels(), this_smv_.NumAskLevels()));

    // just best variables first
    // printf ( "BEST %10d %6d %22.9f %14d X %14d %22.9f %6d %10d \n",
    // 	     this_smv_.bestbid_size ( ), this_smv_.bestbid_ordercount ( ),
    // 	     this_smv_.bestbid_price ( ), this_smv_.bestbid_int_price ( ),
    // 	     this_smv_.bestask_int_price ( ), this_smv_.bestask_price ( ),
    // 	     this_smv_.bestask_ordercount ( ), this_smv_.bestask_size ( ) ) ;

    for (int t_level_ = 0; t_level_ < m_m_levels; t_level_++) {
      switch (this_smv_.secname()[0]) {
        case 'Z':
        case 'U': {
          printf("\033[2K%4d %10d %6d %22.9f %14d X %14d %22.9f %6d %10d %4d\n",
                 std::min(9999, abs(this_smv_.bid_int_price_level(t_level_))), this_smv_.bid_size(t_level_),
                 this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_), this_smv_.bid_int_price(t_level_),
                 this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_), this_smv_.ask_order(t_level_),
                 this_smv_.ask_size(t_level_), std::min(9999, abs(this_smv_.ask_int_price_level(t_level_))));
        } break;
        default: {
          printf("\033[2K%4d %10d %6d %22.9f %14d X %14d %22.9f %6d %10d %4d\n",
                 std::min(9999, abs(this_smv_.bid_int_price_level(t_level_))), this_smv_.bid_size(t_level_),
                 this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_), this_smv_.bid_int_price(t_level_),
                 this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_), this_smv_.ask_order(t_level_),
                 this_smv_.ask_size(t_level_), std::min(9999, abs(this_smv_.ask_int_price_level(t_level_))));
        } break;
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < this_smv_.NumBidLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, this_smv_.NumBidLevels()); t_level_++) {
        switch (this_smv_.secname()[0]) {
          case 'Z':
          case 'U': {
            printf("\033[2K%4d %10d %6d %22.9f %14d X %14s %22s %6s %10s %4s \n",
                   std::min(9999, abs(this_smv_.bid_int_price_level(t_level_))), this_smv_.bid_size(t_level_),
                   this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_), this_smv_.bid_int_price(t_level_), "-",
                   "-", "-", "-", "-");
          } break;
          default: {
            printf("\033[2K%4d %10d %6d %22.9f %14d X %14s %22s %6s %10s %4s \n",
                   std::min(9999, abs(this_smv_.bid_int_price_level(t_level_))), this_smv_.bid_size(t_level_),
                   this_smv_.bid_order(t_level_), this_smv_.bid_price(t_level_), this_smv_.bid_int_price(t_level_), "-",
                   "-", "-", "-", "-");
          } break;
        }
      }
    }

    if ((m_m_levels < max_levels_) && (m_m_levels < this_smv_.NumAskLevels())) {
      for (int t_level_ = m_m_levels; t_level_ < std::min(max_levels_, this_smv_.NumAskLevels()); t_level_++) {
        switch (this_smv_.secname()[0]) {
          case 'Z':
          case 'U': {
            printf("\033[2K%4s %10s %6s %22s %14s X %14d %22.9f %6d %10d %4d \n", "-", "-", "-", "-", "-",
                   this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_), this_smv_.ask_order(t_level_),
                   this_smv_.ask_size(t_level_), std::min(9999, abs(this_smv_.ask_int_price_level(t_level_))));
          } break;
          default: {
            printf("\033[2K%4s %10s %6s %22s %14s X %14d %22.9f %6d %10d %4d \n", "-", "-", "-", "-", "-",
                   this_smv_.ask_int_price(t_level_), this_smv_.ask_price(t_level_), this_smv_.ask_order(t_level_),
                   this_smv_.ask_size(t_level_), std::min(9999, abs(this_smv_.ask_int_price_level(t_level_))));
          } break;
        }
      }
    }
  }

  void PrintCmdStr() {
    printf("\033[%d;1H", (max_levels_ + 4));  // move to max_levels_ + 4th line
    if (mode_ != 2)
      printf("Enter command: ");
    else
      printf("Enter time to go to: ");
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

  virtual void OnSpreadMarketViewUpdate(const HFSAT::SpreadMarketView &_spread_market_view_) {
    if ((start_unix_time_ > 0 && watch_.tv().tv_sec < start_unix_time_) ||
        (end_unix_time_ > 0 && watch_.tv().tv_sec > end_unix_time_)) {
      return;
    }

    _spread_market_view_.PrintBook();
  }
};

int main(int argc, char **argv) {
  // Assume that we get the filename of a file that only has all logged EBS data pertaining to one symbol.
  // Load up the File Source

  std::ostringstream usage;
  usage
      << "expecting :\n"
      << " 1. $EXEC SIM SHORTCODE TRADINGDATE NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI/EOBI_PRICE_FEED/RTS_OF" << '\n'
      << "    $EXEC SIM SPREAD SHC1 SIZE1 SHC2 SIZE2 TRADINGDATE NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI/EOBI_PRICE_FEED/RTS_OF"
      << '\n' << "    $EXEC SIM FLY SHC1 SIZE1 SHC2 SIZE2 SHC3 SIZE3 TRADINGDATE "
                 "NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI/EOBI_PRICE_FEED/RTS_OF" << '\n'
      << "    $EXEC SIM STRUCTURE SHORTCODE TRADINGDATE NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI/EOBI_PRICE_FEED/RTS_OF" << '\n'
      << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI/EOBI_PRICE_FEED/RTS_OF" << '\n'
      << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels defmode=0 "
         "NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI/EOBI_PRICE_FEED/RTS_OF" << '\n' << "    $EXEC SIM SHORTCODE TRADINGDATE PL"
      << '\n' << "    $EXEC SIM SHORTCODE TRADINGDATE max_levels start_unixtime=START end_unixtime=END defmode=0 "
                 "NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI/EOBI_PRICE_FEED/RTS_OF " << '\n'
      << " 2. $EXEC SHM/LIVE SHORTCODE NTP/NTP_ORD/EOBI" << '\n'
      << "    $EXEC SHM/LIVE SHORTCODE max_levels NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI" << '\n'
      << "    $EXEC SHM/LIVE SHORTCODE max_levels NETWORKINFOFILENAME=\"SAME_NETWORK_INFO\" "
         "NTP/NTP_ORD/BMF_EQ/QUINCY/EOBI" << '\n';

  if (argc < 3) {
    std::cerr << usage.str();
    exit(0);
  }

  bool isNTP = (argc >= 4 && strcmp(argv[argc - 1], "NTP") == 0);
  bool isNTPORD = (argc >= 4 && strcmp(argv[argc - 1], "NTP_ORD") == 0);
  bool isBMFEq = (argc >= 4 && strcmp(argv[argc - 1], "BMF_EQ") == 0);
  bool isQuincySource = (argc >= 4 && strcmp(argv[argc - 1], "QUINCY") == 0);
  bool isPL = (argc >= 4 && strcmp(argv[argc - 1], "PL") == 0);
  bool isCHIXORD = (argc >= 4 && strcmp(argv[argc - 1], "CHIXORD") == 0);
  //  bool isEobi = (argc >=4 && strcmp(argv[argc-1], "EOBI") == 0 );
  //  bool isEobiPriceFeed = (argc >=4 && strcmp(argv[argc-1], "EOBI_PRICE_FEED") == 0 );
  bool fpga_available = (argc >= 4 && strcmp(argv[argc - 1], "FPGA") == 0);
  bool set_book_type_ = false;
  bool isRTSOF = (argc >= 4 && strcmp(argv[argc - 1], "RTS_OF") == 0);

  if (isNTP || isNTPORD || isBMFEq || isQuincySource || isCHIXORD) {
    set_book_type_ = true;
    --argc;
  }

  bool livetrading_ = (strcmp(argv[1], "LIVE") == 0 || strcmp(argv[1], "SHM") == 0);
  // bool use_shm_source_ = ( strcmp ( argv[1], "SHM" ) == 0 ) ;

  // std::string _this_shortcode_ = argv[2];
  std::vector<std::string> _this_shortcode_vec_;
  std::vector<int> _this_size_vec_;  // for spread & fly

  int tradingdate_ = 0;
  int max_levels_ = 15;
  int defmode_ = 0;
  int start_unix_time_ = 0;
  int end_unix_time_ = 0;

  std::string network_account_info_filename_ = HFSAT::FileUtils::AppendHome(
      std::string(BASESYSINFODIR) + "TradingInfo/NetworkInfo/network_account_info_filename.txt");

  bool is_structure_ = false;
  bool is_spread_ = false;
  bool is_fly_ = false;

  std::string structure_shc_ = "";
  std::string shc_ = "";

  if (livetrading_) {
    if (argc < 3) {
      std::cerr << usage.str();
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
      std::cerr << usage.str();
      exit(0);
    }

    int read_idx_ = 2;
    is_spread_ = strcmp(argv[read_idx_], "SPREAD") == 0;
    is_fly_ = strcmp(argv[read_idx_], "FLY") == 0;
    is_structure_ = strcmp(argv[read_idx_], "STRUCTURE") == 0;

    if (is_spread_) {
      if (argc < 6) {
        std::cerr << usage.str();
        exit(0);
      }
      read_idx_++;  // for SPREAD
      _this_shortcode_vec_.push_back(argv[read_idx_++]);
      _this_size_vec_.push_back(abs(atoi(argv[read_idx_++])));
      _this_shortcode_vec_.push_back(argv[read_idx_++]);
      _this_size_vec_.push_back(abs(atoi(argv[read_idx_++])));
    } else if (is_fly_) {
      if (argc < 7) {
        std::cerr << usage.str();
        exit(0);
      }
      read_idx_++;  // for FLY
      _this_shortcode_vec_.push_back(argv[read_idx_++]);
      _this_size_vec_.push_back(abs(atoi(argv[read_idx_++])));
      _this_shortcode_vec_.push_back(argv[read_idx_++]);
      _this_size_vec_.push_back(abs(atoi(argv[read_idx_++])));
      _this_shortcode_vec_.push_back(argv[read_idx_++]);
      _this_size_vec_.push_back(abs(atoi(argv[read_idx_++])));
    } else if (is_structure_) {
      read_idx_++;
      structure_shc_ = argv[read_idx_++];
      HFSAT::CurveUtils::GetSpreadShortcodes(structure_shc_, _this_shortcode_vec_);
    } else {
      shc_ = argv[read_idx_++];
      _this_shortcode_vec_.push_back(shc_);
    }
    tradingdate_ = atoi(argv[read_idx_++]);

    if ((tradingdate_ < MIN_YYYYMMDD) || (tradingdate_ > MAX_YYYYMMDD)) {
      std::cerr << tradingdate_ << " not logical date" << std::endl;
      exit(0);
    }
    if (argc > read_idx_) {
      if (!isPL) max_levels_ = std::min(15, atoi(argv[read_idx_++]));
    }
    if (argc > read_idx_) {
      const char *t_start_time_ = argv[read_idx_++];
      if (isdigit(t_start_time_[0])) {
        start_unix_time_ = HFSAT::DateTime::GetTimeFromTZHHMM(tradingdate_, atoi(t_start_time_), "UTC");
      } else {
        start_unix_time_ = HFSAT::DateTime::GetTimeFromTZHHMM(tradingdate_, atoi(t_start_time_ + 4), t_start_time_);
      }
    }
    if (argc > read_idx_) {
      const char *t_end_time_ = argv[read_idx_++];
      if (isdigit(t_end_time_[0])) {
        end_unix_time_ = HFSAT::DateTime::GetTimeFromTZHHMM(tradingdate_, atoi(t_end_time_), "UTC");
      } else {
        end_unix_time_ = HFSAT::DateTime::GetTimeFromTZHHMM(tradingdate_, atoi(t_end_time_ + 4), t_end_time_);
      }
    }
    if (argc > read_idx_) {
      defmode_ = atoi(argv[read_idx_++]);
    }
  }

  if (max_levels_ <= 0) {
    max_levels_ = 10;
  }

  // --- Make object of CommonSMVSource and use it as an API -------------------------------------------
  CommonSMVSource *common_smv_source = new CommonSMVSource(_this_shortcode_vec_, tradingdate_, livetrading_);

  // Set the required book type and network info filename
  if (set_book_type_) {
    common_smv_source->SetBookType(isNTP, isNTPORD, isBMFEq, isPL, isQuincySource, false, false, true, fpga_available,
                                   isCHIXORD);
  }
  if (isRTSOF) common_smv_source->SetRTSOFBookType();
  common_smv_source->SetNetworkAccountInfoFilename(network_account_info_filename_);

  // Initialize the smv source after setting the required variables
  common_smv_source->Initialize();

  // Get the smv and watch after creating the source
  HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map_ = common_smv_source->getSMVMap();
  HFSAT::Watch &watch_ = common_smv_source->getWatch();

  printf("\033c");  // clear screen

  TestBook test_book_(sid_to_smv_ptr_map_, watch_, max_levels_, defmode_, _this_size_vec_, start_unix_time_,
                      end_unix_time_);

  // Subscribe yourself to the smv
  if ((!is_fly_) && (!is_spread_) && (!is_structure_)) {
    // We need to sunscribe to only shortcode SMV here
    // Assuming first shortcode in the map is always the one given as argument since we are pushing it at the start
    HFSAT::SecurityMarketView *p_smv_ = common_smv_source->getSMV();
    p_smv_->subscribe_price_type(&test_book_, HFSAT::kPriceTypeMktSizeWPrice);
    if (max_levels_ > 1) {
      p_smv_->subscribe_L2(&test_book_);
    }
  } else {
    if (!is_structure_) {
      for (auto i = 0u; i < common_smv_source->getNumSecId(); i++) {
        HFSAT::SecurityMarketView *p_smv_ = sid_to_smv_ptr_map_[i];
        p_smv_->subscribe_price_type(&test_book_, HFSAT::kPriceTypeMktSizeWPrice);
        if (max_levels_ > 1) {
          p_smv_->subscribe_L2(&test_book_);
        }
      }
    } else {
      HFSAT::SpreadMarketView *p_spread_market_view_ = common_smv_source->getSpreadMV(structure_shc_);
      p_spread_market_view_->SubscribeSpreadMarketView(&test_book_);
    }
  }

  // start event loop : Runs historical dispatcher, callbacks of MktUpdate, and TradePrint
  common_smv_source->Run();

  // --------------------------------- API use over------------------------------------------------------

  if ((defmode_ == 2) || (defmode_ == 1)) {
    printf("Day Over. Enter to exit ");
    fflush(stdout);
  }

  return 0;
}
