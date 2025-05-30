#include "baseinfra/MinuteBar/db_update_nse.hpp"

namespace HFSAT {

DbUpdateNse *DbUpdateNse::unique_instance_ = nullptr;

DbUpdateNse &DbUpdateNse::GetUniqueInstance(std::string input_date, bool force_clear_mkt, bool force_clear_bhav, bool is_cash_) {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new DbUpdateNse(input_date, force_clear_mkt, force_clear_bhav, is_cash_);
  }
  return *(unique_instance_);
}

DbUpdateNse::DbUpdateNse(std::string input_date_, bool force_clear_mkt, bool force_clear_bhav, bool is_cash_)
    : db_config(HFSAT::GetDBConfig::GetUniqueInstance()) {
  is_cash = is_cash_;
  if (is_cash == true){
  	query_rows_mkt = "insert into MKT_DAILY_MIN_DATA_CASH values";
  } else{
  	query_rows_mkt = "insert into MKT_DAILY_MIN_DATA_FUT values";
  }
  query_rows_cm_bhavcopy = "insert into BHAV_COPY_DETAILS_CASH values";
  query_rows_fo_bhavcopy = "insert into BHAV_COPY_DETAILS_FO values";
  query_rows_technical = "insert into INDICATOR_DAILY_DATA values";
  query_rows_options_bhavcopy = "insert into BHAV_COPY_DETAILS_FO values";
  first_row_mkt = first_row_fo_bhav_opt = first_row_fo_bhav = first_row_cm_bhav = first_row_tech = true;
  std::string db_ip = db_config.GetDbIp();
  std::string db_user = db_config.GetDbUser();
  std::string db_pass = db_config.GetDbPassword();
  std::cout << "Db_Ip: " << db_ip << " "
            << "\nDb_User: " << db_user << "\nDb_Pass: " << db_pass << std::endl;
  try {
    driver = get_driver_instance();
    con = driver->connect("tcp://" + db_ip, db_user, db_pass);
    /* Connect to the MySQL test database */
    con->setSchema("NSE_MTBT");
    stmt = con->createStatement();
    res = stmt->executeQuery("SELECT 'Hello World!' AS _message");
    while (res->next()) {
      std::cout << "\t ...MYSQL Connnected... \n";
    }
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::INTITAZE:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
  input_date = input_date_;
  sql_date = input_date_.substr(0, 4) + "-" + input_date_.substr(4, 2) + "-" + input_date_.substr(6, 2);
  std::cout << "DATE: " << sql_date << std::endl;

  // Pre load the sql queries
  std::cout << "GENERATING STATEMENTS: 1" << std::endl;
  pstmt_product_insert = con->prepareStatement(
      "INSERT INTO PRODUCT(shortcode,expiry,strikePrice,lotSize) VALUES (?,?,?,?)");
  std::cout << "GENERATING STATEMENTS: 2" << std::endl;
  pstmt_MktDaily_insert = con->prepareStatement(
      "INSERT INTO "
      "MKT_DAILY_DATA(shortcode,day,minPx,maxPx,pxRange,avgPx,pxStdev,totalVolume,totalTrades,totalBuyVolume,"
      "totalSellVolume,volumeWeightedAvgPx,volumeWeightedPxStdev,avgL1Size,minTrdSz,maxTrdSz,avgTrdSz,minSpread,"
      "maxSpread,avgSpread,noOfMsg) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

  std::cout << "GENERATING STATEMENTS: 3" << std::endl;
  con->setSchema("NSE_MTBT_MIN");
  if (is_cash == true){
  	pstmt_MktDailyMin_insert = con->prepareStatement(
      	"INSERT INTO "
      	"MKT_DAILY_MIN_DATA_CASH(shortcode,day,time,minPx,maxPx,pxRange,avgPx,pxStdev,totalVolume,totalTrades,"
      	"totalBuyVolume,totalSellVolume,volumeWeightedAvgPx,volumeWeightedPxStdev,avgL1Size,minTrdSz,maxTrdSz,avgTrdSz,"
      	"minSpread,maxSpread,avgSpread) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
  } else {
	pstmt_MktDailyMin_insert = con->prepareStatement(
        "INSERT INTO "
        "MKT_DAILY_MIN_DATA_FUT(shortcode,day,time,minPx,maxPx,pxRange,avgPx,pxStdev,totalVolume,totalTrades,"
        "totalBuyVolume,totalSellVolume,volumeWeightedAvgPx,volumeWeightedPxStdev,avgL1Size,minTrdSz,maxTrdSz,avgTrdSz,"
        "minSpread,maxSpread,avgSpread) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
  }
  std::cout << "GENERATING STATEMENTS: 4" << std::endl;
  con->setSchema("NSE_MTBT");
  pstmt_BhavCopyCm_insert = con->prepareStatement(
      "INSERT INTO "
      "BHAV_COPY_DETAILS_CASH(shortcode,symbol,series,open,high,low,last,prevclose,close,tottrdqty,tottrdval,"
      "timestamp,totaltrades,isin) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
  std::cout << "GENERATING STATEMENTS: 5" << std::endl;
  pstmt_BhavCopyFo_insert = con->prepareStatement(
      "INSERT INTO "
      "BHAV_COPY_DETAILS_FO(shortcode,instrument,symbol,expiry_dt,strike_pr,option_typ,open,high,low,close,settle_"
      "pr,contracts,val_inlakh,open_int,chg_in_oi,timestamp) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
  std::cout << "GENERATING STATEMENTS: 6" << std::endl;
  pstmt_indicator_insert = con->prepareStatement(
      "INSERT INTO "
      "INDICATOR_DAILY_DATA(shortcode,timestamp,SMA50,SMA100,SMA200,MACD,RSI,MFI,CCI,WILLIAM,AvgVolume10,SignalLine)"
      "VALUES (?,?,?,?,?,?,?,?,?,?,?,?)");
  std::cout << "GETTING PRIMARY keys from PRODUCT" << std::endl;
  GetProductKey();
  if (force_clear_mkt == true) {
    RemoveEntryForCurrentDayMkt();
    con->setSchema("NSE_MTBT_MIN");
    RemoveEntryForCurrentDayMinMkt();
   con->setSchema("NSE_MTBT");
  }
  if (force_clear_bhav == true) {
    RemoveEntryForCurrentDayBhav();
    con->setSchema("NSE_MTBT_BHAV_ONLY");
    RemoveEntryForCurrentDay();
    con->setSchema("NSE_MTBT");
  }
}

void DbUpdateNse::RemoveEntryForCurrentDayMkt() {
  std::cout << "REMOVING FROM MKT_DAILY_DATA" << std::endl;
  try {
    stmt->executeQuery("delete from MKT_DAILY_DATA where day = '" + sql_date + "' ;");
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::RemoveEntryForCurrentDayMkt:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
  }
}

void DbUpdateNse::RemoveEntryForCurrentDayMinMkt() {
  std::cout << "REMOVING FROM MKT_MIN_DAILY_DATA_CASH" << std::endl;
  try {
    stmt->executeQuery("delete from MKT_DAILY_MIN_DATA_CASH where day = '" + sql_date + "' ;");
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::RemoveEntryForCurrentDayMkt:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
  }
  std::cout << "REMOVING FROM MKT_MIN_DAILY_DATA_FUT" << std::endl;
  try {
    stmt->executeQuery("delete from MKT_DAILY_MIN_DATA_FUT where day = '" + sql_date + "' ;");
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::RemoveEntryForCurrentDayMkt:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
  }
}

void DbUpdateNse::RemoveEntryForCurrentDayBhav() {
  std::cout << "REMOVING FROM BHAV_COPY_DETAILS_CASH" << std::endl;
  try {
    stmt->executeQuery("delete from BHAV_COPY_DETAILS_CASH where timestamp = '" + sql_date + "' ;");
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::RemoveEntryForCurrentDayBhav:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
  }

  std::cout << "REMOVING FROM BHAV_COPY_DETAILS_FO" << std::endl;
  try {
    stmt->executeQuery("delete from BHAV_COPY_DETAILS_FO where timestamp = '" + sql_date + "' ;");
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::RemoveEntryForCurrentDayBhav:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
  }
}

void DbUpdateNse::RemoveEntryForCurrentDay() {

  std::cout << "REMOVING FROM BHAV_COPY_DETAILS_FO FO_ONLY" << std::endl;
  try {
    stmt->executeQuery("delete from BHAV_COPY_DETAILS_FO where timestamp = '" + sql_date + "' ;");
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::RemoveEntryForCurrentDayBhav:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
  }
}


void DbUpdateNse::RemoveEntryForIndicator() {
  std::cout << "REMOVING FROM INDICATOR_DAILY_DATA" << std::endl;
  try {
    stmt->executeQuery("delete from INDICATOR_DAILY_DATA where timestamp = '" + sql_date + "' ;");
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::RemoveEntryForCurrentDayBhav:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
  }
}

void DbUpdateNse::UpdateBhavCopyCm(std::string exchangeSymbol, std::string symbol, std::string series, double open,
                                   double high, double low, double last, double prevclose, double close, int tottrdqty,
                                   double tottrdval, std::string timestamp, int totaltrades, std::string isin,
                                   std::string shortcode_, std::string expiry_, int strike_, int lotSize) {
  CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize);
  pstmt_BhavCopyCm_insert->setString(1, shortcode_);
  pstmt_BhavCopyCm_insert->setString(2, symbol);
  pstmt_BhavCopyCm_insert->setString(3, series);
  pstmt_BhavCopyCm_insert->setDouble(4, open);
  pstmt_BhavCopyCm_insert->setDouble(5, high);
  pstmt_BhavCopyCm_insert->setDouble(6, low);
  pstmt_BhavCopyCm_insert->setDouble(7, last);
  pstmt_BhavCopyCm_insert->setDouble(8, prevclose);
  pstmt_BhavCopyCm_insert->setDouble(9, close);
  pstmt_BhavCopyCm_insert->setInt(10, tottrdqty);
  pstmt_BhavCopyCm_insert->setDouble(11, tottrdval);
  pstmt_BhavCopyCm_insert->setString(12, sql_date);
  pstmt_BhavCopyCm_insert->setInt(13, totaltrades);
  pstmt_BhavCopyCm_insert->setString(14, isin);
  try {
    pstmt_BhavCopyCm_insert->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::UpdateBhavCopyCm:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
}

void DbUpdateNse::UpdateBhavCopyFo(std::string exchange_sym, std::string instrument, std::string symbol,
                                   std::string expiry_dt, double strike_pr, std::string option_typ, double open,
                                   double high, double low, double close, double settle_pr, double contracts,
                                   double val_inlakh, double open_int, double chg_in_oi, std::string timestamp,
                                   std::string shortcode_, std::string expiry_, int strike_, int lotSize) {
  CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize);
  pstmt_BhavCopyFo_insert->setString(1, shortcode_);
  pstmt_BhavCopyFo_insert->setString(2, instrument);
  pstmt_BhavCopyFo_insert->setString(3, symbol);
  pstmt_BhavCopyFo_insert->setString(4, expiry_dt);
  pstmt_BhavCopyFo_insert->setDouble(5, strike_pr);
  pstmt_BhavCopyFo_insert->setString(6, option_typ);
  pstmt_BhavCopyFo_insert->setDouble(7, open);
  pstmt_BhavCopyFo_insert->setDouble(8, high);
  pstmt_BhavCopyFo_insert->setDouble(9, low);
  pstmt_BhavCopyFo_insert->setDouble(10, close);
  pstmt_BhavCopyFo_insert->setDouble(11, settle_pr);
  pstmt_BhavCopyFo_insert->setDouble(12, contracts);
  pstmt_BhavCopyFo_insert->setDouble(13, val_inlakh);
  pstmt_BhavCopyFo_insert->setDouble(14, open_int);
  pstmt_BhavCopyFo_insert->setDouble(15, chg_in_oi);
  pstmt_BhavCopyFo_insert->setString(16, sql_date);
  try {
    pstmt_BhavCopyFo_insert->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::UpdateBhavCopyFo:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
}

void DbUpdateNse::UpdateMktDaily(std::string exchange_sym, double minPx, double maxPx, double pxRange, double avgPx,
                                 double pxStdev, int totalVolume, int totalTrades, int totalBuyVolume,
                                 int totalSellVolume, double volumeWeightedAvgPx, double volumeWeightedPxStdev,
                                 double avgL1Size, double minTrdSz, double maxTrdSz, double avgTrdSz, double minSpread,
                                 double maxSpread, double avgSpread, std::string shortcode_, std::string expiry_,
                                 int strike_, int lotSize, int noOfMsg) {
  CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize);
  pstmt_MktDaily_insert->setString(1, shortcode_);
  pstmt_MktDaily_insert->setString(2, sql_date);
  pstmt_MktDaily_insert->setDouble(3, minPx);
  pstmt_MktDaily_insert->setDouble(4, maxPx);
  pstmt_MktDaily_insert->setDouble(5, pxRange);
  pstmt_MktDaily_insert->setDouble(6, avgPx);
  if (std::isnan(pxStdev))
    pstmt_MktDaily_insert->setNull(7, 496);
  else
    pstmt_MktDaily_insert->setDouble(7, pxStdev);
  pstmt_MktDaily_insert->setInt(8, totalVolume);
  pstmt_MktDaily_insert->setInt(9, totalTrades);
  pstmt_MktDaily_insert->setInt(10, totalBuyVolume);
  pstmt_MktDaily_insert->setInt(11, totalSellVolume);
  pstmt_MktDaily_insert->setDouble(12, volumeWeightedAvgPx);
  if (std::isnan(volumeWeightedPxStdev))
    pstmt_MktDaily_insert->setNull(13, 496);
  else
    pstmt_MktDaily_insert->setDouble(13, volumeWeightedPxStdev);
  pstmt_MktDaily_insert->setDouble(14, avgL1Size);
  pstmt_MktDaily_insert->setDouble(15, minTrdSz);
  pstmt_MktDaily_insert->setDouble(16, maxTrdSz);
  pstmt_MktDaily_insert->setDouble(17, avgTrdSz);
  pstmt_MktDaily_insert->setDouble(18, minSpread);
  pstmt_MktDaily_insert->setDouble(19, maxSpread);
  pstmt_MktDaily_insert->setDouble(20, avgSpread);
  pstmt_MktDaily_insert->setInt(21, noOfMsg);
  try {
    pstmt_MktDaily_insert->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::UpdateMktDaily:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
}

void DbUpdateNse::UpdateMktMin(int timestamp, std::string exchange_sym, double minPx, double maxPx, double pxRange,
                               double avgPx, double pxStdev, int totalVolume, int totalTrades, int totalBuyVolume,
                               int totalSellVolume, double volumeWeightedAvgPx, double volumeWeightedPxStdev,
                               double avgL1Size, double minTrdSz, double maxTrdSz, double avgTrdSz, double minSpread,
                               double maxSpread, double avgSpread, std::string shortcode_, std::string expiry_,
                               int strike_, int lotSize) {
  std::string timestamp_date = sql_date + " " + std::to_string((timestamp % 86400) / 3600) + ":" +
                               std::to_string((timestamp / 60) % 60) + ":" + std::to_string(timestamp % 60);
  CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize);
  pstmt_MktDailyMin_insert->setString(1, shortcode_);
  pstmt_MktDailyMin_insert->setString(2, sql_date);
  //    pstmt_MktDailyMin_insert->setString(3, timestamp);

  pstmt_MktDailyMin_insert->setString(3, timestamp_date);
  pstmt_MktDailyMin_insert->setDouble(4, minPx);
  pstmt_MktDailyMin_insert->setDouble(5, maxPx);
  pstmt_MktDailyMin_insert->setDouble(6, pxRange);
  pstmt_MktDailyMin_insert->setDouble(7, avgPx);
  if (std::isnan(pxStdev))
    pstmt_MktDailyMin_insert->setNull(8, 496);
  else
    pstmt_MktDailyMin_insert->setDouble(8, pxStdev);
  pstmt_MktDailyMin_insert->setInt(9, totalVolume);
  pstmt_MktDailyMin_insert->setInt(10, totalTrades);
  pstmt_MktDailyMin_insert->setInt(11, totalBuyVolume);
  pstmt_MktDailyMin_insert->setInt(12, totalSellVolume);
  pstmt_MktDailyMin_insert->setDouble(13, volumeWeightedAvgPx);
  if (std::isnan(volumeWeightedPxStdev))
    pstmt_MktDailyMin_insert->setNull(14, 496);
  else
    pstmt_MktDailyMin_insert->setDouble(14, volumeWeightedPxStdev);
  pstmt_MktDailyMin_insert->setDouble(15, avgL1Size);
  pstmt_MktDailyMin_insert->setDouble(16, minTrdSz);
  pstmt_MktDailyMin_insert->setDouble(17, maxTrdSz);
  pstmt_MktDailyMin_insert->setDouble(18, avgTrdSz);
  pstmt_MktDailyMin_insert->setDouble(19, minSpread);
  pstmt_MktDailyMin_insert->setDouble(20, maxSpread);
  pstmt_MktDailyMin_insert->setDouble(21, avgSpread);
  try {
    pstmt_MktDailyMin_insert->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::UpdateMktMin:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
}
void DbUpdateNse::UpdateTechnical(std::string shortcode_, double day50SMA, double day100SMA, double day200SMA,
                                  double MACD, double RSI, double MFI, double CCI, double WILLIAM, double AvgVolume10, double product_signal_line) {
  //  CheckandUpdateProduct(shortcode_, exchange_sym, expiry_, strike_, lotSize);
  pstmt_indicator_insert->setString(1, shortcode_);
  pstmt_indicator_insert->setString(2, sql_date);

  if (std::isnan(day50SMA))
     pstmt_indicator_insert->setNull(3, 496);
  else
     pstmt_indicator_insert->setDouble(3, day50SMA);

  if (std::isnan(day100SMA))
    pstmt_indicator_insert->setNull(4, 496);
  else
    pstmt_indicator_insert->setDouble(4, day100SMA);

 if (std::isnan(day200SMA))
   pstmt_indicator_insert->setNull(5, 496);
 else
   pstmt_indicator_insert->setDouble(5, day200SMA);
  
 if (std::isnan(MACD))
   pstmt_indicator_insert->setNull(6, 496);
 else
   pstmt_indicator_insert->setDouble(6, MACD);

 if (std::isnan(RSI))
   pstmt_indicator_insert->setNull(7, 496);
 else
   pstmt_indicator_insert->setDouble(7, RSI);   

 if (std::isnan(MFI))
   pstmt_indicator_insert->setNull(8, 496);
 else
  pstmt_indicator_insert->setDouble(8, MFI);

 if (std::isnan(CCI))
   pstmt_indicator_insert->setNull(9, 496);
 else
   pstmt_indicator_insert->setDouble(9, CCI);

 if (std::isnan(WILLIAM))
   pstmt_indicator_insert->setNull(10, 496);
 else
   pstmt_indicator_insert->setDouble(10, WILLIAM);

 if (std::isnan(AvgVolume10))
   pstmt_indicator_insert->setNull(11, 496);
 else
   pstmt_indicator_insert->setDouble(11, AvgVolume10);

 if (std::isnan(product_signal_line))
    pstmt_indicator_insert->setNull(12,496);
 else
    pstmt_indicator_insert->setDouble(12, product_signal_line); 
 
  try {
    pstmt_indicator_insert->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::UpdateTechnical:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
}
void DbUpdateNse::PrepareMktMinMultipleRows(int timestamp, std::string exchange_sym, double minPx, double maxPx,
                                            double pxRange, double avgPx, double pxStdev, int totalVolume,
                                            int totalTrades, int totalBuyVolume, int totalSellVolume,
                                            double volumeWeightedAvgPx, double volumeWeightedPxStdev, double avgL1Size,
                                            double minTrdSz, double maxTrdSz, double avgTrdSz, double minSpread,
                                            double maxSpread, double avgSpread, std::string shortcode_,
                                            std::string expiry_, int strike_, int lotSize, int noOfMsg) {
  std::string timestamp_date = sql_date + " " + std::to_string((timestamp % 86400) / 3600) + ":" +
                               std::to_string((timestamp / 60) % 60) + ":" + std::to_string(timestamp % 60);
  // CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize); AS database is changed
  if (first_row_mkt == true)
    first_row_mkt = false;
  else
    query_rows_mkt += " , ";
  query_rows_mkt += "('" + shortcode_ + "', '" + sql_date + "', '" + timestamp_date + "', ";
  query_rows_mkt += std::to_string(minPx) + ", " + std::to_string(maxPx) + ", " + std::to_string(pxRange) + ", " +
                    std::to_string(avgPx) + ", ";
  if (std::isnan(pxStdev))
    query_rows_mkt += " null, ";
  else
    query_rows_mkt += std::to_string(pxStdev) + ", ";
  query_rows_mkt += std::to_string(totalVolume) + ", " + std::to_string(totalTrades) + ", " +
                    std::to_string(totalBuyVolume) + ", " + std::to_string(totalSellVolume) + ", ";
  if (std::isnan(volumeWeightedAvgPx))
    query_rows_mkt += " null, ";
  else
    query_rows_mkt += std::to_string(volumeWeightedAvgPx) + ", ";

  if (std::isnan(volumeWeightedPxStdev))
    query_rows_mkt += " null, ";
  else
    query_rows_mkt += std::to_string(volumeWeightedPxStdev) + ", ";
  query_rows_mkt += std::to_string(avgL1Size) + ", " + std::to_string(minTrdSz) + ", " + std::to_string(maxTrdSz) +
                    ", " + std::to_string(avgTrdSz) + ", ";
  query_rows_mkt +=
      std::to_string(minSpread) + ", " + std::to_string(maxSpread) + ", " + std::to_string(avgSpread) + ", " + std::to_string(noOfMsg) + ")";
   // std::cout<< query_rows_mkt <<std::endl;
}

void DbUpdateNse::PrepareCmBhavCopyMultipleRows(std::string exchangeSymbol, std::string symbol, std::string series,
                                                double open, double high, double low, double last, double prevclose,
                                                double close, int tottrdqty, double tottrdval, std::string timestamp,
                                                int totaltrades, std::string isin, std::string shortcode_,
                                                std::string expiry_, int strike_, int lotSize) {
  CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize);
  if (first_row_cm_bhav == true)
    first_row_cm_bhav = false;
  else
    query_rows_cm_bhavcopy += " , ";
  query_rows_cm_bhavcopy += "('" + shortcode_ + "', '" + symbol + "', '" + series + "', ";
  query_rows_cm_bhavcopy += std::to_string(open) + ", " + std::to_string(high) + ", " + std::to_string(low) + ", " +
                            std::to_string(last) + ", ";
  query_rows_cm_bhavcopy += std::to_string(prevclose) + ", " + std::to_string(close) + ", " +
                            std::to_string(tottrdqty) + ", " + std::to_string(tottrdval) + ", ";
  query_rows_cm_bhavcopy += "'" + sql_date + "', " + std::to_string(totaltrades) + ", '" + isin + "')";
  // std::cout<<query_rows_cm_bhavcopy <<std::endl;
}

void DbUpdateNse::PrepareFoBhavCopyMultipleRows(std::string exchange_sym, std::string instrument, std::string symbol,
                                                std::string expiry_dt, double strike_pr, std::string option_typ,
                                                double open, double high, double low, double close, double settle_pr,
                                                double contracts, double val_inlakh, double open_int, double chg_in_oi,
                                                std::string timestamp, std::string shortcode_, std::string expiry_,
                                                int strike_, int lotSize) {
  CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize);
  if (first_row_fo_bhav == true)
    first_row_fo_bhav = false;
  else
    query_rows_fo_bhavcopy += " , ";
  query_rows_fo_bhavcopy += "('" + shortcode_ + "', '" + instrument + "', '" + exchange_sym + "', ";
  query_rows_fo_bhavcopy += "'" + expiry_dt + "', " + std::to_string(strike_pr) + ", '" + option_typ + "', " +
                            std::to_string(open) + ", " + std::to_string(high) + ", ";
  query_rows_fo_bhavcopy += std::to_string(low) + ", " + std::to_string(close) + ", " + std::to_string(settle_pr) +
                            ", " + std::to_string(contracts) + ", ";
  query_rows_fo_bhavcopy += std::to_string(val_inlakh) + ", " + std::to_string(open_int) + ", " +
                            std::to_string(chg_in_oi) + ", '" + sql_date + "') ";
//   std::cout << query_rows_fo_bhavcopy <<std::endl;
}


void DbUpdateNse::PrepareFutOptBhavCopyMultipleRows(std::string exchange_sym, std::string instrument, std::string symbol,
                                                std::string expiry_dt, double strike_pr, std::string option_typ,
                                                double open, double high, double low, double close, double settle_pr,
                                                double contracts, double val_inlakh, double open_int, double chg_in_oi,
                                                std::string timestamp, std::string shortcode_, std::string expiry_,
                                                int strike_, int lotSize) {
  CheckandUpdateProduct(shortcode_, expiry_, strike_, lotSize);
  if (first_row_fo_bhav_opt == true)
    first_row_fo_bhav_opt = false;
  else
  	query_rows_options_bhavcopy  += " , ";
  query_rows_options_bhavcopy += "('" + shortcode_ + "', '" + instrument + "', '" + exchange_sym + "', ";
  query_rows_options_bhavcopy += "'" + expiry_dt + "', " + std::to_string(strike_pr) + ", '" + option_typ + "', " +
                            std::to_string(open) + ", " + std::to_string(high) + ", ";
  query_rows_options_bhavcopy += std::to_string(low) + ", " + std::to_string(close) + ", " + std::to_string(settle_pr) +
                            ", " + std::to_string(contracts) + ", ";
  query_rows_options_bhavcopy += std::to_string(val_inlakh) + ", " + std::to_string(open_int) + ", " +
                            std::to_string(chg_in_oi) + ", '" + sql_date + "') ";
//  std::cout << query_rows_options_bhavcopy <<std::endl; exit(0);
}

void DbUpdateNse::PrepareTechnicalMultipleRows(std::string shortcode_, double day50SMA, double day100SMA,
                                               double day200SMA, double MACD, double RSI, double MFI, double CCI,
                                               double WILLIAM, double AvgVolume10, double product_signal_line) {
  // CheckandUpdateProduct(shortcode_, exchange_sym, expiry_, strike_, lotSize); NO CALL AS BHAV COPY SHOULD HAVED
  // UPDAteD It
  if (first_row_tech == true)
    first_row_tech = false;
  else
    query_rows_technical += " , ";
  query_rows_technical += "('" + shortcode_ + "', '" + sql_date + "', " + std::to_string(day50SMA) + ", " +
                          std::to_string(day100SMA) + ", " + std::to_string(day200SMA) + ", ";
  query_rows_technical += std::to_string(MACD) + ", " + std::to_string(RSI) + ", " + std::to_string(MFI) + ", ";
  query_rows_technical +=
      std::to_string(CCI) + ", " + std::to_string(WILLIAM) + ", " + std::to_string(AvgVolume10) + ", " 
	+ std::to_string(product_signal_line) + ")";
  // std::cout << query_rows_technical <<std::endl;
}

void DbUpdateNse::ExecuteMktMinMultipleRows() {
  try {
    // std::cout<<"QUERY: " << query_rows <<std::endl;
    pstmt_Multiple_row_insert = con->prepareStatement(query_rows_mkt);
    res = pstmt_Multiple_row_insert->executeQuery();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::ExecuteMktMinMultipleRows:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
  if (is_cash == true){
        query_rows_mkt = "insert into MKT_DAILY_MIN_DATA_CASH values";
  } else{
        query_rows_mkt = "insert into MKT_DAILY_MIN_DATA_FUT values";
  }
  first_row_mkt = true;
}

void DbUpdateNse::ExecuteBhavCopyCmMultipleRows() {
  try {
    // std::cout<<"QUERY: " << query_rows <<std::endl;
    pstmt_Multiple_row_insert = con->prepareStatement(query_rows_cm_bhavcopy);
    res = pstmt_Multiple_row_insert->executeQuery();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::ExecuteBhavCopyCmMultipleRows:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
  query_rows_cm_bhavcopy = "insert into BHAV_COPY_DETAILS_CASH values";
  first_row_cm_bhav = true;
}

void DbUpdateNse::ExecuteBhavCopyFoMultipleRows() {
  try {
    // std::cout<<"QUERY: " << query_rows <<std::endl;
    pstmt_Multiple_row_insert = con->prepareStatement(query_rows_fo_bhavcopy);
    res = pstmt_Multiple_row_insert->executeQuery();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::ExecuteBhavCopyfoMultipleRows:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
  query_rows_fo_bhavcopy = "insert into BHAV_COPY_DETAILS_FO values";
  first_row_fo_bhav = true;
}


void DbUpdateNse::ExecuteBhavCopyFutOptMultipleRows() {
  try {
    // std::cout<<"QUERY: " << query_rows <<std::endl;
    pstmt_Multiple_row_insert = con->prepareStatement(query_rows_options_bhavcopy);
    res = pstmt_Multiple_row_insert->executeQuery();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::ExecuteBhavCopyfoMultipleRows:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
  query_rows_fo_bhavcopy = "insert into BHAV_COPY_DETAILS_FO values";
  first_row_fo_bhav_opt = true;
}

void DbUpdateNse::ExecuteTechnicalMultipleRows() {
  try {
    // std::cout<<"QUERY: " << query_rows <<std::endl;
    pstmt_Multiple_row_insert = con->prepareStatement(query_rows_technical);
    res = pstmt_Multiple_row_insert->executeQuery();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::ExecuteTechnicalMultipleRows:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
  query_rows_technical = "insert into INDICATOR_DAILY_DATA values";
  first_row_tech = true;
}

void DbUpdateNse::GetProductKey() {
  res = stmt->executeQuery("select * from PRODUCT;");
  //   std::cout << "KEY EXISTING ";
  while (res->next()) {
    product_added.insert(res->getString("shortcode"));
  }
  std::cout << std::endl;
}

void DbUpdateNse::CheckandUpdateProduct(std::string shortcode_, std::string expiry_,
                                        int strike_, int lotSize) {
  if (product_added.find(shortcode_) == product_added.end()) {
    UpdateProduct(shortcode_, expiry_, strike_, lotSize);
  }
}

void DbUpdateNse::UpdateProduct(std::string shortcode_, std::string expiry_, int strike_,
                                int lotSize) {
  std::cout << "Pushing New Product " << shortcode_ << std::endl;
  pstmt_product_insert->setString(1, shortcode_);
  pstmt_product_insert->setString(2, expiry_);  // expriy date
  if (strike_ != 0)
    pstmt_product_insert->setInt(3, strike_);
  else
    pstmt_product_insert->setNull(3, 496);  // add NULL
  pstmt_product_insert->setInt(4, lotSize);
  try {
    pstmt_product_insert->executeUpdate();
  } catch (sql::SQLException &e) {
    std::cout << "DbUpdateNse::UpdateProduct:: ERR: SQLException in " << __FILE__;
    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
    std::cout << "# ERR: " << e.what();
    std::cout << " (MySQL error code: " << e.getErrorCode();
    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    exit(-1);
  }
  product_added.insert(shortcode_);
}

}  // namespace HFSAT
