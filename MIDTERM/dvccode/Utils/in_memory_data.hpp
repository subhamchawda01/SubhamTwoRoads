#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <iostream>

#pragma once

namespace HFSAT {
class InMemData {
 public:
  InMemData();
  void AddWord(double);
  // virtual void AddLine(std::vector<double> _line_){};
  void FinishLine();
  int NumLines() const;
  int NumWords() const;
  void GetRow(int, std::vector<double> &) const;
  void GetColumn(int, std::vector<double> &) const;
  double Item(int, int);
  void close(){};
  // virtual ~InMemData(){};
 protected:
  int num_words_in_line;    // colums
  int num_lines;            // rows
  std::vector<double> arr;  // make_template_instead
};
}
