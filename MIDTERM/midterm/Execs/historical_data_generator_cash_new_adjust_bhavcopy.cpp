#include <map> 
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

std::map<std::string,double> shortcode_2_last_close_;

void LoadCMBhavCopyParams(const int t_date_) {
  std::ostringstream nse_bhav_copy_file_name_oss_;
  std::string nse_bhav_copy_file_name_ = "";
  std::vector<std::string> month_vec_ = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };

  int this_date = t_date_;
  this_date = HFSAT::HolidayManagerUtils::GetPrevBusinessDayForExchange("NSE", this_date);
  // split into dd mm yy
  std::ostringstream date_string_oss_;
  date_string_oss_ << this_date;
  std::string prev_date_ = date_string_oss_.str();
  nse_bhav_copy_file_name_oss_ << "NSE_Files/Margin_Files/Exposure_Files/cm" << prev_date_.substr(6, 2)
    << month_vec_[std::stoi(prev_date_.substr(4, 2)) - 1] << prev_date_.substr(0, 4)
    << "bhav.csv";
  nse_bhav_copy_file_name_ = std::string("/spare/local/tradeinfo/") + nse_bhav_copy_file_name_oss_.str();
  std::cout<<"FILENAME: " << nse_bhav_copy_file_name_ <<std::endl;
  if (HFSAT::FileUtils::ExistsAndReadable(nse_bhav_copy_file_name_)) {
    std::ifstream nse_bhavcopy_file_;
    nse_bhavcopy_file_.open(nse_bhav_copy_file_name_.c_str(), std::ifstream::in);
    char readline_buffer_[1024];
    if (nse_bhavcopy_file_.is_open()) {
      while (nse_bhavcopy_file_.good()) {
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        nse_bhavcopy_file_.getline(readline_buffer_, sizeof(readline_buffer_));

        std::vector<char*> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        std::string expr_date_;
        std::string trimmed_str_;
        // trim the inst_type field: contains spaces
        if (tokens_.size() >= 9) HFSAT::PerishableStringTokenizer::TrimString(tokens_[1], trimmed_str_, ' ');
        if (tokens_.size() >= 9 && tokens_[0][0] != '#' && (trimmed_str_ == "EQ")) {
          HFSAT::PerishableStringTokenizer::TrimString(tokens_[0], trimmed_str_, ' ');
          std::string shortcode_ = "NSE_" + trimmed_str_;
          shortcode_2_last_close_[shortcode_] = std::stof(tokens_[5]);
           std::cout << "sc, close:: " << shortcode_ << ", " <<  tokens_[5] << "\n";
        }
      }  // end while
    }
    nse_bhavcopy_file_.close();
  }
  else {
    std::cerr << "Fatal error - could not read NSE Bhavcopy file " << nse_bhav_copy_file_name_ << ".Exiting.\n";
    exit(0);
  }
}

double GetClosePriceFromShortCode(const std::string& _shortcode_) {
  std::string local_shc = _shortcode_;
  std::replace(local_shc.begin(), local_shc.end(), '~', '&');
  if (shortcode_2_last_close_.find(local_shc) != shortcode_2_last_close_.end())
    return shortcode_2_last_close_[local_shc];
  else
    return 0;
}

struct Metrics {
  int time;
  std::string name;
  int64_t start_time;
  int64_t end_time;
  double open_price;
  double close_price;
  double high_price;
  double low_price;
  uint64_t volume;
  uint64_t trades;
  double total_trade_qty_price;
};

int getAdjustmentFactor(std::string shortcode){
  double px_last = 0;//  HFSAT::NSESecurityDefinitions::GetClosePriceFromShortCode(shortcode);
  if (px_last == 0) return 1;
 return 0;    
}

double simple_tokenizer(std::string s){
    std::stringstream ss(s);
    std::string word;
    int i =0;
    while (ss >> word) {
      i++;
      if ( i ==6 ) break;
    }
    return stod(word);
}

Metrics line_parse(std::string s){
  Metrics mt;
  std::stringstream ss(s);
    std::string word;
    ss >> word; mt.time = stoi(word);
    ss >> word; mt.name = word;
    ss >> word; mt.start_time = stoi(word);
    ss >> word; mt.end_time = stoi(word);
    ss >> word; mt.open_price = stod(word);
    ss >> word; mt.close_price = stod(word);
    ss >> word; mt.low_price = stod(word);
    ss >> word; mt.high_price = stod(word);
    ss >> word; mt.volume = stod(word);
    ss >> word; mt.trades = stod(word);
    ss >> word; mt.total_trade_qty_price = stod(word);
    return mt;
}

double lastLine(std::string filename){
    std::string lastline="",lastLine = "";    
    std::ifstream fin;
    fin.open(filename);
    // std::cout<<"FILENAME: " << filename <<std::endl;
    while (std::getline(fin, lastLine)) { lastline=lastLine; }
    // std::cout<<"LAST LINE: " << lastline <<std::endl;
    return simple_tokenizer(lastline);
}

int main(int argc, char const * argv[]) {
  if (argc < 4) {
    std::cout << "USAGE : <exec> <dateNextDay> <input_dir>  <output_dir>" << std::endl;
    exit(-1);
  }
  uint32_t date = atoi(argv[1]);
  std::vector<std::string>shortcode_file;
  std::string input_dir_(argv[2]);
  std::string output_dir_(argv[3]);
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (argv[2])) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      if (isalnum(ent->d_name[0]))
      shortcode_file.push_back(ent->d_name);
    }
    closedir (dir);
  } else {
    return EXIT_FAILURE;
  } 
//  HFSAT::SecurityDefinitions::GetUniqueInstance(date).LoadNSESecurityDefinitions();
 LoadCMBhavCopyParams(date);
 for (auto file : shortcode_file) { 
   std::cout << file << std::endl;
   std::string line_;
   std::string input_file = input_dir_ + file; 
   double price_last = lastLine(input_file);
   double last_price_bhav = GetClosePriceFromShortCode("NSE_" + file);
   double adjust_factor = last_price_bhav /price_last;
   if (last_price_bhav == 0 || last_price_bhav == -1 ) adjust_factor = 1;
   else
     std::cout << "Result: " << file << " " << price_last  << " " << last_price_bhav << " " << adjust_factor << '\n';
   std::string outfilename = output_dir_ + file;
   std::ofstream outstream_;
   outstream_.open(outfilename, std::ofstream::app);
  //  std::cout<<"OUTPUT FILE: " << outfilename << std::endl;
   outstream_ << std::fixed;
   outstream_ << std::setprecision(2);

   std::ifstream if_stream_;
  //  std::cout<<"INPUT FILE: " << input_file << std::endl;
   if_stream_.open(input_file, std::ifstream:: in);
   if (!if_stream_.is_open()) {
     std::cerr << "Error : Unable to open product file " << input_file << std::endl;
     exit(-1);
   }
   while (std::getline(if_stream_, line_)) {
   //  std::cout<<"LINE " << line_ << std::endl;
     Metrics mt = line_parse(line_);
     outstream_ << mt.time << "\t" <<
          file << "\t" <<
          mt.start_time << "\t" <<
          mt.end_time << "\t" <<
          mt.open_price * adjust_factor << "\t" <<
          mt.close_price * adjust_factor<< "\t" <<
          mt.low_price * adjust_factor << "\t" <<
          mt.high_price * adjust_factor << "\t" <<
          mt.volume << "\t" <<
          mt.trades << "\t" <<
          mt.total_trade_qty_price * adjust_factor << "\n";
      }
    outstream_.close();
  }
  return 0;
};
