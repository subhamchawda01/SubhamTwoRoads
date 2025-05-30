// =====================================================================================
// 
//       Filename:  bardata_logged_message_filesource.cpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  11/23/2022 07:33:24 AM
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

#include <algorithm>
#include "baseinfra/LoggedSources/bardata_logged_message_filesource.hpp"

#define MIN_BARDATA_TOKENS_SIZE 10

namespace HFSAT {

ttime_t BarDataLoggedFileSource::last_processed_bardata_time_ = ttime_t(0,0);

BarDataLoggedFileSource::BarDataLoggedFileSource(DebugLogger& t_dbglogger_, SecurityNameIndexer& t_sec_name_indexer_,
                                                const unsigned int _yyyymmdd_,
                                                const unsigned int t_security_id_, const char* t_bardata_symbol_,
                                                const char* t_bardata_mapped_symbol_in_file_, const char* t_shortcode_,
                                                bool t_is_options_=false, std::string t_options_expiry_="", std::string t_exch_="NSE"):
  ExternalDataListener(),
  dbglogger_(t_dbglogger_),
  sec_name_indexer_(t_sec_name_indexer_),
  simrun_date_(_yyyymmdd_),
  security_id_(t_security_id_),
  bardata_symbol_(t_bardata_symbol_),
  bardata_mapped_symbol_in_file_(t_bardata_mapped_symbol_in_file_),
  shortcode_(t_shortcode_),
  p_time_keeper_(NULL),
  p_nse_bardata_listener_(NULL),
  p_bse_bardata_listener_(NULL),
  p_cboe_bardata_listener_(NULL),
  bulk_file_reader_(128*1024*1024),
  lastline_buffer_(),
  line_read_string(),
  is_options_(t_is_options_),
  options_expiry_(t_options_expiry_),
  exch_(t_exch_){

  next_event_timestamp_.tv_sec = 0;
  next_event_timestamp_.tv_usec = 0;

  std::string t_bardata_filename_ = "DUMMY";

  if (exch_ == "NSE") {
    InstrumentType_t inst_type = BarDataLoggedFileNamer::GetInstrumentTypeFromBarDataSymbol(shortcode_.c_str());  
    t_bardata_filename_ = BarDataLoggedFileNamer::GetName(inst_type, t_bardata_symbol_, _yyyymmdd_);
  }
  else if (exch_ == "BSE") {
    InstrumentType_t inst_type = BseBarDataLoggedFileNamer::GetInstrumentTypeFromBarDataSymbol(shortcode_.c_str());  
    t_bardata_filename_ = BseBarDataLoggedFileNamer::GetName(inst_type, t_bardata_symbol_, _yyyymmdd_);
  }
  else if (exch_ == "CBOE") {
    InstrumentType_t inst_type = CBOEBarDataLoggedFileNamer::GetInstrumentTypeFromBarDataSymbol(shortcode_.c_str());  
    t_bardata_filename_ = CBOEBarDataLoggedFileNamer::GetName(inst_type, t_bardata_symbol_, _yyyymmdd_);
  }

  //std::cout <<t_shortcode_<< " Filename : " <<t_bardata_filename_ << std::endl;

  bulk_file_reader_.open(t_bardata_filename_);
}

void BarDataLoggedFileSource::SeekToFirstEventAfter(const ttime_t r_start_time_, bool& rw_hasevents_){

  std::string this_bardata_mapped_symbol = "";
  std::string this_line_expiry_for_options = "";
  int tokens_size_from_last_line = 0;
  int this_line_event_time = 0;
  bool security_found = false;

  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
      size_t available_len_;
      do {

        memset((void*)&lastline_buffer_,0,sizeof(lastline_buffer_));
        available_len_ = bulk_file_reader_.GetLine(lastline_buffer_, sizeof(lastline_buffer_));
        line_read_string = lastline_buffer_;

        if(available_len_> 0){
          HFSAT::PerishableStringTokenizer pst(lastline_buffer_,sizeof(lastline_buffer_));
          std::vector<char const*> const& tokens = pst.GetTokens();
          tokens_size_from_last_line = tokens.size();

          if( tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE ){ 
            this_bardata_mapped_symbol = tokens[1];   //2nd column is mapped symbol
            this_line_event_time = std::atoi(tokens[0]) + 60;
            next_event_timestamp_ = ttime_t(time_t(this_line_event_time), 0);
            if(true == is_options_){
              this_line_expiry_for_options = tokens[4];
            }
          }
        }

        if(std::string(this_bardata_mapped_symbol).find(bardata_mapped_symbol_in_file_) == std::string::npos) {
          security_found = false;
          continue;
	}else{
          security_found = true;
        }

        if((true == is_options_) && this_line_expiry_for_options != options_expiry_) { security_found = false; continue; }

      } while (available_len_ > 0 && tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE && 
               (next_event_timestamp_ < r_start_time_ || (!security_found)));

      if (available_len_ <= 0 || tokens_size_from_last_line < MIN_BARDATA_TOKENS_SIZE ){ 
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = ttime_t(time_t(this_line_event_time), 0);
      }
    }
  } else {
    rw_hasevents_ = false;
  }

  SetTimeToSkipUntilFirstEvent(r_start_time_);
}

