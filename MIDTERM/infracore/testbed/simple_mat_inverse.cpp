#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "infracore/Math/square_matrix.hpp"

int main(int argc, char** argv) {
  FILE* inputFile = fopen(argv[1], "r");
  int dimension = 0;
  if (!inputFile) {
    std::cout << " File not loaded" << std::endl;
    return 0;
  }
  fscanf(inputFile, "%d", &dimension);

  HFSAT::SquareMatrix<double> mat(dimension);
  HFSAT::SquareMatrix<double> mat_out(dimension);

  // int jj = 1;
  // for ( int ii = 1; ii < dimension*dimension ; ii ++)
  //   {
  //     double data_  =0.0;
  //     fscanf(inputFile, "%lf", &data_ );
  //     mat.SetData( (unsigned int ) ((jj-1) / dimension ) , (unsigned int ) (ii-1) % dimension , data_);
  //     jj++;
  //   }
  for (int ii = 0; ii < dimension; ii++) {
    for (int jj = 0; jj < dimension; jj++) {
      double data_ = 0.0;
      fscanf(inputFile, "%lf", &data_);
      mat.SetData(ii, jj, data_);
    }
  }
  //
  std::cout << "INPUT" << std::endl;
  for (int ii = 0; ii < dimension; ii++) {
    for (int jj = 0; jj < dimension; jj++) {
      printf("%20.7lf", mat(ii, jj));
    }
    printf("\n");
  }

  bool success_ = mat.InvertMatrixSimple(mat_out);

  std::cout << "Output" << std::endl;
  for (int ii = 0; ii < dimension; ii++) {
    for (int jj = 0; jj < dimension; jj++) {
      printf("%20.7lf", mat_out(ii, jj));
    }
    printf("\n");
  }
}
