	// =====================================================================================
//
//       Filename:  span_calculator.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/04/2015 02:17:14 PM
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

#include "dvccode/SpanMargin/span_calculator.hpp"

#include <vector>
#include <algorithm>

#include "dvccode/rapidxml/rapidxml.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {

/*
 *
 * @param tradindate
 */
SpanCalculator::SpanCalculator(int tradindate, SecurityNameIndexer& sec_name_indexer, std::string exch)
    : exchange(exch),
      tradindate_(tradindate),
      sec_name_indexer_(sec_name_indexer),
      risk_array_sec_id(sec_name_indexer_.NumSecurityId()),
      pos_risk(6),
      underlying_wise_short_positions(6, 0),
      underlying_wise_spotpx(6),
      sec_id_to_stpx(sec_name_indexer_.NumSecurityId()),
      position_map(sec_name_indexer_.NumSecurityId(), 0),
      sec_id_to_underlying(sec_name_indexer_.NumSecurityId()),
      secid_to_optpx(sec_name_indexer_.NumSecurityId()),
      secid_to_optelm(sec_name_indexer_.NumSecurityId())

    {
  std::stringstream st;
  st << tradindate_;
  date_string_ = st.str();

  if ("NSE" == exchange) {
    ParseSpanFile(std::string(NSE_SPAN_FILE_DIR) + "/nsccl." + date_string_ + ".s_1.spn");
    LoadRiskArrays();
  }
  else if ("BSE" == exchange) {
    ParseSpanFile(std::string(BSE_SPAN_FILE_DIR) + "/BSERISK" + std::to_string(tradindate_) + "-FINAL.spn");
    LoadRiskArrays();
  }
  else if("BSE_NSE" == exchange) {
    exchange = "BSE";
    ParseSpanFile(std::string(BSE_SPAN_FILE_DIR) + "/BSERISK" + std::to_string(tradindate_) + "-FINAL.spn");
    LoadRiskArrays();

    exchange = "NSE";
    ParseSpanFile(std::string(NSE_SPAN_FILE_DIR) + "/nsccl." + date_string_ + ".s_1.spn");
    LoadRiskArrays();
  }else if ("CBOE" == exchange) {
    return;
  }
  else {
    exchange = "NSE";
    std::cout << "Wrong exchange...\n";
    std::cout << "Loading default as NSE\n";
    ParseSpanFile(std::string(NSE_SPAN_FILE_DIR) + "/nsccl." + date_string_ + ".s_1.spn");
    LoadRiskArrays();
  }
  LoadExposureData();
  CreateSecIdUnderlyingVec();
}

/**
 *
 */
void SpanCalculator::LoadRiskArrays() {
  std::string line;
  std::ifstream inputFile;

  if ("NSE" == exchange) {
    inputFile = std::ifstream(std::string(NSE_RISK_ARRAY_DIR) + std::to_string(tradindate_));

    while (std::getline(inputFile, line)) {
      std::istringstream iss(line);
      std::string key;
      iss >> key;
      std::array<double, 16> values;
      for (double& val : values) {
          if (!(iss >> val)) {
              std::cerr << "Error reading values for key: " << key << std::endl;
          }
      }

      int sec_id = sec_name_indexer_.GetIdFromSecname(key.c_str());

      if (-1 != sec_id) {
        risk_array_sec_id[sec_id] = values;
      }
    }
  }
  else if ("BSE" == exchange) {
    inputFile = std::ifstream(std::string(BSE_RISK_ARRAY_DIR) + std::to_string(tradindate_));

    while (std::getline(inputFile, line)) {
      std::istringstream iss(line);
      std::string key;
      iss >> key;
      std::array<double, 16> values;
      for (double& val : values) {
          if (!(iss >> val)) {
              std::cerr << "Error reading values for key: " << key << std::endl;
          }
      }

      int sec_id = sec_name_indexer_.GetIdFromSecname(key.c_str());

      if (-1 != sec_id) {
        risk_array_sec_id[sec_id] = values;
      }
    }
  }

  else {
    std::cout << "Wrong exchange...\n";
    std::cout << "Loading default as NSE\n";
    inputFile = std::ifstream(std::string(NSE_RISK_ARRAY_DIR) + std::to_string(tradindate_));
  }

  inputFile.close();
}

