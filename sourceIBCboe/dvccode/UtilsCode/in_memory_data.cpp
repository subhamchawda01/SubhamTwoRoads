#include "dvccode/Utils/in_memory_data.hpp"

namespace HFSAT {

InMemData::InMemData() : num_words_in_line(0), num_lines(0) {}

void InMemData::AddWord(double _item_) { arr.push_back(_item_); }

void InMemData::FinishLine() {
  if (num_lines == 0) {
    num_words_in_line = arr.size();
  }
  num_lines++;
}

int InMemData::NumLines() const { return num_lines; }

int InMemData::NumWords() const { return num_words_in_line; }

void InMemData::GetRow(int t_idx_, std::vector<double>& r_row_) const {
  if (t_idx_ >= num_lines) {
    std::cerr << "number of lines: " << num_lines << " less than queried index row " << t_idx_ << "\n";
    return;
  }
  r_row_.resize(num_words_in_line);
  int i = 0;
  for (i = 0, t_idx_ = t_idx_ * num_words_in_line; i < num_words_in_line; i++, t_idx_++) {
    r_row_[i] = arr[t_idx_];
  }
}

void InMemData::GetColumn(int t_idx_, std::vector<double>& r_column_) const {
  if (t_idx_ >= num_words_in_line) {
    std::cerr << "number of words in each line: " << num_words_in_line << " less than queried index column " << t_idx_
              << "\n";
    return;
  }

  int t_max_ = t_idx_ + (num_lines - 1) * num_words_in_line;
  r_column_.resize(num_lines);
  for (int i = 0; t_idx_ <= t_max_; t_idx_ = t_idx_ + num_words_in_line, i++) {
    r_column_[i] = arr[t_idx_];
  }
}

double InMemData::Item(int i, int j) { return arr[i * num_words_in_line + j]; }
}
