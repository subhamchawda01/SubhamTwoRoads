#include <iostream>
#include <fstream>
#include <stdio.h>
#include <map>
#include <vector>
#include <string.h>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

using namespace std;
std::map<std::string, std::string> time_to_bar;
int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "expecting :\n"
              << " MIDTERMDATA FILE ---- HFTBARDATA ---- OUTFILE" << '\n';
    return 0;
  }
  ifstream barFile1, barFile2;
  ofstream barFile3;
  std::string file1, file2, out_file;
  file1 = argv[1];
  file2 = argv[2];
  out_file = argv[3];
  std::cout << "FILE1: " << file1 << " FILE2: " << file2 << " OUTFILE: " << out_file << std::endl;
  barFile1.open(file1, std::ifstream::in);
  if (!barFile1.is_open()) {
    std::cerr << "UNABLE TO FILE FOR READING :" << file1 << std::endl;
    return 0;
  }
  barFile2.open(file2, std::ifstream::in);
  if (!barFile2.is_open()) {
    std::cerr << "UNABLE TO FILE FOR READING :" << file2 << std::endl;
    return 0;
  }
  std::cout << "READING FILE 1 " << file1 << std::endl;
  char line_[2048];
  int count = 0;
  while (!barFile1.eof()) {
    memset(line_, 0, sizeof(line_));
    barFile1.getline(line_, sizeof(line_));
    string data_com_bar = line_;
    HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
    const std::vector<const char *> &tokens_ = t_tokenizer_.GetTokens();
    if (tokens_.size() < 11) {
      std::cerr << "Bar Data File Error " << file1 << " : IGNORING LINE : " << data_com_bar << std::endl;
      continue;
    } else if (strcmp(tokens_[9], "0") == 0 && strcmp(tokens_[10], "0") == 0) {
      // std::cerr << "Bar1 Data File Error " << file1 << " : IGNORING LINE : " << data_com_bar << std::endl;
      // return 0;
      count++;
      continue;
    }

    std::stringstream tmp_shortcode_;
    tmp_shortcode_ << tokens_[0] << "_" << tokens_[1];
    time_to_bar[tmp_shortcode_.str()] = data_com_bar;
  }
  cout << "Ignored Line " << count << std::endl;
  std::cout << "READING FILE 2 " << file2 << std::endl;
  while (!barFile2.eof()) {
    memset(line_, 0, sizeof(line_));
    barFile2.getline(line_, sizeof(line_));
    string data_com_bar = line_;
    HFSAT::PerishableStringTokenizer t_tokenizer_(line_, sizeof(line_));
    const std::vector<const char *> &tokens_ = t_tokenizer_.GetTokens();
    if (tokens_.size() < 11) {
      std::cerr << "Bar Data File Error " << file2 << " : IGNORING LINE : " << line_ << std::endl;
      continue;
    }
    std::stringstream tmp_shortcode_;
    tmp_shortcode_ << tokens_[0] << "_" << tokens_[1];
    // std::cout << "FILE2 " << tmp_shortcode_.str() << " LINE: " << line_ <<std::endl;
    // return 0;
    time_to_bar[tmp_shortcode_.str()] = data_com_bar;
  }

  barFile3.open(out_file, std::ofstream::out);
  for (auto bardata : time_to_bar) {
    // std::cout << bardata.first << " : " << bardata.second << std::endl;
    barFile3 << bardata.second << std::endl;
  }

  barFile1.close();
  barFile2.close();
  barFile3.close();
  return 0;
}