/**
 *
 */
void SpanCalculator::LoadExposureData() {  
    std::string filename = std::string(NSE_EXPOSURE_FILE_DIR) + "/exposure_margin_rates." + date_string_;
     if (FileUtils::ExistsAndReadable(filename)) {
    std::ifstream filereader(filename.c_str(), std::ifstream::in);

    if (filereader.is_open()) {
      while (filereader.good()) {

        int readlen = 1024;
        char buffer[readlen];
        filereader.getline(buffer, readlen);
	if (buffer[0]=='\0')
          break;
        HFSAT::PerishableStringTokenizer st(buffer, readlen);
        const std::vector<const char*>& tokens = st.GetTokens();

        if (tokens.size() > 1) {
          if (tokens[0][0] == '#') {
            continue;
          }
	  //std::cout<<tokens[0]<<"\t"<<tokens[1]<<"\t"<<tokens[2]<<"\n";
          std::string underlying = std::string(tokens[1]);
	  float exposure_margin_rate = atof(tokens[2]);
	  underlying_to_exposure_margin_rate[underlying]=exposure_margin_rate/100;
	}
      }
    }
    }
}

/**
 *
 * @param filename
 */
void SpanCalculator::ParseSpanFile(std::string filename) {

  if ( !FileUtils::ExistsAndReadable( filename ) ) {
    std::cerr << "Span File does not exit, file: "
              << filename
              << std::endl;
    exit(1);
    // return;
  }
  rapidxml::file<char> xml_file = rapidxml::file<char>(filename.c_str());
  rapidxml::xml_document<> doc_;   // character type defaults to char
  doc_.parse<0>(xml_file.data());  // 0 means default parse flags

  if (!xml_file.data()) {
    std::cout << "Invalid xml file\n";
  }
  rapidxml::xml_node<char>* first_node = doc_.first_node();
  rapidxml::xml_node<char>* l1 = first_node->first_node("pointInTime");
  rapidxml::xml_node<char>* l2 = l1->first_node("clearingOrg");

  for (rapidxml::xml_node<char>* l3 = l2->first_node(); l3; l3 = l3->next_sibling()) {
    for (rapidxml::xml_node<char>* phypf = l3->first_node("phyPf"); phypf; phypf = phypf->next_sibling("phyPf")) {
      if (phypf != NULL) {
        ExtractStockData(phypf);
      }
    }

    for (rapidxml::xml_node<char>* futPf = l3->first_node("futPf"); futPf; futPf = futPf->next_sibling("futPf")) {
      if (futPf) {
    	//std::cout << "Extracting FUT data" << std::endl;
    	ExtractFuturesData(futPf);
      }
    }

    for (rapidxml::xml_node<char>* oopPf = l3->first_node("oopPf"); oopPf; oopPf = oopPf->next_sibling("oopPf")) {
      if (oopPf) {
        ExtractOptionsData(oopPf);
      }
    }

    for (rapidxml::xml_node<char>* spread = l3->first_node("dSpread"); spread;
         spread = spread->next_sibling("dSpread")) {
      if (spread) {
        ExtractSpreadData(spread);
      }
    }
  }
}

/**
 *
 * @param phyPf
 */
