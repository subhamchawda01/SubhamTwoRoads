#ifndef _TECHNICAL_INDICATORS_HPP_
#define _TECHNICAL_INDICATORS_HPP_

#include <iostream>
#include <string>
#include <inttypes.h>
#include <unordered_set>
#include <unordered_map>
#include <stdlib.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <mysql_connection.h>
#include "dvccode/Utils/get_db_config.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "baseinfra/MinuteBar/db_update_nse.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MinuteBar/db_cash_corporate_action.hpp"

namespace HFSAT {

class TechnicalsIndicators {
 public:
  static TechnicalsIndicators &GetUniqueInstance(std::string input_date);
  ~TechnicalsIndicators() {}

  void CalculateSMA() {
    for (auto product : product_closing_prices) {
      // closing price saved
      double product_daylast100 = 0;
      double product_day50 = 0;
      double product_day100 = 0;
      double product_day200 = 0;
      if (product.second.size() < 200) continue;
      for (int index = 0; index < 200; index++) {
        if (index < 50) product_day50 += product.second[index];
        if (index < 100) product_day100 += product.second[index];
        if (index >= 100) product_daylast100 += product.second[index];
        product_day200 += product.second[index];
      }
      product_day50SMA[product.first] = product_day50 / 50;
      product_day100SMA[product.first] = product_day100 / 100;
      product_daylast100SMA[product.first] = product_daylast100 / 100;
      product_day200SMA[product.first] = product_day200 / 200;
    }
  }

  void CalculateMACD() {
    for (auto product : product_daylast100SMA) {
      double EMATODAY, EMAPREV = product_daylast100SMA[product.first];
      double days = 100;
      double MACD[100] = {0};
      for (int index = 99; index >= 11; index--) {
        days++;
        EMATODAY = (product_closing_prices[product.first][index] * (2 / (days + 1)) + EMAPREV * (1 - (2 / (days + 1))));

        MACD[index] = EMATODAY;
        EMAPREV = EMATODAY;
      }
      //    std::cout << MACD26 << " MACD26 " << MACD12 << std::endl;
      product_MACD[product.first] = MACD[11] - MACD[25];
      // CALCULATE SIGNAL LINE
      days = 1;
      EMAPREV = MACD[19] - MACD[33];
      for (int index = 18; index >= 11; index--) {
        days++;
        int macd_t = MACD[index] - MACD[index + 14];
        EMATODAY = (macd_t * (2 / (days + 1)) + EMAPREV * (1 - (2 / (days + 1))));
        EMAPREV = EMATODAY;
      }
      product_signal_line[product.first] = EMATODAY;
    }
  }

  void CalculateRSI() {  // 14 days data  close price
    // only first calculated NEED MORE CODE
    for (auto product : product_closing_prices) {
      if (product.second.size() < 15) continue;
      double last_price = product.second[14];
      double gain_per = 0, loss_per = 0;
      for (int index = 13; index >= 1; index--) {
        if (last_price > product.second[index])
          loss_per += abs(last_price - product.second[index]) / last_price;
        else
          gain_per += abs(last_price - product.second[index]) / last_price;
        last_price = product.second[index];
      }
      double curr_gain_per = 0, curr_loss_per = 0;
      if (product.second[1] > product.second[0])
        curr_loss_per = abs(product.second[1] - product.second[0]) / product.second[1];
      else
        curr_gain_per = abs(product.second[1] - product.second[0]) / product.second[1];
      double RSI = 100 - (100 / (1 + (gain_per + curr_gain_per) / (loss_per + curr_loss_per)));
      product_RSI[product.first] = RSI;
      // over bought/oversold
    }
  }

  void CalculateMFI() {
    // When the price advances from one period to the next Raw Money Flow is positive and it is added to Positive Money
    // Flow.
    // When Raw Money Flow is negative because the price dropped that period, it is added to Negative Money Flow.
    // 14 days
    for (auto product : product_closing_prices) {
      if (product.second.size() < 15) continue;
      double typical_price = (product_closing_prices[product.first][14] + product_high[product.first][14] +
                              product_low[product.first][14]) /
                             3;
      double prev_raw_money_flow = product_closing_prices[product.first][14] * typical_price;
      double neg_money_flow = 0, pos_money_flow = 0;
      for (int index = 13; index > 0; index--) {
        typical_price = (product_closing_prices[product.first][index] + product_high[product.first][index] +
                         product_low[product.first][index]) /
                        3;
        double raw_money_flow = product_closing_prices[product.first][index] * typical_price;
        if (raw_money_flow > prev_raw_money_flow)
          pos_money_flow += raw_money_flow;
        else if (raw_money_flow < prev_raw_money_flow)
          neg_money_flow += raw_money_flow;
        prev_raw_money_flow = raw_money_flow;
      }
      double money_ratio = pos_money_flow / neg_money_flow;
      double MFI = 100 - (100 / (1 + money_ratio));
      product_MFI[product.first] = MFI;
      // MFI generates three main signals – overbought/oversold conditions, failure swings, and divergences.
    }
  }

