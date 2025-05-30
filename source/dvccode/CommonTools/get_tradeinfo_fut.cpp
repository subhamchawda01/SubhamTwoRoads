#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <numeric>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#define MAX_LINE_SIZE 1024

int lotsize_fut0, lotsize_fut1;
double slipage_fut0 = 0;
double slipage_fut1 = 0;
std::string strat_ftrade_path;
#define MAX_LINE_LENGTH 1024

struct fut_product {
  int trade_lot;
  double trade_price;
};

std::string trim_str(const std::string &line) {
  const char *WhiteSpace = " \t\v\r\n";
  std::size_t start = line.find_first_not_of(WhiteSpace);
  std::size_t end = line.find_last_not_of(WhiteSpace);
  return start == end ? std::string() : line.substr(start, end - start + 1);
}

bool getLotSize(std::string product, std::string date) {
  char buffer[MAX_LINE_LENGTH];
  std::ifstream lotsize_file;
  std::string lotsize_path = "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_" + date + ".csv";
  //  std::cout << "Lot Size FILE : " << lotsize_path.c_str() << std::end;
  lotsize_file.open(lotsize_path.c_str(), std::ifstream::in);

  if (!lotsize_file.is_open()) {
    std::cerr << "Failed To LOTSIZES For Reading : " << lotsize_path << " System Error : " << strerror(errno)
              << std::endl;
    exit(-1);
  }
  while (lotsize_file.good()) {
    lotsize_file.getline(buffer, MAX_LINE_LENGTH);
    //	HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
    //	std::vector<const char *> const &tokens = pst.GetTokens(",");
    std::vector<std::string> tokens;
    HFSAT::PerishableStringTokenizer::StringSplit(buffer, ',', tokens);
    if (tokens.size() < 5) continue;
    //	std::cout << trim_str(tokens[1]) << "1" <<std::endl;
    if (trim_str(tokens[1]) == product) {
      lotsize_fut0 = stoi(tokens[2]);
      lotsize_fut1 = stoi(tokens[3]);
      return 1;
    }
  }
  return 0;
}

bool getslipge(std::string product) {
  char buffer[MAX_LINE_LENGTH];
  std::ifstream strat_trades;
  //  std::cout<<"STRAT FILE :" << strat_ftrade_path << std::end;
  strat_trades.open(strat_ftrade_path.c_str(), std::ifstream::in);

  if (!strat_trades.is_open()) {
    std::cerr << "Failed To Open Strat File For Reading : " << strat_ftrade_path
              << " System Error : " << strerror(errno) << std::endl;
    exit(-1);
  }
  int slippagefut1_set = 0, slippagefut0_set = 0;
  while (strat_trades.good()) {
    strat_trades.getline(buffer, MAX_LINE_LENGTH);
    HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
    std::vector<const char *> const &tokens = pst.GetTokens();

    if (tokens.size() < 5) continue;
    std::string shortcode_line = tokens[2];
    //	char * shortcode = strtok(tokens[1], ".");
    std::string shortcode = shortcode_line.substr(0, shortcode_line.find("."));
    //	std::cout << shortcode << std::endl;
    if (shortcode == "NSE_" + product + "_FUT0" && slippagefut0_set == 0) {
      int factor = atoi(tokens[4]) / lotsize_fut0;
      slipage_fut0 = atoi(tokens[8]) / factor;
      slippagefut0_set = 1;
    } else if (shortcode == "NSE_" + product + "_FUT1" && slippagefut1_set == 0) {
      int factor = atoi(tokens[4]) / lotsize_fut1;
      slipage_fut1 = atoi(tokens[8]) / factor;
      slippagefut1_set = 1;
    }

    if (slipage_fut0 != 0 && slipage_fut1 != 0) return 1;
  }

  return 0;
}

double cal_spread_v(std::vector<double> vec) {
  double sum = 0;
  for (unsigned int i = 0; i < vec.size(); i++) sum += vec[i];
  sum = sum / vec.size();
  return sum;
}