void SpanCalculator::ExtractStockData(rapidxml::xml_node<char>* phyPf) {
  rapidxml::xml_node<char>* name = phyPf->first_node("name");
  if (print_dbg_) std::cout << "ExtractStockData: " << phyPf->name() << phyPf->value() << std::endl;
  std::string instr = "";
  if (name) {
    instr = std::string(name->value());
  }

  if (instr == "BSE 30 SENSEX")
    instr = "BSX";
  if (instr == "BSE BANKEX")
    instr = "BKX";


  rapidxml::xml_node<char>* lphy = phyPf->first_node("phy");
  if (!lphy) {
    return;
  }
  double px = 0;
  px = atof(lphy->first_node("p")->value());

  rapidxml::xml_node<char>* scan_rate = lphy->first_node("scanRate");
  double px_scan = 0;
  double vol_scan = 0;
  if (scan_rate) {
    px_scan = atof(scan_rate->first_node("priceScan")->value());
    vol_scan = atof(scan_rate->first_node("volScan")->value());
  }
  std::vector<double> risk_scenario;
  rapidxml::xml_node<char>* rs = lphy->first_node("ra");
  std::string internal_name = exchange + "_" + instr;
  if (rs) {
    for (rapidxml::xml_node<char>* sc = rs->first_node("a"); sc; sc = sc->next_sibling("a")) {
      risk_scenario.push_back(atof(sc->value()));
      risk_array[internal_name].push_back(atof(sc->value()));
    }
    // Getting delta tag <d> in <ra>
    rapidxml::xml_node<char>* detla = rs->first_node("d");
    delta_map[internal_name] = atof(detla->value());

    if (print_dbg_) {
      std::cout << "STOCK: " << instr << " " << px << " " << px_scan << " " << vol_scan << " \n";
      for (unsigned i = 0; i < risk_scenario.size(); i++) {
        std::cout << risk_scenario[i] << " ";
      }
      std::cout << std::endl;
    }

    // For SPOT of Indices.
    if (internal_name == "NSE_NIFTY")
      underlying_wise_spotpx[0] = px;  //NIFTY
    else if (internal_name == "NSE_BANKNIFTY")
      underlying_wise_spotpx[1] = px;  //BANKNIFTY
    else if (internal_name == "NSE_FINNIFTY")
      underlying_wise_spotpx[2] = px;  //FINNIFTY
    else if (internal_name == "NSE_MIDCPNIFTY")
      underlying_wise_spotpx[3] = px;  //MIDCPNIFTY
    else if (internal_name == "BSE_BSX")
      underlying_wise_spotpx[4] = px;  //SENSEX
    else if (internal_name == "BSE_BKX")
      underlying_wise_spotpx[5] = px;  //BANKEX

    AddRiskScenario(internal_name, risk_scenario);
  }
}

/**
 *
 * @param futPf
 */
void SpanCalculator::ExtractFuturesData(rapidxml::xml_node<char>* futPf) {
  rapidxml::xml_node<char>* name = futPf->first_node("name");
  if (print_dbg_) std::cout << "FUT: " << futPf->name() << futPf->value() << std::endl;
  std::string instr = "";
  if (name) {
    instr = std::string(name->value());
  }
  int record_fut_px=1;
  for (rapidxml::xml_node<char>* fut = futPf->first_node("fut"); fut; fut = fut->next_sibling("fut")) {
    ExtractFuturesDataPerExpiry(fut, instr , record_fut_px);
    record_fut_px=0;
  }
  
}

/**
 *
 * @param fut
 */
void SpanCalculator::ExtractFuturesDataPerExpiry(rapidxml::xml_node<char>* fut, const std::string& instrument, int to_record_px) {
  double px = 0;
  //std::cout<<instrument;
  px = atof(fut->first_node("p")->value());
  if(to_record_px)
  	underlying_to_futpx[instrument] = px;
  rapidxml::xml_node<char>* scan_rate = fut->first_node("scanRate");

  double px_scan = 0;
  double vol_scan = 0;
  if (scan_rate) {
    px_scan = atof(scan_rate->first_node("priceScan")->value());
    vol_scan = atof(scan_rate->first_node("volScan")->value());
  }
  std::string expiry_date = "";
  rapidxml::xml_node<char>* expiry_node = fut->first_node("pe");
  if (expiry_node) {
    expiry_date = std::string(expiry_node->value());
  }
  //std::cout<<px<<'\t';
  //std::cout<<expiry_date<<'\n';
  std::vector<double> risk_scenario;
  rapidxml::xml_node<char>* rs = fut->first_node("ra");
  std::string internal_name = exchange + "_" + instrument + "_FUT_" + expiry_date;
  if (rs) {
    for (rapidxml::xml_node<char>* sc = rs->first_node("a"); sc; sc = sc->next_sibling("a")) {
      risk_scenario.push_back(atof(sc->value()));
      risk_array[internal_name].push_back(atof(sc->value()));
    }
    // Getting delta tag <d> in <ra>
    rapidxml::xml_node<char>* detla = rs->first_node("d");
    delta_map[internal_name] = atof(detla->value());
    
    if (print_dbg_) {
      std::cout << "FUT: " << px << " " << px_scan << " " << vol_scan << " \n";
      for (unsigned i = 0; i < risk_scenario.size(); i++) {
        std::cout << risk_scenario[i] << " ";
      }
      std::cout << std::endl;
    }
    AddRiskScenario(internal_name, risk_scenario);
  }
}

