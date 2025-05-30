/**
   \file Utils/HistoricalBarDataGenerator.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717

 */

#include <inttypes.h>
#include <stdio.h>      /* printf */
#include <time.h>
#include <bits/stdc++.h>

#define MARKET_START_TIME_MSECS 13500000
#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22241225
#define BAR_DATA_DIR_PATH "/spare/local/BarData/"

std::string GetStdoutFromCommand(std::string cmd) {

  std::string data;
  FILE * stream;
  const int max_buffer = 256;
  char buffer[max_buffer];
  cmd.append(" 2>&1");
  
  stream = popen(cmd.c_str(), "r");
  if (stream) {
  while (!feof(stream))
  if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
  pclose(stream);
  }
  return data;
}

class PerishableStringTokenizer {
 public:
  std::vector<const char *> tokens_;
  const std::vector<const char *> &GetTokens() const { return tokens_; }
  PerishableStringTokenizer(char *_readline_buffer_, unsigned int _bufferlen_, bool _ignore_after_hash_ = false){
    bool reading_word_ = false;
  for (auto i = 0u; i < _bufferlen_; i++) {
    int int_value_char_ = (int)_readline_buffer_[i];

    if (iscntrl(int_value_char_) &&
        (_readline_buffer_[i] != '\t')) {  // end of string .. should be replaced with == NUL ?
      _readline_buffer_[i] = '\0';
      break;
    }

    if (_ignore_after_hash_ && (_readline_buffer_[i] == '#')) {  // not reading anything after #
      _readline_buffer_[i] = '\0';
      break;
    }

    if (!isspace(int_value_char_) && !reading_word_) {
      tokens_.push_back(&(_readline_buffer_[i]));
      reading_word_ = true;
    }

    if (isspace(int_value_char_) && reading_word_) {  // first space char after word ... replace with NUL
      _readline_buffer_[i] = '\0';
      reading_word_ = false;
    }
  }
  }
};


class BarData { 
  //std::vector<std::ofstream*> file_list;
  std::vector<std::ifstream*> bar_data_file_list;
  //std::vector<std::string> output_file_;
  std::vector<std::string> input_file_;
  std::vector<std::string> underlying_stock_list_;
  /// 0 : keep changing book and showing current book
  /// 1 : event by event
  /// 2 : keep changing book and refreshing screen till the specified time is reached
  bool day_over_;
  int start_unix_time_;
  int end_unix_time_;
  int trading_date_;
  int granularity_;
  int start_time_;  
  int end_time_;  
  std::vector<double> open_px_;
  std::vector<int> open_time_;
  std::vector<double> close_px_;
  std::vector<int> close_time_;
  std::vector<double> high_px_;
  std::vector<double> low_px_;
  std::vector<unsigned int> vol_;
  std::vector<int> count_;
  std::vector<bool> is_open_;
  std::vector<int> start_time_vec_;
  std::vector<int> min_time_vec_;

 public:
  BarData(int trading_date, int last_midnight_sec, 
	   /*std::vector<std::string> _output_file_,*/ std::vector<std::string> _underlying_stock_list_,
           int _granularity_ = 1, const int _start_unix_time_ = 0, const int _end_unix_time_ = 0)
      : 
	//output_file_(_output_file_),
	underlying_stock_list_(_underlying_stock_list_),
        day_over_(false),
        start_unix_time_(_start_unix_time_),
        end_unix_time_(_end_unix_time_),
	trading_date_(trading_date),
        granularity_(_granularity_ * 60),
        start_time_(last_midnight_sec + MARKET_START_TIME_MSECS/1000),
        end_time_(last_midnight_sec + 86400)
  {

    //std::cout << "start_time_, end_time, last_midnight_sec, MARKET_START_TIME_MSECS/1000 :: " << start_time_ << ", " << end_time_ << ", " << last_midnight_sec << ", " << MARKET_START_TIME_MSECS/1000 << std::endl;
    //_watch_.subscribe_DynamicMinutesPeriod(this);
  }

  ~BarData(){
    //std::cout << "TestBook destructor called\n";
   
    for(unsigned int _security_id_=0; _security_id_<underlying_stock_list_.size(); _security_id_++){
        if(open_time_[_security_id_] != 0){
	  std::cout << start_time_vec_[_security_id_] << "," 
                                      //<< open_time_[_security_id_] << "\t" << close_time_[_security_id_] << "\t"
                                      << open_px_[_security_id_] << "," << high_px_[_security_id_] << ","
                                      << low_px_[_security_id_] << "," << close_px_[_security_id_] << ","
                                      << vol_[_security_id_] << std::endl;
        }
    } 

    for(unsigned int i=0; i< underlying_stock_list_.size(); i++){
      //delete file_list[i];
      delete bar_data_file_list[i];
    }
    //std::cout << "TestBook destructor END\n";
  }


  void set_file_name(){
    for( unsigned int i=0; i< underlying_stock_list_.size(); i++ ){ 
      std::ostringstream str1;
      str1 << BAR_DATA_DIR_PATH << underlying_stock_list_[i] ;
      //file_list.push_back(new std::ofstream(output_file_[i].c_str(), ios::out));
      bar_data_file_list.push_back(new std::ifstream(str1.str().c_str(), std::ifstream::in));
      open_px_.push_back(0);
      open_time_.push_back(0);
      close_px_.push_back(0);
      close_time_.push_back(0);
      low_px_.push_back(0);
      high_px_.push_back(0);
      vol_.push_back(0);
      count_.push_back(0);
      is_open_.push_back(true);
      start_time_vec_.push_back(start_time_);
      min_time_vec_.push_back(start_time_ + granularity_);
    }
  }
	