void BarDataLoggedFileSource::ComputeEarliestDataTimestamp(bool& rw_hasevents_){

  std::string this_bardata_mapped_symbol = "";
  std::string this_line_expiry_for_options = "";
  int tokens_size_from_last_line = 0;
  int this_line_event_time = 0;

  if (bulk_file_reader_.is_open()) {
    if (next_event_timestamp_.tv_sec == 0 && next_event_timestamp_.tv_usec == 0) {
      size_t available_len_;
      do {
        memset((void*)&lastline_buffer_,0,sizeof(lastline_buffer_));
        available_len_ = bulk_file_reader_.GetLine(lastline_buffer_, sizeof(lastline_buffer_));
        line_read_string = lastline_buffer_;

        if(available_len_> 0){
          HFSAT::PerishableStringTokenizer pst(lastline_buffer_,sizeof(lastline_buffer_));
          std::vector<char const*> const& tokens = pst.GetTokens();
          tokens_size_from_last_line = tokens.size();

          if( tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE ){ 
            this_bardata_mapped_symbol = tokens[1];   //2nd column is mapped symbol
            this_line_event_time = std::atoi(tokens[0]) + 60;
            if(true == is_options_){
              this_line_expiry_for_options = tokens[4];
            }

          }
        }

        if(std::string(this_bardata_mapped_symbol).find(bardata_mapped_symbol_in_file_) == std::string::npos || ((true == is_options_) && this_line_expiry_for_options != options_expiry_)) continue;
        else break;

      } while (available_len_ > 0 && tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE);

      if (available_len_ <= 0 || tokens_size_from_last_line < MIN_BARDATA_TOKENS_SIZE ){ 
        next_event_timestamp_ =
            ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
        rw_hasevents_ = false;
      } else {
        next_event_timestamp_ = ttime_t(time_t(this_line_event_time), 0);
      }
    }
  } else {
    rw_hasevents_ = false;
  }

}

void BarDataLoggedFileSource::_ProcessThisMsg(){

  NotifyExternalDataListenerListener(security_id_);

  if (exch_ == "NSE") {
  //bar has moved forward
    if(last_processed_bardata_time_.tv_sec != 0 && (next_event_timestamp_ > last_processed_bardata_time_)){
      if(p_nse_bardata_listener_){
        char local_temp_buffer[1024];
        bzero(local_temp_buffer,1024);
        memcpy((void*)&local_temp_buffer,line_read_string.c_str(),line_read_string.length());
        HFSAT::PerishableStringTokenizer pst(local_temp_buffer, 1024);
        std::vector<char const*> const& tokens = pst.GetTokens();

        ttime_t last_bardata_time = last_processed_bardata_time_;
        last_bardata_time.tv_sec -= 60;

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << last_bardata_time.ToString() << ' ' << tokens[1] << " -1 -1 -1 -1 -1 -1 -1 -1 -1" ;
        p_nse_bardata_listener_->TriggerBardataUpdate(security_id_,t_temp_oss_.str().c_str());
      }
    }

    p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    last_processed_bardata_time_ = next_event_timestamp_;
 
    //processing
    if(p_nse_bardata_listener_){
      p_nse_bardata_listener_->TriggerBardataUpdate(security_id_,line_read_string.c_str());
    }
  }
  else if (exch_ == "BSE") {
  //bar has moved forward
    if(last_processed_bardata_time_.tv_sec != 0 && (next_event_timestamp_ > last_processed_bardata_time_)){
      if(p_bse_bardata_listener_){
        char local_temp_buffer[1024];
        bzero(local_temp_buffer,1024);
        memcpy((void*)&local_temp_buffer,line_read_string.c_str(),line_read_string.length());
        HFSAT::PerishableStringTokenizer pst(local_temp_buffer, 1024);
        std::vector<char const*> const& tokens = pst.GetTokens();

        ttime_t last_bardata_time = last_processed_bardata_time_;
        last_bardata_time.tv_sec -= 60;

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << last_bardata_time.ToString() << ' ' << tokens[1] << " -1 -1 -1 -1 -1 -1 -1 -1 -1" ;
        p_bse_bardata_listener_->TriggerBardataUpdate(security_id_,t_temp_oss_.str().c_str());
      }
    }

    p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    last_processed_bardata_time_ = next_event_timestamp_;
 
    //processing
    if(p_bse_bardata_listener_){
      p_bse_bardata_listener_->TriggerBardataUpdate(security_id_,line_read_string.c_str());
    }
  }
  else if (exch_ == "CBOE") {
  //bar has moved forward
    if(last_processed_bardata_time_.tv_sec != 0 && (next_event_timestamp_ > last_processed_bardata_time_)){
      if(p_cboe_bardata_listener_){
        char local_temp_buffer[1024];
        bzero(local_temp_buffer,1024);
        memcpy((void*)&local_temp_buffer,line_read_string.c_str(),line_read_string.length());
        HFSAT::PerishableStringTokenizer pst(local_temp_buffer, 1024);
        std::vector<char const*> const& tokens = pst.GetTokens();

        ttime_t last_bardata_time = last_processed_bardata_time_;
        last_bardata_time.tv_sec -= 60;

        std::ostringstream t_temp_oss_;
        t_temp_oss_ << last_bardata_time.ToString() << ' ' << tokens[1] << " -1 -1 -1 -1 -1 -1 -1 -1 -1" ;
        p_cboe_bardata_listener_->TriggerBardataUpdate(security_id_,t_temp_oss_.str().c_str());
      }
    }

    p_time_keeper_->OnTimeReceived(next_event_timestamp_, security_id_);
    last_processed_bardata_time_ = next_event_timestamp_;
 
    //processing
    if(p_cboe_bardata_listener_){
      p_cboe_bardata_listener_->TriggerBardataUpdate(security_id_,line_read_string.c_str());
    }
  }
}

