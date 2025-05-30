#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/LoggedSources/minute_bar_filesource.hpp"

namespace HFSAT {

MidTermFileSource::MidTermFileSource(DebugLogger& t_dbglogger_, const std::string underlying_, const char segment_,
                                     std::string expiry_list)
    : dbglogger_(t_dbglogger_),
      are_we_filtering(false),
      underlying_(underlying_),
      segment_(segment_),
      infile(),
      next_event_() {
  // cur_event_vec = new std::vector< hftrap::defines::MbarEvent >();
  expiry_list_ = expiry_list;
  next_event_timestamp_ = 0;
  curr_event_timestamp_ = 0;
  std::string filename_ = hftrap::loggedsources::MinuteBarDataFileNamer::GetFileName(underlying_, segment_);
  DBGLOG_CLASS_FUNC << "Underlying: " << underlying_ << "\tType:" << segment_ << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
  // std::cout<<"Underlying: " << underlying_<< std::endl;
  // Open file with ifstream
  infile.open(filename_.c_str(), std::ios::in);
  if (false == infile.is_open()) {
    std::cerr << "Unable To Open The DataFile For Reading Data : " << filename_
              << " For Given Underlying : " << underlying_ << " Segment : " << segment_ << std::endl;
  }

  are_we_filtering = false;
  // Max Expiry Can be 2
  for (int32_t i = 0; i <= 9; i++) {
    list_of_expiries_to_consider[i] = 'N';
  }
  if (std::string("ALL") != expiry_list) {
    are_we_filtering = true;
    std::vector<std::string> list_of_expiry_strings;
    HFSAT::PerishableStringTokenizer::StringSplit(expiry_list, ',', list_of_expiry_strings);
    // copying strings to char to make comparions into char later on
    for (auto& itr : list_of_expiry_strings) {
      list_of_expiries_to_consider[atoi(itr.c_str())] = 'Y';
    }
  }
}

void MidTermFileSource::SeekToFirstEventAfter(const time_t r_start_time_, bool& rw_hasevents_) {
  if (infile.good()) {
    // Read the file till r_start_time_
    do {
      memset((void*)line_buffer, 0, 1024);
      infile.getline(line_buffer, 1024);
      if (line_buffer[0] == '\0') break;
      // Skip comments in case we are adding in datafile
      if (std::string::npos != std::string(line_buffer).find("#")) continue;
      HFSAT::PerishableStringTokenizer pst = HFSAT::PerishableStringTokenizer(line_buffer, 1024);
      std::vector<char const*> const& tokens = pst.GetTokens();
      if (NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS != tokens.size() &&
          NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS + 1 != tokens.size())
        continue;  // adjusted data files have more tokens

      next_event_.event_time = atoi(tokens[0]);
      memcpy((void*)next_event_.instrument, (void*)tokens[1], strlen(tokens[1]));
      next_event_.first_trade_time = atoi(tokens[2]);
      next_event_.last_trade_time = atoi(tokens[3]);
      next_event_.expiry_date = atoi(tokens[4]);
      next_event_.open_price = atof(tokens[5]);
      next_event_.close_price = atof(tokens[6]);
      next_event_.low_price = atof(tokens[7]);
      next_event_.high_price = atof(tokens[8]);
      next_event_.total_volume = atoi(tokens[9]);
      next_event_.no_of_trades = atoi(tokens[10]);
    } while ((time_t)next_event_.event_time < r_start_time_);
    curr_event_timestamp_ = next_event_timestamp_;
    next_event_timestamp_ = (time_t)next_event_.event_time;
    SetNextEventVector();
  } else {  // data file not open
    rw_hasevents_ = false;
  }

  rw_hasevents_ = infile.good();
}

bool MidTermFileSource::SetNextEventVector() {
  cur_event_vec.clear();
  if (next_event_.event_time != 0) {
    int to_consider = 1;
    if (true == are_we_filtering) {
      int32_t this_expiry = next_event_.instrument[strlen(next_event_.instrument) - 1] - '0';
      if ('N' == list_of_expiries_to_consider[this_expiry]) to_consider = 0;
    }
    if (strlen(next_event_.instrument) >= 32) {
      std::cerr << "One Of Our Assumption Of Max Instrument Length Has Broken Here : " << next_event_.instrument
                << " With length : " << strlen(next_event_.instrument) << std::endl;
      exit(-1);
    }
    if (to_consider == 1) cur_event_vec.push_back(next_event_);
  }

  while (infile.good()) {
    memset((void*)line_buffer, 0, 1024);
    infile.getline(line_buffer, 1024);
    if (line_buffer[0] == '\0') break;
    if (std::string::npos != std::string(line_buffer).find("#")) continue;
    HFSAT::PerishableStringTokenizer pst = HFSAT::PerishableStringTokenizer(line_buffer, 1024);
    std::vector<char const*> const& tokens = pst.GetTokens();
    if (NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS != tokens.size() &&
        NSE_MEDIUMTERM_MBAR_DATA_FORMAT_TOKENS + 1 != tokens.size())
      continue;  // adjusted data files have more tokens

    next_event_.event_time = atoi(tokens[0]);
    memcpy((void*)next_event_.instrument, (void*)tokens[1], strlen(tokens[1]));
    next_event_.first_trade_time = atoi(tokens[2]);
    next_event_.last_trade_time = atoi(tokens[3]);
    next_event_.expiry_date = atoi(tokens[4]);
    next_event_.open_price = atof(tokens[5]);
    next_event_.close_price = atof(tokens[6]);
    next_event_.low_price = atof(tokens[7]);
    next_event_.high_price = atof(tokens[8]);
    next_event_.total_volume = atoi(tokens[9]);
    next_event_.no_of_trades = atoi(tokens[10]);
    if (next_event_.event_time > next_event_timestamp_) {
      curr_event_timestamp_ = next_event_timestamp_;
      next_event_timestamp_ = next_event_.event_time;
      return true;
    }

    else if (next_event_.event_time != 0) {
      if (true == are_we_filtering) {
        int32_t this_expiry = next_event_.instrument[strlen(next_event_.instrument) - 1] - '0';
        if ('N' == list_of_expiries_to_consider[this_expiry]) continue;
      }
      if (strlen(next_event_.instrument) >= 32) {
        std::cerr << "One Of Our Assumption Of Max Instrument Length Has Broken Here : " << tokens[1]
                  << " With length : " << strlen(tokens[1]) << std::endl;
        exit(-1);
      }

      cur_event_vec.push_back(next_event_);
    }
  }
  next_event_.event_time = 0;

  if (!infile.good()) return false;
  return true;
}

std::vector<hftrap::defines::MbarEvent> MidTermFileSource::GetCurrentEventVector(const time_t time) {
  if (time == curr_event_timestamp_) {
    return cur_event_vec;
  } else
    return std::vector<hftrap::defines::MbarEvent>();
}
}