  void GenerateBarData(){
    for( unsigned int i=0; i< underlying_stock_list_.size(); i++ ){
      if(bar_data_file_list[i]->is_open()){
        const int kBufferLen = 1024;
        char readline_buffer_[kBufferLen];
        bzero(readline_buffer_, kBufferLen);
        while(bar_data_file_list[i]->good()){
	  bzero(readline_buffer_, kBufferLen);
          bar_data_file_list[i]->getline(readline_buffer_, kBufferLen);
	  PerishableStringTokenizer st_(readline_buffer_, kBufferLen);
	  const std::vector<const char *> &tokens_ = st_.GetTokens();
          if(tokens_.size() == 12 && atoi(tokens_[0]) >= start_time_ && atoi(tokens_[0]) < end_time_){
	    OnMinBarData(i, tokens_); 
	  }
        }
      }else{
        std::cerr << "FILE for " << underlying_stock_list_[i] << " is not READABLE or does not exist under DIR: " << BAR_DATA_DIR_PATH << ". EXITING ............\n";
        exit(-1);
      }
    }
  }


  inline void OnMinBarData(const unsigned int _security_id_, const std::vector<const char *> &tokens_) {
  
      while(min_time_vec_[_security_id_] <= atoi(tokens_[0])){
        if(open_time_[_security_id_] != 0){
          //std::cout << "  start, end :: " << start_time_vec_[_security_id_] << ", " << min_time_vec_[_security_id_] << std::endl;
          std::cout << start_time_vec_[_security_id_] << ","
                                      //<< open_time_[_security_id_] << "\t" << close_time_[_security_id_] << "\t"
                     << open_px_[_security_id_] << "," << high_px_[_security_id_] << ","
                     << low_px_[_security_id_] << "," << close_px_[_security_id_] << ","
                     << vol_[_security_id_] << std::endl;


          is_open_[_security_id_] =true;
          open_px_[_security_id_] = 0;
          open_time_[_security_id_] = 0;
          close_px_[_security_id_] = 0;
          close_time_[_security_id_] = 0;
          high_px_[_security_id_] = 0;
          low_px_[_security_id_] = 0;
          vol_[_security_id_] = 0;
          count_[_security_id_] = 0;
        }
        start_time_vec_[_security_id_] += granularity_;
        min_time_vec_[_security_id_] = start_time_vec_[_security_id_] + granularity_;
      }
       if(is_open_[_security_id_]){
          //std::cout << "is_open_ = 1\n";
          open_px_[_security_id_] =  atof(tokens_[5]);
          open_time_[_security_id_] =  atoi(tokens_[2]);
          low_px_[_security_id_] = atof(tokens_[7]);
          high_px_[_security_id_] =  atof(tokens_[8]);
          is_open_[_security_id_] = false;
        }
        close_px_[_security_id_] =  atof(tokens_[6]);
        close_time_[_security_id_] = atoi(tokens_[3]);
        low_px_[_security_id_] = (low_px_[_security_id_] > atof(tokens_[7])) ? atof(tokens_[7]) : low_px_[_security_id_];
        high_px_[_security_id_] = (high_px_[_security_id_] < atof(tokens_[8])) ? atof(tokens_[8]) : high_px_[_security_id_];
        vol_[_security_id_] += atoi(tokens_[9]);
        count_[_security_id_] += atoi(tokens_[10]);
    }

};

int main(int argc, char **argv) {
  // Assume that we get the filename of a file that only has all logged EBS data pertaining to one symbol.
  // Load up the File Source
 if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " PRODUCT input_date_YYYYMMDD granularity[default: 1(min)]"
      << std::endl;
    exit(0);
  }

  int granularity = 0;
  if (argc >= 4) {
    granularity = atoi(argv[3]);
  }

  int tradingdate_ = atoi(argv[2]);

  int start_unix_time_ = 0;
  int end_unix_time_ = 24 * 60 * 60;
  if (argc > 4) {
    start_unix_time_ = (atoi(argv[4]) / 100) * 60 * 60 + (atoi(argv[4]) % 100) * 60;
  }
  if (argc > 5) {
    end_unix_time_ = (atoi(argv[5]) / 100) * 60 * 60 + (atoi(argv[5]) % 100) * 60;
  }

  std::vector<std::string> shortcode_list_;


shortcode_list_.push_back(argv[1]);

 std::ostringstream cmd;
 cmd << "date -d'" << tradingdate_ << "' -u +%s" ;
 
 int last_midnight_sec = std::stoi(GetStdoutFromCommand(cmd.str())); 
// std::cout << "last_midnight_sec: " << last_midnight_sec << std::endl;
 BarData * new_book = new BarData(tradingdate_, last_midnight_sec, shortcode_list_, granularity, start_unix_time_, end_unix_time_);

 new_book->set_file_name();

 std::cout << "Date,AAPL.Open,AAPL.High,AAPL.Low,AAPL.Close,AAPL.Volume" << std::endl;
 new_book->GenerateBarData();

  delete new_book;
  return 0;
}