  void CalculateCCI() {
    // 20 days
    // 	CCI= .015×Mean Deviation Typical Price - Moving Average
    for (auto product : product_closing_prices) {
      if (product.second.size() < 20) continue;
      double typical_price[20];
      double ag_typical_price = 0;
      for (int index = 0; index < 20; index++) {
        typical_price[index] = (product_closing_prices[product.first][index] + product_high[product.first][index] +
                                product_low[product.first][index]) /
                               3;
        ag_typical_price += typical_price[index];
      }
      double MA = ag_typical_price / 20;
      double sum = 0;
      for (int index = 0; index < 20; index++) {
        sum = abs(typical_price[index] - MA);
      }
      product_CCI[product.first] = sum / 20;
    }
  }

  void CalculateWILLIAM() {
    // 14 days
    for (auto product : product_closing_prices) {
      if (product.second.size() < 15) continue;
      double highest_close = product_closing_prices[product.first][14];
      double lowest_close = product_closing_prices[product.first][14];
      for (int index = 13; index >= 0; index--) {
        highest_close = std::max(highest_close, product_closing_prices[product.first][index]);
        lowest_close = std::min(lowest_close, product_closing_prices[product.first][index]);
      }
      product_WILLIAM[product.first] =
          (highest_close - product_closing_prices[product.first][0]) / (highest_close - lowest_close);
    }
  }

  void CalculateADX() {
    /*
    +DI=( ATR Smoothed +DM)×100
    -DI=(
    ATR Smoothed -DM )×100
    DX=(
    ∣+DI+-DI∣
    ∣+DI−-DI|
     )×100
    ADX=
    (Prior ADX×13)+Current A
    +DM (Directional Movement)=Current High−PH
    PH=Previous High
    -DM=Previous Low−Current Low
    Smoothed +/-DM= DM−(  DM+CDM
    CDM=Current DM
    ATR=Average True Range
    */
    /*
            for ( auto product : product_volumes){
            double pdm[14]={0},ndm[14]={0},TR[14]={0}, ATR=0;
            for (int index=0; index<14;index++){
                    double moveUp = product_high[product.first][index] -product_high[product.first][index-1];
                    double moveDown = product_low[product.first][index-1] -product_low[product.first][index];
            if ( moveUp > moveDown && moveUp > 0 ) pdm[index]  = moveUp;
            if ( moveDown > moveUp && moveDown > 0 ) ndm[index]  = moveDown;
            TR[index] = std::max(product_high[product.first][index] - product_low[product.first][index] ,
       std::max(product_high[product.first][index] -
       product_close_price[product.first][index-1],product_low[product.first][index]-
       product_close_price[product.first][index-1]));
            ATR += TR[index];
            }
            // ATR /=14;
            double smoothed_pdm, smoothed_ndm;
            for (int index = 13; index>= 0;index--){
                    smoothed_pdm += pdm[index];
                    smoothed_ndm += ndm[index];
            }
            smoothed_pdm = smoothed_pdm - (smoothed_pdm/14) + pdm[0];
            smoothed_ndm = smoothed_ndm - (smoothed_ndm/14) + ndm[0];
            // ONLY BASE CASE FOR TR EIXST NOW
            double pos_di = 100 * (smoothed_pdm/ATR);
            double neg_di = 100 * (smoothed_ndm/ATR);
            double DMI = 100 * abs((pos_di - neg_di )(pos_di + neg_di));
            double PEMATODAY, PEMAPREV = dm[13];
            double NEMATODAY, NEMAPREV = ndm[13];
            int days = 1;
            for (int index = 12; index>= 0;index--){

            }
            double pos_di = 100 * ( PEMATODAY /ATR);
            double neg_di = 100 * ( NEMATODAY /ATR);
            double adx  =  100 *  (
    */
  }

  void CalculatePSAR() {
    // 5days
    /*
    RPSAR=Prior PSAR + [Prior AF(Prior EP-Prior PSAR)]
    FPSAR=Prior PSAR − [Prior AF(Prior PSAR-Prior EP)]
    where:
    RPSAR = Rising PSAR
    AF = Acceleration Factor, it starts at 0.02 and increases by 0.02, up to a maximum of 0.2, each time the extreme
    point makes a new low (falling SAR) or high(rising SAR)
    FPSAR = Falling PSAR
    EP = Extreme Point, the lowest low in the current downtrend(falling SAR)or the highest high in the current
    uptrend(rising SAR)
    */
    /*
    double AF = .2;
            for ( auto product : product_volumes){
                    for(int index=0;index<5;index++){
                            double moveUp = product_high[product.first][index] -product_high[product.first][index-1];
                    double moveDown = product_low[product.first][index-1] -product_low[product.first][index];
            if ( moveUp > moveDown && moveUp > 0 ) pdm[index]  = moveUp;
            if ( moveDown > moveUp && moveDown > 0 ) ndm[index]  = moveDown;

                    }
            }
    */
  }

