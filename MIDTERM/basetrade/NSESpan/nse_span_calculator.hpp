// =====================================================================================
//
//       Filename:  nse_span_calculator.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/04/2015 02:16:45 PM
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

#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>

#include "basetrade/rapidxml/rapidxml_utils.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {
#define EXPOSURE_FILE_DIR "/spare/local/tradeinfo/NSE_Files/Margin_Files/Exposure_Files"
#define SPAN_FILE_DIR "/spare/local/tradeinfo/NSE_Files/Margin_Files/Span_Files"
#define SPAN_SCENARIOS 16

class NSESpanCalculator {
  /**
   * Keeps track of Span risk,
   * Currently for optimization it parses and stores data for securities which are added to
   *  sec name indexer
   */

 private:
  SecurityNameIndexer *sec_name_indexer_;  // doesn't work if secame is > 16
  int tradindate_;
  std::string date_string_;
  std::string current_filename_;
  std::vector<std::string> IDX_stocks = { "NIFTY" , "BANKNIFTY"};
  std::map<std::string, std::vector<double> > sec_id_to_risk_scenario_vec_;
  std::map<std::string, std::vector<double> > sec_id_to_risk_vec_per_pos_;
  std::map<std::string, double> sec_id_to_option_minimum_;
  std::map<std::string, double> underlying_to_futpx;
  std::map<std::string, double> underlying_to_exposure_margin_rate;
  /// Map from underlying to spread to rate
  /// Can optimize to vector of map if we can give to index underlying
  std::map<std::string, std::map<std::string, double> > base_to_spread_to_val_;

  bool print_dbg_ = false;

  void GetLatestFile();
  void ParseFile(std::string filename);
  void ExtractStockData(rapidxml::xml_node<char> *lphy);

  void ExtractFuturesData(rapidxml::xml_node<char> *futPf);
  void ExtractFuturesDataPerExpiry(rapidxml::xml_node<char> *fut, const std::string &instrument, int to_record_px);

  void ExtractOptionsData(rapidxml::xml_node<char> *oopPf);
  void ExtractOptionsDataPerSeries(rapidxml::xml_node<char> *series, const std::string &instruemnt);
  void ExtractOptionsDataPerPrice(rapidxml::xml_node<char> *opt, const std::string &instrument,
                                  const std::string &expriy);

  void ExtractSpreadData(rapidxml::xml_node<char> *spread);
  void AddRiskScenario(std::string &shortcode, const std::vector<double> &scenario_param);
  bool isIndex(std::string instrument);
  std::string GetInstrumentFromShortcode(const std::string &shortcode);
  bool IsOption(const std::string &name);
  double GetShortOptionMinimum(const std::string &secname);
  double GetCalendarSpreadCharge(const std::vector<std::string> &shortcode_vec, const std::vector<int> &position);

 public:
  NSESpanCalculator(int tradindate, SecurityNameIndexer *sec_name_indexer);
  ~NSESpanCalculator(){};
  void ReloadExposureData();
  void ReloadSpanData();

  /// Better if we pass vector of sec_id here for speed purpose, which should be possible in code
  double GetSpanRisk(const std::vector<std::string> &shortcode, const std::vector<int> &position);
  double GetExposureMargin(const std::vector<std::string> &shortcode, const std::vector<int> &position, const std::string &instrument);
};
}
