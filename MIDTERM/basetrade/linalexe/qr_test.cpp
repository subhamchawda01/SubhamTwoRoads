#include <iostream>
#include "dvctrade/linal/QRDecomposition.hpp"

void print(std::vector<std::vector<double> > data) {
  for (int j = 0; j < data[0].size(); ++j) {
    for (int i = 0; i < data.size(); ++i) {
      std::cout << data[i][j] << " ";
    }
    std::cout << "\n";
  }
  std::cout << "====================\n";
}

int main() {
  std::vector<double> col1;
  std::vector<double> col2;
  std::vector<double> col3;
  std::vector<double> col4;
  std::vector<double> col5;

  double x = 13.0;
  double y = 17.0;
  double t = 1.0;
  for (int i = 0; i < 10; ++i) {
    t = t + x / y;
    col1.push_back(t);
    t = t - x + 4 / y;
    col2.push_back(t);
    t = (1 + t) + x / y;
    col3.push_back(t);
    t = t + 2 / (1 + y);
    col4.push_back(t);
    t = t + x / 7;
    col5.push_back(t);
  }

  std::vector<std::vector<double> > data;

  data.push_back(col1);
  data.push_back(col2);
  data.push_back(col3);
  data.push_back(col4);
  data.push_back(col5);

  std::vector<std::vector<double> > Q;
  Q.resize(data.size(), std::vector<double>(data[0].size()));

  std::vector<std::vector<double> > R;
  R.resize(data.size(), std::vector<double>(data.size()));

  LINAL::QRDecomposition(data, Q, R);

  print(data);

  print(Q);

  print(R);

  std::vector<std::vector<double> > p;
  p.resize(data.size(), std::vector<double>(data[0].size()));
  LINAL::Mult(Q, R, p);
  print(p);
}
