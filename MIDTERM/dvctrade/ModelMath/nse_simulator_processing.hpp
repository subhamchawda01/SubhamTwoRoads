/**
   \file Tools/t1processing.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
    Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_NSE_SIMULATOR_PROCESSING_HPP
#define BASE_NSE_SIMULATOR_PROCESSING_HPP

namespace HFSAT {

void InMemT1Processing(const InMemData* timeseries_, InMemData* change_data_, const int msecs_to_predict_,
                       const int number_of_dependants_, bool is_returns_based_ = false) {
  // we need one base_rowindex and one future_rowindex
  // first column is time and second column is ( events/trades )
  // we need to know number of dependants on which change is computed

  unsigned int base_row_index_ = 0;
  unsigned int future_row_index_ = 0;

  unsigned int num_columns_ = timeseries_->NumWords();
  unsigned int num_rows_ = timeseries_->NumLines();

  std::vector<double> base_line_(num_columns_, 0);
  std::vector<double> future_line_(num_columns_, 0);

  int base_msecs_ = 0;
  int future_msecs_ = 0;
  bool found_ = false;
  // base pointer is good
  while (base_row_index_ < num_rows_) {
    // future index is good
    found_ = false;
    timeseries_->GetRow(base_row_index_, base_line_);
    // we dont have check anywhere
    if (base_line_.size() < num_columns_) {
      std::cerr << "Malformed InMemData " << base_row_index_ << " row has less than expected number of words "
                << base_line_.size() << " instead " << num_columns_ << "\n";
      exit(-1);
    }
    base_msecs_ = base_line_[0];

    while (future_row_index_ < num_rows_) {
      timeseries_->GetRow(future_row_index_, future_line_);
      if (future_line_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index_ << " row has less than expected number of words "
                  << future_line_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs_ = future_line_[0];
      if (future_msecs_ > (base_msecs_ + msecs_to_predict_)) {
        found_ = true;
        break;
      }
      future_row_index_++;
    }

    if (found_) {
      // factor_ would be good at this point, a vector to be used for all rows or matrix weights data
      // dependants here
      for (int dependants_ = 0; dependants_ < number_of_dependants_; dependants_++) {
        register double change_value_ = future_line_[dependants_ + 2] - base_line_[dependants_ + 2];
        if (is_returns_based_) {
          change_value_ /= base_line_[dependants_ + 2];
        }
        change_data_->AddWord(change_value_);
      }

      // signal values here
      for (unsigned int signals_ = number_of_dependants_; signals_ < num_columns_ - 2; signals_++) {
        change_data_->AddWord(base_line_[2 + signals_]);
      }
      change_data_->FinishLine();
    } else {
      break;
    }
    base_row_index_++;
  }
}

void InMemT3Processing(const InMemData* timeseries_, InMemData* change_data_, const int msecs_to_predict_,
                       const int number_of_dependants_, bool is_returns_based_ = false) {
  // we need one base_rowindex and three future_rowindices
  // first column is time and second column is ( events/trades )
  // we need to know number of dependants on which change is computed

  unsigned int base_row_index_ = 0;
  unsigned int future_row_index1_ = 0;
  unsigned int future_row_index2_ = 0;
  unsigned int future_row_index3_ = 0;

  unsigned int num_columns_ = timeseries_->NumWords();
  unsigned int num_rows_ = timeseries_->NumLines();

  std::vector<double> base_line_(num_columns_, 0);
  std::vector<double> future_line1_(num_columns_, 0);
  std::vector<double> future_line2_(num_columns_, 0);
  std::vector<double> future_line3_(num_columns_, 0);

  int base_msecs_ = 0;
  int future_msecs1_ = 0;
  int future_msecs2_ = 0;
  int future_msecs3_ = 0;

  bool found_ = false;

  int msecs_to_predict1_ = std::max(1, (msecs_to_predict_ * 1) / 3);
  int msecs_to_predict2_ = std::max(1, (msecs_to_predict_ * 2) / 3);
  int msecs_to_predict3_ = msecs_to_predict_;

  // weights for each change
  double nfac1_ = 4.0 / 9.0;
  double nfac2_ = 3.0 / 9.0;
  double nfac3_ = 2.0 / 9.0;

  // base pointer is good
  while (base_row_index_ < num_rows_) {
    found_ = false;
    // future index is good
    timeseries_->GetRow(base_row_index_, base_line_);
    // we dont have check anywhere
    if (base_line_.size() < num_columns_) {
      std::cerr << "Malformed InMemData " << base_row_index_ << " row has less than expected number of words "
                << base_line_.size() << " instead " << num_columns_ << "\n";
      exit(-1);
    }
    base_msecs_ = base_line_[0];

    while (future_row_index1_ < num_rows_) {
      timeseries_->GetRow(future_row_index1_, future_line1_);
      if (future_line1_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index1_ << " row has less than expected number of words "
                  << future_line1_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs1_ = future_line1_[0];
      if (future_msecs1_ > (base_msecs_ + msecs_to_predict1_) && future_msecs1_ > base_msecs_) {
        found_ = true;
        break;
      }
      future_row_index1_++;
    }
    if (!found_) {
      break;
    }

    found_ = false;

    while (future_row_index2_ < num_rows_) {
      timeseries_->GetRow(future_row_index2_, future_line2_);
      if (future_line2_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index2_ << " row has less than expected number of words "
                  << future_line2_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs2_ = future_line2_[0];
      if (future_msecs2_ > (base_msecs_ + msecs_to_predict2_) && future_msecs2_ > base_msecs_) {
        found_ = true;
        break;
      }
      future_row_index2_++;
    }
    if (!found_) {
      break;
    }

    found_ = false;
    while (future_row_index3_ < num_rows_) {
      timeseries_->GetRow(future_row_index3_, future_line3_);
      if (future_line3_.size() < num_columns_) {
        std::cerr << "Malformed InMemData " << future_row_index3_ << " row has less than expected number of words "
                  << future_line3_.size() << " instead " << num_columns_ << "\n";
        exit(-1);
      }
      future_msecs3_ = future_line3_[0];
      if (future_msecs3_ > (base_msecs_ + msecs_to_predict3_) && future_msecs3_ > base_msecs_) {
        found_ = true;
        break;
      }
      future_row_index3_++;
    }
    if (!found_) {
      break;
    }

    if (found_) {
      // factor_ would be good at this point, a vector to be used for all rows or matrix weights data
      // dependants here
      for (int dependants_ = 0; dependants_ < number_of_dependants_; dependants_++) {
        register double change_value1_ = future_line1_[dependants_ + 2] - base_line_[dependants_ + 2];
        register double change_value2_ = future_line2_[dependants_ + 2] - base_line_[dependants_ + 2];
        register double change_value3_ = future_line3_[dependants_ + 2] - base_line_[dependants_ + 2];

        register double change_value_ = nfac1_ * change_value1_ + nfac2_ * change_value2_ + nfac3_ * change_value3_;

        if (is_returns_based_) {
          change_value_ /= base_line_[dependants_ + 2];
        }
        change_data_->AddWord(change_value_);
      }
      // signal values here
      for (unsigned int signals_ = number_of_dependants_; signals_ < num_columns_ - 2; signals_++) {
        change_data_->AddWord(base_line_[2 + signals_]);
      }
      change_data_->FinishLine();
    } else {
      break;
    }
    base_row_index_++;
  }
}
}
#endif
