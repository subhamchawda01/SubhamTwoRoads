// =====================================================================================
//
//       Filename:  db_create_handle_2.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/05/2021 10:52:45 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

/* Standard C++ includes */
#include <stdlib.h>
#include <iostream>
#include <chrono>
/*
  Include directly the different
  headers from cppconn/ and mysql_driver.h + mysql_util.h
  (and mysql_connection.h). This will reduce your build time!
*/

#include <mysql_connection.h>
#include "dvccode/Utils/get_db_config.hpp"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using namespace std;

HFSAT::GetDBConfig &db_config = HFSAT::GetDBConfig::GetUniqueInstance();

int main(void) {
  cout << endl;
  cout << "Let's have MySQL count from 10 to 1..." << endl;

  try {
    sql::Driver *driver;
    sql::Connection *con;
    sql::Statement *stmt;
    sql::ResultSet *res;
    sql::PreparedStatement *pstmt;

    /* Create a connection */
    driver = get_driver_instance();
    std::string db_ip = db_config.GetDbIp();
    std::string db_user = db_config.GetDbUser();
    std::string db_pass = db_config.GetDbPassword();
    con = driver->connect("tcp://" + db_ip, db_user, db_pass);
    /* Connect to the MySQL test database */
    con->setSchema("testing_r");

    stmt = con->createStatement();
    //  stmt->execute("DROP TABLE IF EXISTS test");
    //  stmt->execute("CREATE TABLE test(id INT)");
    delete stmt;
 auto start1 = std::chrono::high_resolution_clock::now();
    string query = "insert into Product values ";
	bool flag = false;
    for (int i = 1000; i < 2000; i++) {
		if (flag == false) flag = true;
		else query += ",";
		query += "('NSE42412" + to_string(i) +"', '2012-01-12', 'NSE_HDFCY', -1,-1) ";
	}
   std::cout<< "query: " << query<<std::endl; 
    pstmt = con->prepareStatement(query); 
    res = pstmt->executeQuery();
 auto stop1 = std::chrono::high_resolution_clock::now();
auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);

    cout << "TIME1: " << duration1.count() << endl;
   
    /* '?' is the supported placeholder syntax */
    pstmt = con->prepareStatement(
        "INSERT INTO Product(exchangeSymbol,expiry,product,strikePrice,lotSize) VALUES (?,?,?,?,?)");
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
      pstmt->setString(1, "NSE42412" + to_string(i));
      pstmt->setString(2, "2012-01-12");
      pstmt->setString(3, "NSE_HDFCY");
      pstmt->setNull(4, 496);
      pstmt->setNull(5, 496);
      pstmt->executeUpdate();
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  
    cout << "TIME2: " << duration.count() << endl;
    

    delete pstmt;
    return 0;
    /* Select in ascending order */
    pstmt = con->prepareStatement("SELECT id FROM test ORDER BY id ASC");
    res = pstmt->executeQuery();

    /* Fetch in reverse = descending order! */
    res->afterLast();
    while (res->previous()) cout << "\t... MySQL counts: " << res->getInt("id") << endl;
    delete res;

    delete pstmt;
    delete con;

  } catch (sql::SQLException &e) {
    cout << "# ERR: SQLException in " << __FILE__;
    cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
    cout << "# ERR: " << e.what();
    cout << " (MySQL error code: " << e.getErrorCode();
    cout << ", SQLState: " << e.getSQLState() << " )" << endl;
  }

  cout << endl;

  return EXIT_SUCCESS;
}