/**
 *
 * @param oopPf
 */
void SpanCalculator::ExtractOptionsData(rapidxml::xml_node<char>* oopPf) {
  rapidxml::xml_node<char>* name = oopPf->first_node("name");
  if (print_dbg_) std::cout << "OPT: " << oopPf->name() << oopPf->value() << std::endl;
  std::string instr = "";
  if (name) {
    instr = std::string(name->value());
  }

  for (rapidxml::xml_node<char>* series = oopPf->first_node("series"); series;
       series = series->next_sibling("series")) {
    ExtractOptionsDataPerSeries(series, instr);
  }
}

void SpanCalculator::ExtractOptionsDataPerSeries(rapidxml::xml_node<char>* series, const std::string& instrument) {
  std::string expiry_date = "";
  rapidxml::xml_node<char>* expiry_node = series->first_node("pe");
  if (expiry_node) {
    expiry_date = std::string(expiry_node->value());
  }

  rapidxml::xml_node<char>* scan_rate = series->first_node("scanRate");

  double px_scan = 0;
  double vol_scan = 0;
  if (scan_rate) {
    px_scan = atof(scan_rate->first_node("priceScan")->value());
    vol_scan = atof(scan_rate->first_node("volScan")->value());
  }
  if (print_dbg_) std::cout << "OPTION-SERIES: " << px_scan << " " << vol_scan << " \n";
  for (rapidxml::xml_node<char>* opt = series->first_node("opt"); opt; opt = opt->next_sibling("opt")) {
    ExtractOptionsDataPerPrice(opt, instrument, expiry_date);
  }
}

/**
 *
 * @param opt
 */
void SpanCalculator::ExtractOptionsDataPerPrice(rapidxml::xml_node<char>* opt, const std::string& instrument,
                                                   const std::string& expiry_date) {
  double px = 0;
  px = atof(opt->first_node("p")->value());
  
  char put_or_call = ' ';
  rapidxml::xml_node<char>* poc = opt->first_node("o");
  if (poc) {
    put_or_call = poc->value()[0];
  }

  std::string price = "";
  rapidxml::xml_node<char>* opt_price = opt->first_node("k");
  if (opt_price) {
    price = opt_price->value();
  }

  double option_minimum = 0.0;
  rapidxml::xml_node<char>* opt_min = opt->first_node("p");
  if (opt_min) {
    //option_minimum = atof(opt_min->value());
   if(isIndex(instrument)){
    	option_minimum = 0.03*underlying_to_futpx[instrument];
   }
   else
	option_minimum = 0.075*underlying_to_futpx[instrument];
  }

  std::string instr = "";

  if (instrument == "BSE 30 SENSEX OPTIONS") {
    instr = "BSX";
  } else if (instrument == "BSE BANKEX OPTIONS") {
    instr = "BKX";
  } else {
    instr = instrument;
  }

  std::string sc = exchange + "_" + instr + "_" + put_or_call + "E_" + price + "_" + expiry_date;

  std::string exch_sym = HFSAT::SecurityDefinitions::ConvertDataSourceNametoExchSymbol(sc);
  std::string scc = HFSAT::SecurityDefinitions::GetWeeklyShortCodeFromSymbol(exch_sym);
  int sec_id = sec_name_indexer_.GetIdFromSecname(exch_sym.c_str());

  if (-1 != sec_id) {
    // For option price
    sec_id_to_stpx[sec_id] = std::stoi(price);
    secid_to_optpx[sec_id] = px;

    // For option elm rate
    int expiry = HFSAT::SecurityDefinitions::GetExpiryFromShortCode(scc);
    if (tradindate_ == expiry)
      secid_to_optelm[sec_id] = 0.04;
    else
      secid_to_optelm[sec_id] = 0.02;
  }

  std::vector<double> risk_scenario;
  rapidxml::xml_node<char>* rs = opt->first_node("ra");
  std::stringstream st;
  st << exchange << "_" << instr << "_" << put_or_call << "E_" << price << "_" << expiry_date;

  std::string internal_name = st.str();
  if (rs) {
    for (rapidxml::xml_node<char>* sc = rs->first_node("a"); sc; sc = sc->next_sibling("a")) {
      risk_scenario.push_back(atof(sc->value()));
      risk_array[internal_name].push_back(atof(sc->value()));
    }
    
    // Getting delta tag <d> in <ra>
    rapidxml::xml_node<char>* detla = rs->first_node("d");
    delta_map[internal_name] = atof(detla->value());
    
    if (print_dbg_) {
      std::cout << "OPTION: " << px << " \n";
      for (unsigned i = 0; i < risk_scenario.size(); i++) {
        std::cout << risk_scenario[i] << " ";
      }
      std::cout << std::endl;
    }
    AddRiskScenario(internal_name, risk_scenario);
    sec_id_to_option_minimum_[internal_name] = option_minimum;
  }
}

