/**
   \file Tools/timed_data_src_2_reg_data.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/error_codes.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/buffered_vec.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "basetrade/MTools/data_processing.hpp"
#include "basetrade/Tools/simple_line_processor.hpp"

#pragma once

namespace HFSAT {
namespace TD2RD_UTILS {

#define NUM_PRICE_TYPES 4
#define MAX_INDICATORS 1500
#define DATA_NOT_PRESENT -1

struct TimedData {
  int msec;
  double prices[NUM_PRICE_TYPES];
  unsigned long num_l1_events;
  unsigned int num_indicators;
  double indicator_values[MAX_INDICATORS];

  ~TimedData() {}

  bool parse_from_stream(char* ch, int num_chars_in_line) {
    HFSAT::PerishableStringTokenizer base_st_(ch, num_chars_in_line);
    const std::vector<const char*>& base_tokens_ = base_st_.GetTokens();
    if (base_tokens_.size() < 4) return false;

    msec = atoi(base_tokens_[0]);
    num_l1_events = atol(base_tokens_[1]);
    prices[0] = atof(base_tokens_[2]);
    prices[1] = atof(base_tokens_[3]);

    num_indicators = base_tokens_.size() - 4;
    for (auto i = 0u; i < num_indicators; ++i) {
      indicator_values[i] = atof(base_tokens_[i + 4]);
    }
    return true;
  }
};

template <class TimedData>
class TimedDataFileSource : public HFSAT::BufferedDataSource<TimedData> {
  static const int BUFF_CAP = 128 * 1024;  // 128kB
  HFSAT::BulkFileReader reader;
  char buffer[BUFF_CAP];
  int bytes_read;
  int bytes_remaining;
  bool eof_reached;

  int find_end_of_line(const char* ch, const int num_chars_to_check) {
    if (ch == NULL) return -1;
    for (int chars_read = 0; chars_read < num_chars_to_check; ++chars_read)
      if (*(ch + chars_read) == '\n') return chars_read + 1;
    return -1;
  }

  void fetch_data_from_file() {
    /// sets bytes_read to 0.
    /// sets bytes_remaining
    /// updates eof_reached if required

    /// copy if any bytes remaining to the start of buffer
    if (bytes_remaining > 0) memcpy(buffer, buffer + bytes_read, bytes_remaining);
    int bytes_read_now = reader.read(buffer + bytes_remaining, BUFF_CAP - bytes_remaining);
    if (bytes_read_now < BUFF_CAP - bytes_remaining) {
      // eof reached
      eof_reached = true;
    }

    bytes_read = 0;
    bytes_remaining = bytes_remaining + bytes_read_now;
  }

 public:
  TimedDataFileSource(const std::string& file_name)
      : reader(BUFF_CAP), bytes_read(0), bytes_remaining(0), eof_reached(false) {
    reader.open(file_name);
    if (!reader.is_open()) {
      std::cerr << "timed data file does not exist\n";
      exit(-1);
    }
  }

  virtual bool getNextData(TimedData* data) {
    if (bytes_remaining == 0) {
      if (eof_reached)
        return false;  // nothing more to read from file or from buffer
      else
        fetch_data_from_file();  // nothing in buffer, but still need to read data from file
    }
    int num_chars_in_line = find_end_of_line(buffer + bytes_read, bytes_remaining);
    if (num_chars_in_line == -1) {
      // no end of line means we have the data in the buffer, but its not a complete data
      // hence we need to fetch data from the file again
      fetch_data_from_file();
      num_chars_in_line = find_end_of_line(buffer + bytes_read, bytes_remaining);  // recompute
    }

    bool retVal = data->parse_from_stream(buffer + bytes_read, num_chars_in_line);
    bytes_read += num_chars_in_line;
    bytes_remaining -= num_chars_in_line;

    return retVal;
  }
};

class TD2PredictionData {
 public:
  bool isEventPredictor;
  bool isTimePredictor;

  std::vector<int> prediction_gaps;  // expected to be sorted
  std::vector<double> prediction_data;

  BufferedVecIter<TimedData>* base_iter;
  std::vector<BufferedVecIter<TimedData>*> prediction_iter_vec;
  TimedData base_data;

  TD2PredictionData(BufferedVec<TimedData>* td, const std::vector<int>& prediction_gaps_, bool isEventPredictor_)
      : isEventPredictor(isEventPredictor_),
        isTimePredictor(!isEventPredictor_),
        prediction_gaps(),
        prediction_data(),
        base_data() {
    base_iter = td->getIter();
    prediction_iter_vec = std::vector<BufferedVecIter<TimedData>*>();
    for (auto i = 0u; i < prediction_gaps_.size(); ++i) {
      prediction_iter_vec.push_back(td->getIter());
      prediction_gaps.push_back(prediction_gaps_[i]);
    }
    prediction_data.resize(prediction_gaps_.size());
  }

  TimedData* getBaseData() { return &base_data; }

  std::vector<double>* getPredictionData() { return &prediction_data; }

  bool predictForNextEvent() {
    TimedData* tmp = base_iter->getItem();
    if (tmp == NULL) return false;
    memcpy(&base_data, tmp, sizeof(TimedData));  // We need to store this data
    // since the iterator will mark this location for future use

    for (auto i = 0u; i < prediction_gaps.size(); ++i) {
      while (true) {
        TimedData* pred_data = prediction_iter_vec[i]->getItem();
        if (pred_data != NULL) {
          if (isTimePredictor && pred_data->msec <= base_data.msec + prediction_gaps[i]) continue;
          if (isEventPredictor && pred_data->num_l1_events <= base_data.num_l1_events + prediction_gaps[i]) continue;
          prediction_data[i] =
              pred_data->prices[0];  // TODO change this in future when all sorts of prices are stored in timed data
          break;
        } else {
          // future data for this interval is missing, set it to DATA_NOT_PRESENT (-1)
          prediction_data[i] = DATA_NOT_PRESENT;
          break;
        }
      }
    }
    return true;
  }
};
}
}