  void CalculateAvgVolume() {
    for (auto product : product_volumes) {
      double tot_volume = 0;
      if (product.second.size() < 10) continue;
      // std::cout<< product.first <<std::endl;
      for (int index = 0; index < 10; index++) {
        tot_volume += product.second[index];
      }  // std::cout<<product.second[index]<<std::endl;}
      product_10AvgVolume[product.first] = tot_volume / 10;
    }
  }
  // only cash exchan and shortcode same
  void PopulateValues(std::string shortcode_, double low, double high, double close_price, double volume,
                      bool current_day) {
    // don't populate for product not in current day bhavcopy
    if (product_low.find(shortcode_) == product_low.end()) {
      if (current_day == false) return;
      product_list.insert(shortcode_);
      product_low[shortcode_];
      product_high[shortcode_];
      product_volumes[shortcode_];
      product_closing_prices[shortcode_];
    }
    product_low[shortcode_].push_back(low);
    product_high[shortcode_].push_back(high);
    product_volumes[shortcode_].push_back(volume);
    product_closing_prices[shortcode_].push_back(close_price);
  }

  void PushIndicatorToDB() {
    for (auto product : product_list) {
      if (product_day200SMA.find(product) == product_day200SMA.end() ||
          (product_MACD.find(product) == product_MACD.end() ||
           product_10AvgVolume.find(product) == product_10AvgVolume.end())) {
        continue;
      }
      /*      std::cout << "PRODUCT: " << product << " 50SMA " << product_day50SMA[product] << " 100SMA "
                << product_day100SMA[product] << " 200SMA " << product_day200SMA[product] << " MACD "
                << product_MACD[product] << " RSI " << product_RSI[product] << " MFI " << product_MFI[product]
                << " CCI " << product_CCI[product] << " WILLIAM " << product_WILLIAM[product] << " 10AvgVolume "
                << product_10AvgVolume[product] << std::endl;
 */ db_update_nse
          .PrepareTechnicalMultipleRows(product, product_day50SMA[product], product_day100SMA[product],
                                        product_day200SMA[product], product_MACD[product], product_RSI[product],
                                        product_MFI[product], product_CCI[product], product_WILLIAM[product],
                                        product_10AvgVolume[product], product_signal_line[product]);
    }
  }

  void DumpIndicatorToDB() { db_update_nse.ExecuteTechnicalMultipleRows(); }
  void LoadBhavCopyParams(int date_, bool current_day);
  double getPreviousAdjust(std::string shortcode) {
    if (previous_adjust_to_shortcode.find(shortcode) == previous_adjust_to_shortcode.end()) {
      previous_adjust_to_shortcode[shortcode] = 1;
      return previous_adjust_to_shortcode[shortcode];
    } else {
      return previous_adjust_to_shortcode[shortcode];
    }
  }
  void updatePreviousAdjust(std::string shortcode, double adjust_val) {
    previous_adjust_to_shortcode[shortcode] = adjust_val;
  }

 private:
  TechnicalsIndicators(std::string input_date);
  void GenerateTechnicalIndicator();
  static TechnicalsIndicators *unique_instance_;
  HFSAT::DbUpdateNse &db_update_nse;
  std::string input_date;
  std::unordered_set<std::string> product_list;
  HFSAT::CashCorporateAction& cashCorporateAction; 
  std::unordered_map<std::string, double> previous_adjust_to_shortcode;
  std::unordered_map<std::string, double> product_daylast100SMA;
  std::unordered_map<std::string, double> product_day50SMA;
  std::unordered_map<std::string, double> product_day100SMA;
  std::unordered_map<std::string, double> product_day200SMA;
  std::unordered_map<std::string, double> product_MACD;
  std::unordered_map<std::string, double> product_RSI;
  std::unordered_map<std::string, double> product_MFI;
  std::unordered_map<std::string, double> product_CCI;
  std::unordered_map<std::string, double> product_WILLIAM;
  std::unordered_map<std::string, double> product_ADX;
  std::unordered_map<std::string, double> product_PSAR;
  std::unordered_map<std::string, double> product_10AvgVolume;
  std::unordered_map<std::string, double> product_signal_line;
  std::unordered_map<std::string, std::vector<double>> product_closing_prices;
  std::unordered_map<std::string, std::vector<double>> product_high;
  std::unordered_map<std::string, std::vector<double>> product_low;
  std::unordered_map<std::string, std::vector<double>> product_volumes;
};

}  // namespace HFSAT

#endif