void SpanCalculator::ExtractSpreadData(rapidxml::xml_node<char>* spread) {
  double rate = 0.0;
  rapidxml::xml_node<char>* rate_node = spread->first_node("rate");
  rapidxml::xml_node<char>* val_node = rate_node->first_node("val");
  if (val_node) {
    rate = atof(val_node->value());
  }
  std::string underlying = "";
  std::stringstream st;
  std::string spread_name = "";

  for (rapidxml::xml_node<char>* leg = spread->first_node("pLeg"); leg; leg = leg->next_sibling("pLeg")) {
    rapidxml::xml_node<char>* name = leg->first_node("cc");
    std::string this_underlying = std::string(name->value());

    /// not same underlying, skip
    if (!underlying.empty() && this_underlying.compare(underlying) != 0) {
      return;
    }
    underlying = this_underlying;
    rapidxml::xml_node<char>* expiry = leg->first_node("pe");
    // rapidxml::xml_node<char>*bs = leg->first_node("rs");
    // rapidxml::xml_node<char>*wt = leg->first_node("i");
    if (expiry) {  // Only for calendar spread
      st << expiry->value() << "_";
    }
  }
  spread_name = st.str();
  if (print_dbg_) std::cout << " ADDING: " << underlying << " spd: " << spread_name << " rt: " << rate << "\n";
  base_to_spread_to_val_[underlying][spread_name] = rate;
}

/**
 *
 * @param shortcode
 * @param scenario_param
 */
void SpanCalculator::AddRiskScenario(std::string& shortcode, const std::vector<double>& scenario_param) {
  if (print_dbg_) std::cerr << " ADDING for: " << shortcode << std::endl;
  sec_id_to_risk_scenario_vec_[shortcode] = scenario_param;
  sec_id_to_risk_vec_per_pos_[shortcode] = scenario_param;
}

bool SpanCalculator::isIndex(std::string instrument){
	 for(uint64_t i=0; i<IDX_stocks.size();i++){
		if(IDX_stocks[i]==instrument)
			return true;
	}
	return false;
}
/**
 *
 * @param shortcode
 * @param position
 * @return
 */
double SpanCalculator::GetSpanRisk(const std::map<std::string, int> positions){
  // Better pass the vector of sec_id only
  std::vector<double> span_risk_vec(SPAN_SCENARIOS, 0.0);

  double option_min = 0.0;
  double max_risk = 0.0;

  std::map<std::string, int>::const_iterator it = positions.begin();

  for(it = positions.begin(); it != positions.end(); it++) {
    std::string shortcode = it->first;
    int position = it->second;
  
    // if(IsOption(shortcode) && position >0)
    //     continue;

    if (IsOption(shortcode)) {
      option_min += GetShortOptionMinimum(shortcode) * position * -1;
    }

    auto scenario_vec_iter = sec_id_to_risk_scenario_vec_.find(shortcode);
 
    if (scenario_vec_iter == sec_id_to_risk_scenario_vec_.end()) {
      return 0;
    }

    auto& param_vec = sec_id_to_risk_scenario_vec_[shortcode];
    max_risk = span_risk_vec[0];
    
    for (auto j = 0u; j < param_vec.size(); j++) {
      // Assuming values given in param  are risk-per uts
      span_risk_vec[j] += (position * param_vec[j]);
      // Calculate max here
      if (span_risk_vec[j] > max_risk) {
        max_risk = span_risk_vec[j];
      }
    }
  }
  return std::max(max_risk, option_min);
}

