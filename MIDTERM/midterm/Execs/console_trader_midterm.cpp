/**
   \file Tools/console_trader.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fstream>
#include <iostream>
#include <time.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "baseinfra/BaseTrader/base_live_trader.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/eti_algo_tagging.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"

#define MAX_CONSOLE_ORDER_LIMIT 200
#define MAX_ORDER_LOT_LIMIT 40

#define MAX_CONSOLE_EQUITY_ORDER_LIMIT 50000
#define MAX_EQUITY_ORDER_LOT_LIMIT 20000

#define MAX_CONSOLE_NSE_ORDER_LIMIT 500000

#define START_SEQUENCE 90000000

uint32_t mts = 0;

int total_order_count = 0;
int total_order_size = 0;
HFSAT::ExchSource_t exchange_source = HFSAT::kExchSourceCME;

HFSAT::Watch* watch_;

void print_usage(const char* prog_name) {
  printf("This is the Console Trader program \n");
  printf("Usage:%s [max trading size]\n", prog_name);
}

static struct option options_[] = {{"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

char getAction() {
  char value;
  std::cout << std::endl
            << "1) Enter Order" << std::endl
            << "2) Cancel Order" << std::endl
            << "3) Modify Order" << std::endl
            << "4) Enter FOK Order" << std::endl
            << "5) Enter Immediate or Cancel Order" << std::endl
            << "6) Exit" << std::endl
            << "Action: ";
  std::cin >> value;
  switch (value) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
      break;
    default:
      exit(-1);
  }
  return value;
}

std::string getSymbol() {
  std::string symbol;
  std::cout << std::endl
            << "Symbol: ";
  std::cin >> symbol;
  return symbol;
}

double getPrice() {
  double px;
  std::cout << std::endl
            << "Price: ";
  std::cin >> px;
  return px;
}

int getSize() {
  int size;
  std::cout << std::endl
            << "Size: ";
  std::cin >> size;
  if (mts <= 0) {
    std::cout << std::endl
              << "Max Trading Size: ";
    std::cin >> mts;
  }
  return size;
}

HFSAT::TradeType_t getSide() {
  char value;
  std::cout << std::endl
            << "1) Buy" << std::endl
            << "2) Sell" << std::endl;
  std::cin >> value;
  switch (value) {
    case '1':
      return HFSAT::kTradeTypeBuy;

    case '2':
      return HFSAT::kTradeTypeSell;

    default:
      throw std::exception();
  }
}

int getSeq() {
  static int seqno = START_SEQUENCE;
  return seqno++;
}

int getServerSeq() {
  int value;
  std::cout << std::endl
            << "OrderSeq: " << std::endl;
  std::cin >> value;
  return value;
}

void SendOrder(HFSAT::BaseTrader* tr, const char* inst, HFSAT::FastPriceConvertor* fast_px_converter_, bool _fok_,
               bool ioc) {
  try {
    HFSAT::BaseOrder bord;
    bord.price_ = getPrice();
    bord.int_price_ = fast_px_converter_->GetFastIntPx(bord.price_);
    bord.price_ = fast_px_converter_->GetDoublePx(bord.int_price_);

    uint32_t size_ = getSize();

    if (!mts) {
      std::cerr << "Wrong max-trading-size and size...Try again." << mts << " " << size_ << std::endl;
      mts = 0;
      exit(-1);
    }

    if (((int)size_) <= 0 || ((int)(mts)) <= 0) {
      std::cerr << " Size Requested : " << (int)size_ << " Max Order Size : " << (int)mts << " Fails Sanity Checks \n";
      return;
    }

    bord.buysell_ = getSide();
    bord.security_name_ = inst;
    bord.size_requested_ = mts;
    bord.size_disclosed_ = mts;
    bord.is_fok_ = _fok_;
    bord.is_ioc_ = ioc;

    while (size_ > 0) {
      if (size_ < mts) {
        bord.size_requested_ = size_;
        bord.size_disclosed_ = size_;
        size_ = 0;
      } else
        size_ -= mts;

      if (total_order_count == MAX_ORDER_LOT_LIMIT) {
        std::cerr << " Send Order Fails On : Exceeded Max Order Count, Total Active Order Lots : " << total_order_count
                  << " Total Order Size : " << total_order_size << "\n";
        break;
      }

      if ((exchange_source == HFSAT::kExchSourceBMFEQ &&
           total_order_size + bord.size_requested_ > MAX_CONSOLE_EQUITY_ORDER_LIMIT) ||
          ((exchange_source == HFSAT::kExchSourceNSE_CD || exchange_source == HFSAT::kExchSourceNSE_FO) &&
           total_order_size + bord.size_requested_ > MAX_CONSOLE_NSE_ORDER_LIMIT) ||
          (exchange_source != HFSAT::kExchSourceBMFEQ && exchange_source != HFSAT::kExchSourceNSE_CD &&
           exchange_source != HFSAT::kExchSourceNSE_FO &&
           total_order_size + bord.size_requested_ > MAX_CONSOLE_ORDER_LIMIT)) {
        std::cerr << " Send Order Fails On : Exceeded Max Order Size Placed, Total Order Placed : " << total_order_size
                  << " Allowed : " << (MAX_CONSOLE_ORDER_LIMIT - total_order_size)
                  << " Requested : " << bord.size_requested_ << "\n";
        break;
      }

      bord.client_assigned_order_sequence_ = getSeq();
      std::cout << "Order Sent: " << bord.security_name_ << " " << bord.int_price_ << " " << bord.size_requested_
                << std::endl;

      total_order_count++;
      total_order_size += bord.size_requested_;

      struct timeval tv;
      gettimeofday(&tv, NULL);

      HFSAT::ttime_t temp_(tv);

      watch_->OnTimeReceived(temp_);

      tr->SendTrade(bord);
    }

  } catch (std::exception& e) {
    std::cout << "SendOrder fails: " << e.what();
  }
}

void CancelOrder(HFSAT::BaseTrader* tr, const char* inst) {
  HFSAT::BaseOrder bord;
  bord.server_assigned_order_sequence_ = getServerSeq();

  if (0 == bord.server_assigned_order_sequence_) {
    std::cerr << "Invalid SAOS : " << bord.server_assigned_order_sequence_ << "\n";
    exit(-1);
  }

  bord.security_name_ = inst;  // not really needed in the ORS but livetrader segfaults without it
  struct timeval tv;
  gettimeofday(&tv, NULL);

  HFSAT::ttime_t temp_(tv);

  watch_->OnTimeReceived(temp_);

  tr->Cancel(bord);
}

void ModifyOrder(HFSAT::BaseTrader* tr, HFSAT::FastPriceConvertor* fast_px_converter_, const char* inst) {
  try {
    HFSAT::BaseOrder bord;
    bord.server_assigned_order_sequence_ = getServerSeq();
    bord.security_name_ = inst;  // not really needed in the ORS but livetrader segfaults without it
    int new_size_requested_ = getSize();
    bord.size_disclosed_ = new_size_requested_;
    bord.price_ = getPrice();
    bord.int_price_ = fast_px_converter_->GetFastIntPx(bord.price_);
    struct timeval tv;
    gettimeofday(&tv, NULL);

    HFSAT::ttime_t temp_(tv);

    watch_->OnTimeReceived(temp_);
    tr->Modify(bord, bord.price(), bord.int_price(), new_size_requested_);

  } catch (std::exception& e) {
    std::cout << "ModifyOrder fails: " << e.what();
  }
}

int main(int argc, char** argv) {
  int c;
  int hflag = 0;

  HFSAT::DebugLogger dbglogger_(10240, 1);

  if (argc > 1) mts = atoi(argv[1]);
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", options_, &option_index);
    if (c == -1) break;

    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case '?':
        if (optopt != 'h') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          print_usage(argv[0]);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag) {
    print_usage(argv[0]);
    exit(-1);
  }

  /// get the instrument and corresponding exchange
  std::string shortcode = getSymbol();

  int yyyymmdd = HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate();

  if (std::string::npos != shortcode.find("NSE_")) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd).LoadNSESecurityDefinitions();

    if (std::string::npos != shortcode.find("INR_") || std::string::npos != shortcode.find("_GIND")) {
      exchange_source = HFSAT::kExchSourceNSE_CD;
    } else {
      if (std::string::npos != shortcode.find("_FUT")) {
        exchange_source = HFSAT::kExchSourceNSE_FO;
      } else {
        exchange_source = HFSAT::kExchSourceNSE_EQ;
      }
    }

  } else {
    exchange_source = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode, yyyymmdd);
  }

  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  if (exchange_source == HFSAT::kExchSourceEUREX) {
    HFSAT::DataInfo m_dinfo =
        network_account_info_manager_.GetMarginControlDataInfo(HFSAT::kExchSourceEUREX, "NTAPROD4");
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("EUREX")
        .SetORSControlNetworkConfiguration("127.0.0.1", m_dinfo.bcast_port_);
  }

  if (exchange_source == HFSAT::kExchSourceICE) {
    HFSAT::DataInfo m_dinfo = network_account_info_manager_.GetMarginControlDataInfo(HFSAT::kExchSourceICE, "MSICE1");
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("ICE")
        .SetORSControlNetworkConfiguration("127.0.0.1", m_dinfo.bcast_port_);
  }

  double min_price_increment_ = HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(shortcode, yyyymmdd);

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd);
  const char* exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);

  std::cout << "Using Exchange Symbol : " << exch_symbol_ << "\n";

  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/console_trader_log." << exch_symbol_ << "."
                << HFSAT::DateTime::GetCurrentIsoDateLocal();
    std::string logfilename_ = t_temp_oss_.str();
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::FastPriceConvertor* fast_px_converter_ = new HFSAT::FastPriceConvertor(min_price_increment_);

  watch_ = new HFSAT::Watch(dbglogger_, tradingdate_);

  /// create trader class
  HFSAT::BaseTrader* trader = new HFSAT::BaseLiveTrader(
      exchange_source, network_account_info_manager_.GetDepTradeAccount(exchange_source, shortcode),
      network_account_info_manager_.GetDepTradeHostIp(exchange_source, shortcode),
      network_account_info_manager_.GetDepTradeHostPort(exchange_source, shortcode), *watch_, dbglogger_);

  if (exchange_source == HFSAT::kExchSourceEUREX) {
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("EUREX")
        .TagAlgo(trader->GetClientId(), "CONSOLE", "CONSOLE", "EUREX");
  }
  if (exchange_source == HFSAT::kExchSourceICE) {
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("ICE").TagAlgo(trader->GetClientId(), "CONSOLE", "CONSOLE", "ICE");
  }

  HFSAT::ProfilerTimeInfo time_info{1, 0, 0, 0, 1, 0};
  HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT).Start(time_info);

  while (true) {
    char action = getAction();

    if (action == '1') {
      SendOrder(trader, exch_symbol_, fast_px_converter_, false, false);
    } else if (action == '4') {
      SendOrder(trader, exch_symbol_, fast_px_converter_, true, false);
    } else if (action == '2') {
      CancelOrder(trader, exch_symbol_);
    } else if (action == '5') {
      SendOrder(trader, exch_symbol_, fast_px_converter_, false, true);
    } else if (action == '3') {
      ModifyOrder(trader, fast_px_converter_, exch_symbol_);
    } else {
      exit(-1);
    }
  }
}