void compute_spreads(std::string product) {
  // FUT1 and FUT0 trades
  // FUT1 buy and FUT0 sell-> buy speard. FUT1 Sell and FUT0 Buy-> Sell speard

  int start_trade_no = 0, end_trade_no = -1, current_trade_no = 0;
  char buy_trade;
  int fut_no = 0, trade_size = 0;
  double trade_price = 0;
  char buffer[MAX_LINE_LENGTH];
  std::ifstream strat_trades;
  // std::cout<<"STRAT FILE IN COMPUTE :" << strat_ftrade_path << std::end;
  strat_trades.open(strat_ftrade_path.c_str(), std::ifstream::in);

  if (!strat_trades.is_open()) {
    std::cerr << "Failed To Open Strat File For Reading : " << strat_ftrade_path
              << " System Error : " << strerror(errno) << std::endl;
    exit(-1);
  }
  int curr_buy_spread = 0, fut0_pos_buy = 0, fut1_pos_buy = 0, fut0_pos_sell = 0, fut1_pos_sell = 0;
  std::vector<double> buy_spread, sell_spread;
  std::vector<fut_product> fut1_products_buy, fut1_products_sell, fut0_products_buy, fut0_products_sell;
  int buy_spread_size = 0, sell_spread_size = 0;
  int tot_fut1 = 0, tot_fut0 = 0;
  int is_buy_fut0 = 0, is_buy_fut1 = 0, is_sell_fut0 = 0, is_sell_fut1 = 0, fut0_fut0_slipage = 0,
      fut1_fut1_slipage = 0;
  double fut0_pnl = 0, fut1_pnl = 0;
  double tot_fut0_pnl = 0, tot_fut1_pnl = 0;
  while (strat_trades.good()) {
    strat_trades.getline(buffer, MAX_LINE_LENGTH);
    HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
    std::vector<const char *> const &tokens = pst.GetTokens();
    if (tokens.size() < 5) continue;
    std::string shortcode_line = tokens[2];
    if (shortcode_line.find("NSE_" + product + "_FUT")) continue;
    //	std::cout << shortcode_line << std::endl;
    std::string shortcode = shortcode_line.substr(0, shortcode_line.find("."));
    if (shortcode == "NSE_" + product + "_FUT0")
      fut_no = 0;
    else if (shortcode == "NSE_" + product + "_FUT1")
      fut_no = 1;
    buy_trade = tokens[3][0];
    trade_size = atoi(tokens[4]);
    std::string price_str = tokens[5];
    trade_price = std::stod(price_str);
    if (current_trade_no - 1 == end_trade_no)
      std::cout << "##################################################" << std::endl;
    std::cout << current_trade_no << ": FUT - " << fut_no << (buy_trade == 'B' ? " BUY " : " SELL ")
              << " Price: " << trade_price << " Size: " << trade_size << std::endl;
    if (fut_no == 0) {
      if (buy_trade == 'B') {
        fut0_products_buy.push_back(fut_product());
        fut0_products_buy[fut0_products_buy.size() - 1].trade_lot = trade_size;
        fut0_products_buy[fut0_products_buy.size() - 1].trade_price = trade_price;
        is_buy_fut0++;
        fut0_pos_buy += trade_size;
      } else {
        fut0_products_sell.push_back(fut_product());
        fut0_products_sell[fut0_products_sell.size() - 1].trade_lot = trade_size;
        fut0_products_sell[fut0_products_sell.size() - 1].trade_price = trade_price;
        is_sell_fut0++;
        fut0_pos_sell += trade_size;
      }
      tot_fut0++;
    } else {
      if (buy_trade == 'B') {
        fut1_products_buy.push_back(fut_product());
        fut1_products_buy[fut1_products_buy.size() - 1].trade_lot = trade_size;
        fut1_products_buy[fut1_products_buy.size() - 1].trade_price = trade_price;
        is_buy_fut1++;
        fut1_pos_buy += trade_size;
      } else {
        fut1_products_sell.push_back(fut_product());
        fut1_products_sell[fut1_products_sell.size() - 1].trade_lot = trade_size;
        fut1_products_sell[fut1_products_sell.size() - 1].trade_price = trade_price;
        is_sell_fut1++;
        fut1_pos_sell += trade_size;
      }
      tot_fut1++;
      curr_buy_spread += (buy_trade == 'B' ? 1 : -1);
    }
    if (is_buy_fut0 > 0 && is_sell_fut0 > 0) {
      fut0_pnl += (fut0_products_sell[fut0_products_sell.size() - 1].trade_price -
                   fut0_products_buy[fut0_products_buy.size() - 1].trade_price) *
                  lotsize_fut0;
      fut0_pos_sell -= lotsize_fut0;
      fut0_pos_buy -= lotsize_fut0;
      // std::cout << "LOTS FUT0 TRADE OVERALL " << fut1_pos_buy + fut1_pos_sell << " ewr " << fut0_pos_buy +
      // fut0_pos_sell << std::endl;
      is_buy_fut0--;
      is_sell_fut0--;
      fut0_fut0_slipage += 2;
      tot_fut0 -= 2;
      fut0_products_sell.pop_back();
      fut0_products_buy.pop_back();
    } else if (is_buy_fut1 > 0 && is_sell_fut1 > 0) {
      fut1_pnl += (fut1_products_sell[fut1_products_sell.size() - 1].trade_price -
                   fut1_products_buy[fut1_products_buy.size() - 1].trade_price) *
                  lotsize_fut1;
      fut1_pos_sell -= lotsize_fut1;
      fut1_pos_buy -= lotsize_fut1;
      // std::cout << "LOTS  FUT1 TRADE OVERALL " << fut1_pos_buy + fut1_pos_sell << " ewr " << fut0_pos_buy +
      // fut0_pos_sell << std::endl;
      is_buy_fut1--;
      is_sell_fut1--;
      fut1_fut1_slipage += 2;
      tot_fut1 -= 2;
      fut1_products_sell.pop_back();
      fut1_products_buy.pop_back();
    }
    // net_fut_factor = (double)(abs(fut0_pos) - abs(fut1_pos))/std::max(lotsize_fut0, lotsize_fut1);
    // std:: cout << "BEFORE IF OVERALL LOTS  " << fut1_pos_buy - fut1_pos_sell << " " << fut0_pos_buy - fut0_pos_sell
    // <<std::endl;
    int fut0_pos = fut0_pos_buy - fut0_pos_sell;
    int fut1_pos = fut1_pos_buy - fut1_pos_sell;
    int x1 = fut1_pos + fut0_pos;
    int x2 = fut1_pos + fut0_pos;
    int x3 = fut1_pos + fut0_pos;
    if (lotsize_fut0 < lotsize_fut1)
      x2 += (curr_buy_spread > 0 ? -lotsize_fut0 : lotsize_fut0);
    else
      x2 += (curr_buy_spread > 0 ? -lotsize_fut1 : lotsize_fut1);
    if (lotsize_fut0 < lotsize_fut1)
      x3 += (curr_buy_spread > 0 ? lotsize_fut0 : -lotsize_fut0);
    else
      x3 += (curr_buy_spread > 0 ? lotsize_fut1 : -lotsize_fut1);

    if (((is_sell_fut0 && is_buy_fut1) || (is_sell_fut1 && is_buy_fut0)) &&
        (abs(x1) <= abs(x2)) /*&& abs(x1) <= abs(x3))*/) {
      start_trade_no = end_trade_no + 1;
      end_trade_no = current_trade_no;
      std::cout << "TRADE_NO: start, end :: " << start_trade_no << ", " << end_trade_no << "\n" << std::endl;
      int spread_cal = std::min(abs(fut0_pos), abs(fut1_pos));
      double calc_spread_buy, calc_spread_sell;

      if (curr_buy_spread > 0) {
        buy_spread_size += spread_cal;
        int spread_deduct = spread_cal;
        while (true) {
          // std::cout << "DEDUCT :" << spread_deduct << std::endl;
          if (spread_deduct == 0) break;
          if (fut0_products_sell.size() == 0) {
            std::cout << "FUT0 BUY SHOULD NOT HAPPEN" << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut0_products_sell[0].trade_lot) {
            //	std::cout << "SPREAD TOOK " << fut0_products_sell[0].trade_lot << " " <<
            //fut0_products_sell.size()<<std::endl;
            spread_deduct -= fut0_products_sell[0].trade_lot;
            buy_spread.push_back(-fut0_products_sell[0].trade_price * fut0_products_sell[0].trade_lot);
            fut0_products_sell.erase(fut0_products_sell.begin());
          } else {
            buy_spread.push_back(-fut0_products_sell[0].trade_price * spread_deduct);
            fut0_products_sell[0].trade_lot -= spread_deduct;
            //	std::cout << "SELL 0 exist " << fut0_products_sell[0].trade_lot <<std::endl;
            spread_deduct = 0;
          }
        }
        spread_deduct = spread_cal;
        while (true) {
          if (spread_deduct == 0) break;
          if (fut1_products_buy.size() == 0) {
            std::cout << "FUT1 BUY SHOULD NOT HAPPEN" << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut1_products_buy[0].trade_lot) {
            spread_deduct -= fut1_products_buy[0].trade_lot;
            buy_spread.push_back(fut1_products_buy[0].trade_price * fut1_products_buy[0].trade_lot);
            fut1_products_buy.erase(fut1_products_buy.begin());
          } else {
            buy_spread.push_back(fut1_products_buy[0].trade_price * spread_deduct);
            fut1_products_buy[0].trade_lot -= spread_deduct;
            spread_deduct = 0;
          }
        }
        fut1_pos_buy -= spread_cal;
        fut0_pos_sell -= spread_cal;
      } else {
        sell_spread_size += spread_cal;
        int spread_deduct = spread_cal;
        while (true) {
          if (spread_deduct == 0) break;
          if (fut0_products_buy.size() == 0) {
            std::cout << "FUT0 SELL SHOULD NOT HAPPEN" << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut0_products_buy[0].trade_lot) {
            spread_deduct -= fut0_products_buy[0].trade_lot;
            sell_spread.push_back(-fut0_products_buy[0].trade_price * fut0_products_buy[0].trade_lot);
            fut0_products_buy.erase(fut0_products_buy.begin());
          } else {
            sell_spread.push_back(-fut0_products_buy[0].trade_price * spread_deduct);
            fut0_products_buy[0].trade_lot -= spread_deduct;
            spread_deduct = 0;
          }
        }
        spread_deduct = spread_cal;
        while (true) {
          if (spread_deduct == 0) break;
          if (fut1_products_sell.size() == 0) {
            std::cout << "FUT1 SELL SHOULD NOT HAPPEN" << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut1_products_sell[0].trade_lot) {
            spread_deduct -= fut1_products_sell[0].trade_lot;
            sell_spread.push_back(fut1_products_sell[0].trade_price * fut1_products_sell[0].trade_lot);
            fut1_products_sell.erase(fut1_products_sell.begin());
          } else {
            sell_spread.push_back(fut1_products_sell[0].trade_price * spread_deduct);
            fut1_products_sell[0].trade_lot -= spread_deduct;
            spread_deduct = 0;
          }
        }
        fut0_pos_buy -= spread_cal;
        fut1_pos_sell -= spread_cal;
      }
      std::cout << "Total SIZE considered for spread: " << std::endl;
      std::cout << "no_buy, no_sell :: " << buy_spread_size << ", " << sell_spread_size << std::endl;

      calc_spread_buy = std::accumulate(buy_spread.begin(), buy_spread.end(), 0) / (double)(buy_spread_size);
      calc_spread_sell = std::accumulate(sell_spread.begin(), sell_spread.end(), 0) / (double)(sell_spread_size);
      std::cout << "AVG_BUY_SPREAD, AVG_SELL_SPREAD = ";
      if (buy_spread.size() == 0)
        std::cout << "NA ";
      else
        std::cout << calc_spread_buy << " ";
      if (sell_spread.size() == 0)
        std::cout << " NA " << std::endl;
      else
        std::cout << calc_spread_sell << std::endl;
      double spread_av = calc_spread_sell - calc_spread_buy;
      double cal_pnl = 0;
      int tot_sp = std::min(sell_spread_size, buy_spread_size);
      if (buy_spread_size > 0 && sell_spread_size > 0) {
        cal_pnl = spread_av * tot_sp;
        std::cout << "AVG_SPREAD = " << spread_av << std::endl;
        std::cout << "SPREAD_PNL = " << cal_pnl << std::endl;
      } else {
        std::cout << "AVG_SPREAD =  NA" << std::endl;
        std::cout << "SPREAD_PNL =  NA" << std::endl;
      }
      if (fut0_pos_buy > 0 && fut0_pos_sell > 0) {
        // cancel each other
        int lot_cancel = std::min(fut0_pos_buy, fut0_pos_sell);
        fut0_pos_buy -= lot_cancel;
        fut0_pos_sell -= lot_cancel;
        std::vector<double> fut0_fut0_sell, fut0_fut0_buy;
        int spread_deduct = lot_cancel;
        while (true) {
          if (spread_deduct == 0) break;
          if (fut0_products_buy.size() == 0) {
            std::cout << "SHOULD NOT HAPPEN" << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut0_products_buy[0].trade_lot) {
            spread_deduct -= fut0_products_buy[0].trade_lot;
            fut0_fut0_buy.push_back(fut0_products_buy[0].trade_price * fut0_products_buy[0].trade_lot);
            fut0_products_buy.erase(fut0_products_buy.begin());
          } else {
            fut0_fut0_buy.push_back(fut0_products_buy[0].trade_price * spread_deduct);
            fut0_products_buy[0].trade_lot -= spread_deduct;
            spread_deduct = 0;
          }
        }
        spread_deduct = lot_cancel;
        while (true) {
          if (spread_deduct == 0) break;
          if (fut0_products_sell.size() == 0) {
            std::cout << "SHOULD NOT HAPPEN" << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut0_products_sell[0].trade_lot) {
            spread_deduct -= fut0_products_sell[0].trade_lot;
            fut0_fut0_sell.push_back(fut0_products_sell[0].trade_price * fut0_products_sell[0].trade_lot);
            fut0_products_sell.erase(fut0_products_sell.begin());
          } else {
            fut0_fut0_sell.push_back(fut0_products_sell[0].trade_price * spread_deduct);
            fut0_products_sell[0].trade_lot -= spread_deduct;
            spread_deduct = 0;
          }
        }
        calc_spread_buy = std::accumulate(fut0_fut0_buy.begin(), fut0_fut0_buy.end(), 0) / (double)(lot_cancel);
        calc_spread_sell = std::accumulate(fut0_fut0_sell.begin(), fut0_fut0_sell.end(), 0) / (double)(lot_cancel);
        double pnl_cal = lot_cancel * (calc_spread_sell - calc_spread_buy);
        tot_fut0_pnl += pnl_cal;
      } else if (fut1_pos_buy > 0 && fut1_pos_sell > 0) {
        // cancel each other
        std::cout << " CANCELLING EACH OTHER FUT1 " << std::endl;
        int lot_cancel = std::min(fut1_pos_buy, fut1_pos_sell);
        fut1_pos_buy -= lot_cancel;
        fut1_pos_sell -= lot_cancel;
        int spread_deduct = lot_cancel;
        std::vector<double> fut1_fut1_sell, fut1_fut1_buy;
        while (true) {
          if (spread_deduct == 0) break;
          if (fut1_products_buy.size() == 0) {
            std::cout << "SHOULD FUT1 NOT HAPPEN" << spread_deduct << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut1_products_buy[0].trade_lot) {
            spread_deduct -= fut1_products_buy[0].trade_lot;
            fut1_fut1_buy.push_back(fut1_products_buy[0].trade_price * fut1_products_buy[0].trade_lot);
            fut1_products_buy.erase(fut1_products_buy.begin());
          } else {
            fut1_fut1_buy.push_back(fut1_products_buy[0].trade_price * spread_deduct);
            fut1_products_buy[0].trade_lot -= spread_deduct;
            spread_deduct = 0;
          }
        }
        spread_deduct = lot_cancel;
        while (true) {
          if (spread_deduct == 0) break;
          if (fut1_products_sell.size() == 0) {
            std::cout << "SHOULD 2nd FUT1 NOT HAPPEN" << spread_deduct << std::endl;
            exit(-1);
          }
          if (spread_deduct >= fut1_products_sell[0].trade_lot) {
            spread_deduct -= fut1_products_sell[0].trade_lot;
            fut1_fut1_sell.push_back(fut1_products_sell[0].trade_price * fut1_products_sell[0].trade_lot);
            fut1_products_sell.erase(fut1_products_sell.begin());
          } else {
            fut1_fut1_sell.push_back(fut1_products_sell[0].trade_price * spread_deduct);
            fut1_products_sell[0].trade_lot -= spread_deduct;
            spread_deduct = 0;
          }
        }
        calc_spread_buy = std::accumulate(fut1_fut1_buy.begin(), fut1_fut1_buy.end(), 0) / (double)(lot_cancel);
        calc_spread_sell = std::accumulate(fut1_fut1_sell.begin(), fut1_fut1_sell.end(), 0) / (double)(lot_cancel);
        double pnl_cal = lot_cancel * (calc_spread_sell - calc_spread_buy);
        tot_fut1_pnl += pnl_cal;
      }
      fut0_pos = fut0_pos_buy + fut0_pos_sell;
      fut1_pos = fut1_pos_buy + fut1_pos_sell;
      tot_fut0_pnl += fut0_pnl;
      tot_fut1_pnl += fut1_pnl;
      std::cout << "FUT0_FUT0_PNL= " << tot_fut0_pnl << std::endl;
      std::cout << "FUT1_FUT1_PNL= " << tot_fut1_pnl << std::endl;
      double totslipage = tot_fut0 * slipage_fut0 + tot_fut1 * slipage_fut1 + fut0_fut0_slipage * slipage_fut0 +
                          fut1_fut1_slipage * slipage_fut1;
      double result_pnl = tot_fut0_pnl + tot_fut1_pnl + totslipage + cal_pnl;
      std::cout << "NET_PNL: " << tot_fut0_pnl << " + " << tot_fut1_pnl << " + " << totslipage << " + " << cal_pnl
                << " = " << result_pnl << "\n"
                << std::endl;

      std::cout << std::endl;
      // reset values
      start_trade_no = end_trade_no + 1;
      curr_buy_spread = 0;
      is_buy_fut0 = is_buy_fut1 = is_sell_fut0 = is_sell_fut1 = fut0_pnl = fut1_pnl = 0;
      //  std::cout << "Remaining Position: FUT0 " << fut0_pos_buy + fut0_pos_sell << " FUT1: " << fut1_pos_buy +
      //  fut1_pos_sell << std::endl;
    }
    current_trade_no += 1;
  }
  if (fut0_pnl != 0 || fut1_pnl != 0) {
    start_trade_no = end_trade_no + 1;
    current_trade_no--;
    end_trade_no = current_trade_no;
    std::cout << "TRADE_NO: start, end :: " << start_trade_no << ", " << end_trade_no << std::endl << std::endl;
    std::cout << "Total SIZE considered for spread: " << std::endl;
    std::cout << "no_buy, no_sell :: " << buy_spread_size << ", " << sell_spread_size << std::endl;
    double calc_spread_buy = std::accumulate(buy_spread.begin(), buy_spread.end(), 0) / (double)(buy_spread_size);
    double calc_spread_sell = std::accumulate(sell_spread.begin(), sell_spread.end(), 0) / (double)(sell_spread_size);
    std::cout << "AVG_BUY_SPREAD, AVG_SELL_SPREAD = ";
    if (buy_spread.size() == 0)
      std::cout << "NA ";
    else
      std::cout << calc_spread_buy << " ";
    if (sell_spread.size() == 0)
      std::cout << " NA " << std::endl;
    else
      std::cout << calc_spread_sell << std::endl;
    double spread_av = calc_spread_sell - calc_spread_buy;
    double cal_pnl = 0;
    int tot_sp = std::min(sell_spread_size, buy_spread_size);
    if (buy_spread_size > 0 && sell_spread_size > 0) {
      cal_pnl = spread_av * tot_sp;
      std::cout << "AVG_SPREAD = " << spread_av << std::endl;
      std::cout << "SPREAD_PNL = " << cal_pnl << std::endl;
    } else {
      std::cout << "AVG_SPREAD =  NA" << std::endl;
      std::cout << "SPREAD_PNL =  NA" << std::endl;
    }

    tot_fut0_pnl += fut0_pnl;
    tot_fut1_pnl += fut1_pnl;
    std::cout << "FUT0_FUT0_PNL= " << tot_fut0_pnl << std::endl;
    std::cout << "FUT1_FUT1_PNL= " << tot_fut1_pnl << std::endl;
    double totslipage = tot_fut0 * slipage_fut0 + tot_fut1 * slipage_fut1 + fut0_fut0_slipage * slipage_fut0 +
                        fut1_fut1_slipage * slipage_fut1;
    double result_pnl = tot_fut0_pnl + tot_fut1_pnl + totslipage + cal_pnl;
    std::cout << "NET_PNL: " << tot_fut0_pnl << " + " << tot_fut1_pnl << " + " << totslipage << " + " << cal_pnl
              << " = " << result_pnl << "\n"
              << std::endl;
    // std::cout << "Remaining Position: FUT0 " << fut0_pos_buy + fut0_pos_sell << " FUT1: " << fut1_pos_buy +
    // fut1_pos_sell << std::endl;
  }
  std::cout << "END" << std::endl;
}