std::string SpanCalculator::GetInstrumentFromShortcode(const std::string& shortcode){
	int first_underscore  = shortcode.find('_');
	int second_underscore = shortcode.substr(first_underscore+1).find('_');
	std::string instrument = shortcode.substr(first_underscore+1 , second_underscore);
	//std::cout<<instrument<<std::endl;
	return instrument;
}


bool SpanCalculator::IsOption(const std::string& name) {
// Can have better way ?
  if (name.find("_PE_") != std::string::npos || name.find("_CE_") != std::string::npos) {
    return true;
  }
  return false;
}

double SpanCalculator::GetShortOptionMinimum(const std::string& secname) {
  // Is the field extracted correct
  auto iter = sec_id_to_option_minimum_.find(secname);
  if (iter != sec_id_to_option_minimum_.end()) {
    return iter->second;
  }
  return 0.0;
}

/**
 *
 * @param shortcode
 * @param position
 * @return
 */
double SpanCalculator::GetCalendarSpreadCharge(const std::vector<std::string>& shortcode_vec,
                                                  const std::vector<int>& position) {
  /// Currently it's very very inefficient, We need to input such way that filtering spreads is easier
  
  double charge = 0.0;
  std::map<std::string, std::vector<std::pair<std::string, int> > > underlying_to_expiry_vec;  
  //std::cout<<"Utoexpmap";
  for (unsigned i = 0; i < shortcode_vec.size(); i++) {
    std::cout << i << "----\n";
    /// "NSE_ZEEL_FUT_20150827_20150802"
    std::cout<<'\n'<<shortcode_vec[i]<<'\n';
    std::string without_exchange = shortcode_vec[i].substr(4, shortcode_vec[i].length() - 1);
    std::string underlying = without_exchange.substr(0, without_exchange.find("_"));
    auto base_to_spread_iter = base_to_spread_to_val_.find(underlying);
    std::cout << "Spread-> " << underlying << "\n";
    if (base_to_spread_iter == base_to_spread_to_val_.end()) {
      std::cout << "ending...\n";
      continue;
    } else {
      std::string rem_str = without_exchange.substr(without_exchange.find("_") + 1, without_exchange.length() - 1);
      rem_str = rem_str.substr(rem_str.find("_") + 1, rem_str.length() - 1);
       std::cout << " expiry_string: " << rem_str<< " ul: " << underlying << std::endl;
      auto underlying_to_expiry_vec_iter = underlying_to_expiry_vec.find(underlying);
      if (underlying_to_expiry_vec_iter == underlying_to_expiry_vec.end()) {
         std::cout << "adding new: " << rem_str << " pos: " <<position[i] << std::endl;
         std::cout << "pushing into underlying_to_expiry_vec " << underlying << "\n";
        underlying_to_expiry_vec[underlying].push_back({std::make_pair(rem_str, position[i])});
      } else {
	      std::cout<<(underlying_to_expiry_vec_iter->second).size()<<"----------------\n";
        for (unsigned j = 0; j < (underlying_to_expiry_vec_iter->second).size(); j++) {
          std::string existing_expiry = ((underlying_to_expiry_vec_iter->second)[j]).first;
          int value = ((underlying_to_expiry_vec_iter->second)[j]).second;
          std::string new_string = existing_expiry + "_" + rem_str + "_";
          //std::cout << " common:string " << new_string << std::endl;
          auto expiry_to_val_iter = (base_to_spread_iter->second).find(new_string);

          if (expiry_to_val_iter == (base_to_spread_iter->second).end()) {
            (underlying_to_expiry_vec_iter->second).push_back(std::make_pair(rem_str, position[i]));
          } else {
            if (position[i] == -value) {
              charge += expiry_to_val_iter->second * value;
              (underlying_to_expiry_vec_iter->second).erase((underlying_to_expiry_vec_iter->second).begin() + j);
            } else if (std::abs(position[i]) > std::abs(value) && position[i] * value < 0) {
              charge += expiry_to_val_iter->second * value;
              (underlying_to_expiry_vec_iter->second).erase((underlying_to_expiry_vec_iter->second).begin() + j);
              (underlying_to_expiry_vec_iter->second)
                  .push_back(std::make_pair(rem_str, position[i] - (position[i] * value) / std::abs(position[i])));
            } else if (std::abs(position[i]) < std::abs(value) && position[i] * value < 0) {
              charge += expiry_to_val_iter->second * position[i];
              ((underlying_to_expiry_vec_iter->second)[j]).second -= (position[i] * value) / std::abs(value);
            }
          }
        }
      }
    }
  }
  // std::cout << " CS Charge: " << charge << std::endl;
  return charge;
}