void BarDataLoggedFileSource::ProcessAllEvents(){

  while(true) { 
  
    _ProcessThisMsg();

    std::string this_bardata_mapped_symbol = "";
    std::string this_line_expiry_for_options = "";
    int tokens_size_from_last_line = 0;
    int this_line_event_time = 0;

    size_t available_len_;
    do {
      memset((void*)&lastline_buffer_,0,sizeof(lastline_buffer_));
      available_len_ = bulk_file_reader_.GetLine(lastline_buffer_, sizeof(lastline_buffer_));
      line_read_string = lastline_buffer_;

      if(available_len_> 0){
        HFSAT::PerishableStringTokenizer pst(lastline_buffer_,sizeof(lastline_buffer_));
        std::vector<char const*> const& tokens = pst.GetTokens();
        tokens_size_from_last_line = tokens.size();

        if( tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE ){ 
          this_bardata_mapped_symbol = tokens[1];   //2nd column is mapped symbol
          this_line_event_time = std::atoi(tokens[0]) + 60;
          if(true == is_options_){
            this_line_expiry_for_options = tokens[4];
          }
        }
      }

      if(std::string(this_bardata_mapped_symbol).find(bardata_mapped_symbol_in_file_) == std::string::npos || ((true == is_options_) && this_line_expiry_for_options != options_expiry_)) continue;
      else break;

    } while (available_len_ > 0 && tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE);

    if (available_len_ <= 0 || tokens_size_from_last_line < MIN_BARDATA_TOKENS_SIZE ){ 
      next_event_timestamp_ =
          ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = ttime_t(time_t(this_line_event_time), 0);
    }
  }
}

void BarDataLoggedFileSource::ProcessEventsTill(const ttime_t _endtime_){

  while(next_event_timestamp_ <= _endtime_) { 
  
    _ProcessThisMsg();

    std::string this_bardata_mapped_symbol = "";
    std::string this_line_expiry_for_options = "";
    int tokens_size_from_last_line = 0;
    int this_line_event_time = 0;

    size_t available_len_;
    do {
      available_len_ = bulk_file_reader_.GetLine(lastline_buffer_, sizeof(lastline_buffer_));
      line_read_string = lastline_buffer_;

      if(available_len_> 0){
        HFSAT::PerishableStringTokenizer pst(lastline_buffer_,sizeof(lastline_buffer_));
        std::vector<char const*> const& tokens = pst.GetTokens();
        tokens_size_from_last_line = tokens.size();

        if( tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE ){ 
          this_bardata_mapped_symbol = tokens[1];   //2nd column is mapped symbol
          this_line_event_time = std::atoi(tokens[0]) + 60;
          if(true == is_options_){
            this_line_expiry_for_options = tokens[4];
          }
        }
      }

      if(std::string(this_bardata_mapped_symbol).find(bardata_mapped_symbol_in_file_) == std::string::npos || ((true == is_options_) && this_line_expiry_for_options != options_expiry_)) continue;
      else break;

    } while (available_len_ > 0 && tokens_size_from_last_line >= MIN_BARDATA_TOKENS_SIZE);

    if (available_len_ <= 0 || tokens_size_from_last_line < MIN_BARDATA_TOKENS_SIZE ){ 
      next_event_timestamp_ =
          ttime_t(time_t(0), 0);  // to indicate to calling process that we don't have any more data
      break;
    } else {
      next_event_timestamp_ = ttime_t(time_t(this_line_event_time), 0);
    }
  }
}

}
