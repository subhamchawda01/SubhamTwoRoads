	// =====================================================================================
//
//       Filename:  nse_span_calculator.cpp
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

#include "nse_span_calculator.hpp"

#include <vector>

#include "basetrade/rapidxml/rapidxml.hpp"

namespace HFSAT {

/*
 *
 * @param tradindate
 */
NSESpanCalculator::NSESpanCalculator(int tradindate, SecurityNameIndexer* sec_name_indexer)
    : sec_name_indexer_(sec_name_indexer), tradindate_(tradindate), current_filename_() {
  std::stringstream st;
  st << tradindate_;
  date_string_ = st.str();
}

/**
 *
 */

void NSESpanCalculator::ReloadExposureData() {  
    std::string filename = std::string(EXPOSURE_FILE_DIR) + "/exposure_margin_rates." + date_string_;
    //std::cout<<date_string_<<" "<<filename;
    if (FileUtils::ExistsAndReadable(filename)) {
    std::ifstream filereader(filename.c_str(), std::ifstream::in);
     //std::cout<<"here";
    if (filereader.is_open()) {
      while (filereader.good()) {
	//std::cout<<"there";
        int readlen = 1024;
        char buffer[readlen];
        filereader.getline(buffer, readlen);
        //std::cout<<buffer[0];
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

void NSESpanCalculator::GetLatestFile() {
  ParseFile( std::string(SPAN_FILE_DIR) + "/nsccl." + date_string_ + ".s_1.spn" );
  
/*
  std::string filenameprefix_ = "nsccl.";
  int current_file_index = 0;
  if (!current_filename_.empty()) {
    std::string idx_str = current_filename_.substr(17, 1);
    current_file_index = atoi(idx_str.c_str());
  }
  std::string new_filename = current_filename_;
  
  do {
    current_filename_ = new_filename;
    std::ostringstream st;
    st << std::setw(2) << std::setfill('0') << current_file_index + 1;
    std::string new_index = st.str();
    new_filename = filenameprefix_ + date_string_ + ".i" + new_index + "_1.spn";
    new_filename = std::string(SPAN_FILE_DIR) + "/" + new_filename;
    current_file_index++;
  } while (FileUtils::ExistsAndReadable(new_filename));
  *//*
  new_filename = filenameprefix_ + date_string_ + ".s_1.spn";
  new_filename = std::string(SPAN_FILE_DIR) + "/" + new_filename;  
  std::cout<<new_filename;
  std::cout<<current_filename_;
  current_filename_ = new_filename;
  */
//  ParseFile(current_filename_);
}

/**
 *
 * @param filename
 */
void NSESpanCalculator::ParseFile(std::string filename) {

  if ( !FileUtils::ExistsAndReadable( filename ) ) {
    std::cerr << "File does not exit" << std::endl;
    return;
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
 */
void NSESpanCalculator::ReloadSpanData() { GetLatestFile(); }

/**
 *
 * @param phyPf
 */
void NSESpanCalculator::ExtractStockData(rapidxml::xml_node<char>* phyPf) {
  rapidxml::xml_node<char>* name = phyPf->first_node("name");
  if (print_dbg_) std::cout << "ExtractStockData: " << phyPf->name() << phyPf->value() << std::endl;
  std::string instr = "";
  if (name) {
    instr = std::string(name->value());
  }
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
  if (rs) {
    for (rapidxml::xml_node<char>* sc = rs->first_node("a"); sc; sc = sc->next_sibling("a")) {
      risk_scenario.push_back(atof(sc->value()));
    }
    if (print_dbg_) {
      std::cout << "STOCK: " << instr << " " << px << " " << px_scan << " " << vol_scan << " \n";
      for (unsigned i = 0; i < risk_scenario.size(); i++) {
        std::cout << risk_scenario[i] << " ";
      }
      std::cout << std::endl;
    }
    std::string internal_name = std::string("NSE_") + instr;
    AddRiskScenario(internal_name, risk_scenario);
  }
}

/**
 *
 * @param futPf
 */
void NSESpanCalculator::ExtractFuturesData(rapidxml::xml_node<char>* futPf) {
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
void NSESpanCalculator::ExtractFuturesDataPerExpiry(rapidxml::xml_node<char>* fut, const std::string& instrument, int to_record_px) {
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
  if (rs) {
    for (rapidxml::xml_node<char>* sc = rs->first_node("a"); sc; sc = sc->next_sibling("a")) {
      risk_scenario.push_back(atof(sc->value()));
    }
    if (print_dbg_) {
      std::cout << "FUT: " << px << " " << px_scan << " " << vol_scan << " \n";
      for (unsigned i = 0; i < risk_scenario.size(); i++) {
        std::cout << risk_scenario[i] << " ";
      }
      std::cout << std::endl;
    }
    std::string internal_name = std::string("NSE_") + instrument + "_FUT_" + expiry_date;
    AddRiskScenario(internal_name, risk_scenario);
  }
}

/**
 *
 * @param oopPf
 */
void NSESpanCalculator::ExtractOptionsData(rapidxml::xml_node<char>* oopPf) {
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

void NSESpanCalculator::ExtractOptionsDataPerSeries(rapidxml::xml_node<char>* series, const std::string& instrument) {
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
void NSESpanCalculator::ExtractOptionsDataPerPrice(rapidxml::xml_node<char>* opt, const std::string& instrument,
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

  std::vector<double> risk_scenario;
  rapidxml::xml_node<char>* rs = opt->first_node("ra");
  if (rs) {
    for (rapidxml::xml_node<char>* sc = rs->first_node("a"); sc; sc = sc->next_sibling("a")) {
      risk_scenario.push_back(atof(sc->value()));
    }
    if (print_dbg_) {
      std::cout << "OPTION: " << px << " \n";
      for (unsigned i = 0; i < risk_scenario.size(); i++) {
        std::cout << risk_scenario[i] << " ";
      }
      std::cout << std::endl;
    }
    std::stringstream st;
    st << "NSE_" << instrument << "_" << put_or_call << "E_" << expiry_date << "_" << price;
    std::string internal_name = st.str();
    AddRiskScenario(internal_name, risk_scenario);
    sec_id_to_option_minimum_[internal_name] = option_minimum;
  }
}

void NSESpanCalculator::ExtractSpreadData(rapidxml::xml_node<char>* spread) {
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
void NSESpanCalculator::AddRiskScenario(std::string& shortcode, const std::vector<double>& scenario_param) {
  if (print_dbg_) std::cerr << " ADDING for: " << shortcode << std::endl;
  sec_id_to_risk_scenario_vec_[shortcode] = scenario_param;
  sec_id_to_risk_vec_per_pos_[shortcode] = scenario_param;
}

bool NSESpanCalculator::isIndex(std::string instrument){
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
double NSESpanCalculator::GetSpanRisk(const std::vector<std::string>& shortcode, const std::vector<int>& position){
  // Better pass the vector of sec_id only
  //
  std::vector<double> span_risk_vec(SPAN_SCENARIOS, 0.0);
  double option_min = 0.0;
  double max_risk = 0.0;
  //std::cout<"here";
  for (auto i = 0u; i < shortcode.size(); i++) {
    if(IsOption(shortcode[i]) && position[i] >0)
        continue;

    if (IsOption(shortcode[i])) {
      option_min += GetShortOptionMinimum(shortcode[i]) * position[i] * -1;
    }
    //std::cout<<"Option min"<<option_min<<'\n';
    //std::cout << "Shortcode is -> " << shortcode[i] << std::endl;
    //std::cout << sec_id_to_risk_scenario_vec_.size() << std::endl;
    auto scenario_vec_iter = sec_id_to_risk_scenario_vec_.find(shortcode[i]);
 
    if (scenario_vec_iter == sec_id_to_risk_scenario_vec_.end()) {
      //std::cerr << " could not find scenario vec\n";
      //exit(0);
      return 0;
    }

    auto& param_vec = sec_id_to_risk_scenario_vec_[shortcode[i]];
    // std::cout<<"Span risk[0]=\t"<< span_risk_vec[0]<<"\n";
    max_risk = span_risk_vec[0];
    
    //std::cout << shortcode[i] << " ";
    //std::cout<<"\nParam vec"<<std::endl;
    for (auto j = 0u; j < param_vec.size(); j++) {
      // Assuming values given in param  are risk-per uts
      //std::cout<<param_vec[j]<<"\n";
      span_risk_vec[j] += (position[i] * param_vec[j]);
      //std::cout << '\n'<<param_vec[j] << " [ " << span_risk_vec[j] << " " << position[i] << " ] \n";
      // Calculate max here
      if (span_risk_vec[j] > max_risk) {
        max_risk = span_risk_vec[j];
      }
    }
     //std::cout << std::endl;
  }
  //std::cout<<"Position before calender spread charge: "<<max_risk<<std::endl;
  //max_risk += GetCalendarSpreadCharge(shortcode, position);
  //std::cout<<"Position after calender spread charge:  "<<max_risk<<std::endl;
  return std::max(max_risk, option_min);
}

double NSESpanCalculator::GetExposureMargin(const std::vector<std::string>& shortcode, const std::vector<int>& position, const std::string &instrument) {
	double exposure_margin = 0.0;
	float rate;
	if(isIndex(instrument))
                rate = 0.03;
        else{
                rate = 0.05;
                auto iter = underlying_to_exposure_margin_rate.find(instrument);
                if(iter != underlying_to_exposure_margin_rate.end()) {
                        rate = underlying_to_exposure_margin_rate.at(instrument);
		}
	}
	for (auto i = 0u; i < shortcode.size(); i++) {
		if(IsOption(shortcode[i]) && position[i] >0)
		        continue;
		auto scenario_vec_iter = sec_id_to_risk_scenario_vec_.find(shortcode[i]);
		if (scenario_vec_iter == sec_id_to_risk_scenario_vec_.end()) {
		     	//std::cerr << " could not find scenario vec\n";
     			//exit(0);
                        return 0;
    		}
	//std::string instrument = GetInstrumentFromShortcode(shortcode[i])	
	exposure_margin += rate * underlying_to_futpx[instrument] * abs(position[i]);
	}
	if (print_dbg_) std::cout<<"Ticker: "<<instrument<<"\tRate"<<rate<<"\tExp"<<exposure_margin<<"\n";
	return exposure_margin;	
}

std::string NSESpanCalculator::GetInstrumentFromShortcode(const std::string& shortcode){
	int first_underscore  = shortcode.find('_');
	int second_underscore = shortcode.substr(first_underscore+1).find('_');
	std::string instrument = shortcode.substr(first_underscore+1 , second_underscore);
	//std::cout<<instrument<<std::endl;
	return instrument;
}


bool NSESpanCalculator::IsOption(const std::string& name) {
// Can have better way ?
  if (name.find("_PE_") != std::string::npos || name.find("_CE_") != std::string::npos) {
    return true;
  }
  return false;
}

double NSESpanCalculator::GetShortOptionMinimum(const std::string& secname) {
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
double NSESpanCalculator::GetCalendarSpreadCharge(const std::vector<std::string>& shortcode_vec,
                                                  const std::vector<int>& position) {
  /// Currently it's very very inefficient, We need to input such way that filtering spreads is easier
  
  double charge = 0.0;
  //std::cout<<"Here";
  std::map<std::string, std::vector<std::pair<std::string, int> > > underlying_to_expiry_vec;  
  //std::cout<<"Utoexpmap";
  for (unsigned i = 0; i < shortcode_vec.size(); i++) {
    /// "NSE_ZEEL_FUT_20150827_20150802"
    std::cout<<'\n'<<shortcode_vec[i]<<'\n';
    std::string without_exchange = shortcode_vec[i].substr(4, shortcode_vec[i].length() - 1);
    std::string underlying = without_exchange.substr(0, without_exchange.find("_"));
    auto base_to_spread_iter = base_to_spread_to_val_.find(underlying);
    if (base_to_spread_iter == base_to_spread_to_val_.end()) {
      continue;
    } else {
      std::string rem_str = without_exchange.substr(without_exchange.find("_") + 1, without_exchange.length() - 1);
      rem_str = rem_str.substr(rem_str.find("_") + 1, rem_str.length() - 1);
       //std::cout << " expiry_string: " << rem_str<< " ul: " << underlying << std::endl;
      auto underlying_to_expiry_vec_iter = underlying_to_expiry_vec.find(underlying);
      if (underlying_to_expiry_vec_iter == underlying_to_expiry_vec.end()) {
         //std::cout << "adding new: " << rem_str << " pos: " <<position[i] << std::endl;
        underlying_to_expiry_vec[underlying] = {std::make_pair(rem_str, position[i])};
      } else {
	std::cout<<(underlying_to_expiry_vec_iter->second).size()<<'\n';
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
}