std::vector<double> SpanCalculator::GetRiskArray(const std::string internal_name) {
  return risk_array[internal_name];
}

std::unordered_map<std::string, std::vector<double>> SpanCalculator::GetRiskMap() {
  return risk_array;
}

double SpanCalculator::GetDelta(const std::string internal_name) {
  return delta_map[internal_name];
}


void SpanCalculator::GetPositionsVec() {
  int non_zero = 0;
  for(unsigned int i = 0; i < position_map.size(); i++) {
    if (0 != position_map[i]) {
      non_zero++;
      std::cout << i << " "
                << sec_name_indexer_.GetShortcodeFromId(i) << " "
                << position_map[i]
                << "\n";
    } else
      continue;
  }

  std::cout << "Number of securities: "
            << position_map.size()
            << "\nNumber of non zero securities: "
            << non_zero << "\n";
}

void SpanCalculator::GetShortPositionsVec() {
  for(unsigned int i = 0; i < underlying_wise_short_positions.size(); i++) {
    std::cout << i << " "
              << underlying_wise_short_positions[i]
              << "\n";
  }
}

/*
 * Returns total margin.
 * total = (net_premium + exposure_margin + upfront_margin)
 */
double SpanCalculator::GetMargin()  {
  return (upfront_margin - exposure_margin - net_premium);
}

std::stringstream SpanCalculator::GetMarginD()  {
  std::stringstream stringStream;
  stringStream << "SPOT VALUES\n";
  stringStream << underlying_wise_spotpx[0] << "\n"
               << underlying_wise_spotpx[1] << "\n"
               << underlying_wise_spotpx[2] << "\n"
               << underlying_wise_spotpx[3] << "\n";

  for (auto arr : pos_risk) {
    for (auto i : arr) {
      stringStream << i << " ";
    }
    stringStream << "\n";
  }
  stringStream << "opt wise risk array\n";

  for (unsigned int ii = 0; ii < position_map.size(); ii++) {
    if (position_map[ii] == 0)
      continue;

    int sec_id = ii;
    int pos = position_map[sec_id];
    std::array<double, 16> param_vec = risk_array_sec_id[sec_id];

    stringStream << sec_id << "\t" << sec_name_indexer_.GetShortcodeFromId(sec_id) << "\t" << pos << " ";
    for (auto r : param_vec)
      stringStream << r*pos << " ";
    stringStream << "\n";
  }

  stringStream << "total: " << (upfront_margin - exposure_margin - net_premium)/10000000
               << "\n" << "upfront: " << upfront_margin
               << "\n" << "exposure: "
               << exposure_margin << "\n"
               << "premium: "
               << net_premium << "\n"
               << "===========================";

  return stringStream;
  // return (upfront_margin - exposure_margin + net_premium);
}

/*
 * Calculates all the components of the margin.
 * Updates class variables: net_premium, exposure_margin, upfront_margin.
 *
 * @param shortcode
 * @param position
 */
void SpanCalculator::AddPosition(const int sec_id, const int position, const double trade_price)  {
  // HFSAT::CpucycleProfiler::GetUniqueInstance().Start(1);
  GetUpfrontMargin(sec_id, position);
  GetExposureMargin(sec_id, position);
  GetNetPremium(sec_id, position, trade_price);

  position_map[sec_id] += position;
  // HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
}

/*
 * If a portfolio has only long positions, we
 * consider upfront margin as 0, but as soon
 * as there is even a single short position, we
 * have to calculate span matrix.
 *
 * @param sec_id
 * @param trade_pos
 */
void SpanCalculator::GetUpfrontMargin(const int sec_id, const int trade_pos){
  AddToRiskMatrix(sec_id, trade_pos);
}