int main(int argc, char *argv[]) {
  // Check to have config file
  if (argc < 4) {
    std::cerr << "USAGE: EXEC PRODUCT STRAT_ID( strat id or --file filepath) DATE" << std::endl;
    exit(-1);
  }
  std::string strat_id = "0";
  std::string date;
  std::string product = argv[1];
  if (strcmp(argv[2], "--file") == 0) {
    strat_ftrade_path = argv[3];
    date = argv[4];
  } else {
    strat_id = argv[2];
    date = argv[3];
    strat_ftrade_path = "/spare/local/logs/tradelogs/trades." + date + "." + strat_id;
  }
  std::cout << "ARGV " << product << " " << strat_id << " " << date << std::endl;
  std::cout << "strat file used: " << strat_ftrade_path << std::endl << std::endl;

  if (!getLotSize(product, date)) {
    std::cerr << "Entry in LotSize doesn't exist" << std::endl;
    exit(-1);
  }

  if (!getslipge(product)) {
    if (slipage_fut0 == 0 && slipage_fut1 == 0) {
      std::cerr << "FUT0 Trade Not found!!" << std::endl;
      exit(-1);
    }
  }
  std::cout << "FUT0_slipage, FUT1_slipage, FUT0_lotsize, FUT1_lotsize ::" << slipage_fut0 << ", " << slipage_fut1
            << ", " << lotsize_fut0 << ", " << lotsize_fut1 << "\n"
            << std::endl;
  compute_spreads(product);

  return 0;
}
