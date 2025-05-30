#ifndef _DB_UPDATE_API_HPP_
#define _DB_UPDATE_API_HPP_

#include <iostream>
#include <string>
#include <unordered_set>
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

namespace HFSAT {

class DbUpdateNse {
 public:
  static DbUpdateNse &GetUniqueInstance(std::string input_date, bool force_clear_mkt, bool force_clear_bhav, bool is_cash_);
  ~DbUpdateNse() {
    delete res;
    delete stmt;
    delete con;
    delete pstmt_product_insert;
  }

  void UpdateMktDaily(std::string exchange_sym, double minPx, double maxPx, double pxRange, double avgPx,
                      double pxStdev, int totalVolume, int totalTrades, int totalBuyVolume, int totalSellVolume,
                      double volumeWeightedAvgPx, double volumeWeightedPxStdev, double avgL1Size, double minTrdSz,
                      double maxTrdSz, double avgTrdSz, double minSpread, double maxSpread, double avgSpread,
                      std::string shortcode_, std::string expiry_, int strike_, int lotSize, int noOfMsg);

  void UpdateMktMin(int timestamp, std::string exchange_sym, double minPx, double maxPx, double pxRange, double avgPx,
                    double pxStdev, int totalVolume, int totalTrades, int totalBuyVolume, int totalSellVolume,
                    double volumeWeightedAvgPx, double volumeWeightedPxStdev, double avgL1Size, double minTrdSz,
                    double maxTrdSz, double avgTrdSz, double minSpread, double maxSpread, double avgSpread,
                    std::string shortcode_, std::string expiry_, int strike_, int lotSize);

  void PrepareMktMinMultipleRows(int timestamp, std::string exchange_sym, double minPx, double maxPx, double pxRange,
                                 double avgPx, double pxStdev, int totalVolume, int totalTrades, int totalBuyVolume,
                                 int totalSellVolume, double volumeWeightedAvgPx, double volumeWeightedPxStdev,
                                 double avgL1Size, double minTrdSz, double maxTrdSz, double avgTrdSz, double minSpread,
                                 double maxSpread, double avgSpread, std::string shortcode_, std::string expiry_,
                                 int strike_, int lotSize, int noOfMsg);
  void ExecuteMktMinMultipleRows();
  void ExecuteTechnicalMultipleRows();
  void ExecuteBhavCopyCmMultipleRows();
  void ExecuteBhavCopyFoMultipleRows();
  void ExecuteBhavCopyFutOptMultipleRows();
  void UpdateBhavCopyFo(std::string exchange_sym, std::string instrument, std::string symbol, std::string expiry_dt,
                        double strike_pr, std::string option_typ, double open, double high, double low, double close,
                        double settle_pr, double contracts, double val_inlakh, double open_int, double chg_in_oi,
                        std::string timestamp, std::string shortcode_, std::string expiry_, int strike_, int lotSize);
  void PrepareFoBhavCopyMultipleRows(std::string exchange_sym, std::string instrument, std::string symbol,
                                     std::string expiry_dt, double strike_pr, std::string option_typ, double open,
                                     double high, double low, double close, double settle_pr, double contracts,
                                     double val_inlakh, double open_int, double chg_in_oi, std::string timestamp,
                                     std::string shortcode_, std::string expiry_, int strike_, int lotSize);
  void PrepareFutOptBhavCopyMultipleRows(std::string exchange_sym, std::string instrument, std::string symbol,
                                     std::string expiry_dt, double strike_pr, std::string option_typ, double open,
                                     double high, double low, double close, double settle_pr, double contracts,
                                     double val_inlakh, double open_int, double chg_in_oi, std::string timestamp,
                                     std::string shortcode_, std::string expiry_, int strike_, int lotSize);
  void PrepareCmBhavCopyMultipleRows(std::string exchangeSymbol, std::string symbol, std::string series, double open,
                                     double high, double low, double last, double prevclose, double close,
                                     int tottrdqty, double tottrdval, std::string timestamp, int totaltrades,
                                     std::string isin, std::string shortcode_, std::string expiry_, int strike_,
                                     int lotSize);

  void UpdateBhavCopyCm(std::string exchangeSymbol, std::string symbol, std::string series, double open, double high,
                        double low, double last, double prevclose, double close, int tottrdqty, double tottrdval,
                        std::string timestamp, int totaltrades, std::string isin, std::string shortcode_,
                        std::string expiry_, int strike_, int lotSize);
  void UpdateTechnical(std::string exchange_sym, double day50SMA, double day100SMA, double day200SMA, double MACD,
                       double RSI, double MFI, double CCI, double WILLIAM, double AvgVolume10, double product_signal_line);
  void PrepareTechnicalMultipleRows(std::string exchange_sym, double day50SMA, double day100SMA, double day200SMA,
                                    double MACD, double RSI, double MFI, double CCI, double WILLIAM,
                                    double AvgVolume10, double product_signal_line);

  void UpdateProduct(std::string shortcode_, std::string expiry_, int strike_, int lotSize);
  void CheckandUpdateProduct(std::string shortcode_, std::string expiry_, int strike_,
                             int lotSize);
  void GetProductKey();
  void RemoveEntryForCurrentDayMkt();
  void RemoveEntryForCurrentDayMinMkt();
  void RemoveEntryForCurrentDayBhav();
  void RemoveEntryForCurrentDay();
  void RemoveEntryForIndicator();
  void UpdateDb(std::string db_loc){
    try {
	    con->setSchema(db_loc);
        } catch (sql::SQLException &e) {
	    std::cout << "DbUpdateNse::UpdateDb:: ERR: SQLException in " << __FILE__;
	    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
	    std::cout << "# ERR: " << e.what();
	    std::cout << " (MySQL error code: " << e.getErrorCode();
	    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    	    exit(-1);
  	}
  }

 private:
  DbUpdateNse(std::string input_date, bool force_clear, bool force_clear_bhav, bool is_cash_);
  static DbUpdateNse *unique_instance_;
  HFSAT::GetDBConfig &db_config;
  sql::Driver *driver;
  sql::Connection *con;
  sql::Statement *stmt;
  sql::ResultSet *res;
  sql::PreparedStatement *pstmt_product_insert;
  sql::PreparedStatement *pstmt_MktDaily_insert;
  sql::PreparedStatement *pstmt_MktDailyMin_insert;
  sql::PreparedStatement *pstmt_indicator_insert;
  sql::PreparedStatement *pstmt_BhavCopyFo_insert;
  sql::PreparedStatement *pstmt_BhavCopyCm_insert;
  sql::PreparedStatement *pstmt_Multiple_row_insert;
  std::string input_date;
  std::string sql_date;
  std::string query_rows_mkt;
  std::string query_rows_cm_bhavcopy;
  std::string query_rows_fo_bhavcopy;
  std::string query_rows_options_bhavcopy;
  std::string query_rows_technical;
  std::unordered_set<std::string> product_added;
  bool first_row_mkt, first_row_fo_bhav, first_row_fo_bhav_opt, first_row_cm_bhav, first_row_tech, is_cash;
};

}  // namespace HFSAT

#endif