void SpanCalculator::AddToRiskMatrix(const int sec_id, const int position) {
  std::array<double, 16> param_vec = risk_array_sec_id[sec_id];
  int ul = sec_id_to_underlying[sec_id];

  for (unsigned int i = 0; i < 16; i++) {
    pos_risk[ul][i] += (param_vec[i] * position);
  }

  upfront_margin = 0;
  double max_element = 0;

  for (const std::array<double, 16> &arr: pos_risk) {
    max_element = *(std::max_element(arr.begin(), arr.end()));

    if (0 >= max_element) {
      max_element = 0;
      continue;
    }

    upfront_margin += max_element;
  }

}

/*
 * Long positions dont have exposure margin.
 *
 * For this function's context current position
 * is "position before adding this trade"
 *
 * @param sec_id
 * @param trade_pos
 */
void SpanCalculator::GetExposureMargin(const int sec_id, const int trade_pos) {
  float rate = secid_to_optelm[sec_id];
  int pos_to_add = 0;

  // Get current position.
  int curr_pos = position_map[sec_id];

  // Get underlying & respective SPOT.
  int ul = sec_id_to_underlying[sec_id];
  double spot = underlying_wise_spotpx[ul];

  if (0 <= curr_pos) {  // LONG curr pos.
    if (0 <= trade_pos) { // LONG trade pos.
      return;
    } else {  // SHORT trade pos.
      if (0 <= (curr_pos + trade_pos)) { // curr + trade > 0 LONG.
        return;
      } else {  // curr + trade < 0 SHORT
        // pos reversal/adding -> LONG to SHORT
        pos_to_add = curr_pos + trade_pos;
        exposure_margin += ((spot * pos_to_add) * rate);
      }
    }
  } else {  // SHORT curr pos.
    if (0 > trade_pos) { // SHORT trade pos, simply add.
      pos_to_add = trade_pos;
      exposure_margin += ((spot * pos_to_add) * rate);
    } else {  // LONG trade pos.
      if (0 <= (curr_pos + trade_pos)){  // curr + trade >= 0 LONG
        // pos reversal/removal -> SHORT to LONG(or 0).
        pos_to_add = -curr_pos;
      } else {  // curr + trade <= 0 SHORT.
        pos_to_add = trade_pos;
      }
      exposure_margin += ((spot * pos_to_add) * rate);
    }
  }
}

/*
 * Applicable on both long and short positions.
 * For Long position only premium is applicable.
 *
 * @param sec_id
 * @param trade_pos
 */
void SpanCalculator::GetNetPremium(const int sec_id, const int position, const double trade_price) {
  net_premium += secid_to_optpx[sec_id] * position;
}

void SpanCalculator::Clear() {
  std::fill(position_map.begin(), position_map.end(), 0); // positions
  std::for_each(pos_risk.begin(), pos_risk.end(), [](auto& arr) { std::fill(arr.begin(), arr.end(), 0.0); }); // upfront margin
  std::fill(underlying_wise_short_positions.begin(), underlying_wise_short_positions.end(), 0);  // exposure margin

  upfront_margin = 0;
  exposure_margin = 0;
  net_premium = 0;
}


int SpanCalculator::GetNumUniqueInstruments() {
  std::set<std::string> uniqueValues;
  for (unsigned int i = 0; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string shortcode = sec_name_indexer_.GetShortcodeFromId(i);
    std::string instrument = GetInstrumentFromShortcode(shortcode);
    uniqueValues.insert(instrument);
  }

  return uniqueValues.size();
}

void SpanCalculator::CreateSecIdUnderlyingVec() {
  sec_id_to_underlying.clear();

  for (unsigned int i = 0; i < sec_name_indexer_.NumSecurityId(); i++) {
    std::string shortcode = sec_name_indexer_.GetShortcodeFromId(i);
    std::string instrument = GetInstrumentFromShortcode(shortcode);

    if ("NIFTY" == instrument)
      sec_id_to_underlying[i] = 0;
    else if ("BANKNIFTY" == instrument)
      sec_id_to_underlying[i] = 1;
    else if ("FINNIFTY" == instrument)
      sec_id_to_underlying[i] = 2;
    else if ("MIDCPNIFTY" == instrument)
      sec_id_to_underlying[i] = 3;
    else if ("BSX" == instrument)
      sec_id_to_underlying[i] = 4;
    else if ("BKX" == instrument)
      sec_id_to_underlying[i] = 5;
  }
}

}
