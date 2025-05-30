/**
    \file Tools/quantile_filter.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include <iostream>
#include <stdlib.h>

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
/**
 * This tool filters a reg data file.
 * for a given 'q', it finds
 *   start := value of dependent whose rank/num_rows = q
 *   end := value of dependent whose rank/num_rows = 1-q
 * if L is given as an argument, it prints all rows whose dependent is between start and end
 * if G is an argument, it acts like a complement of L
 *
 * Further more, start and end are updated after every NUM_LINES_TO_UPDATE lines of input are read
 * as per the function updateQuantileBoundaries
 */

// this function takes d1 ( sorted vector and adds to it elements of d2 so that d1 is still sorted
// Next we assign the value in start and end based on stratIndex and endIndex
void updateQuantileBoundaries(std::vector<double>& d1, std::vector<double>& d2, size_t startIndx, size_t endIndx,
                              double& start, double& end) {
  // sort d2. d1 is already assumed to be sorted
  std::sort(d2.begin(), d2.end());
  size_t sz1 = d1.size();
  size_t sz2 = d2.size();

  std::vector<double> mergedQ = std::vector<double>(sz1 + sz2, 0);
  size_t i, j, k;
  for (i = 0, j = 0, k = 0; i < sz1 && j < sz2;) {
    if (d1[i] < d2[j])
      mergedQ[k++] = d1[i++];
    else
      mergedQ[k++] = d2[j++];
  }
  if (i == sz1)
    while (j < sz2) mergedQ[k++] = d2[j++];
  if (j == sz2)
    while (i < sz1) mergedQ[k++] = d1[i++];
  // So far the code has managed to merge d1 and d2 to get a larger sorted vector

  d2.clear();
  d1 = mergedQ;
  // set the values in start and end
  start = mergedQ[startIndx];
  end = mergedQ[endIndx];
}

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << "For Summary: " << argv[0] << " input_file outfile quantile_as_ratio [GL]\n";
    std::cerr << "if G is chosen, 0 to Q and 1-Q to 1 ";
    std::cerr << "if L is chosen, Q to 1-Q";
    exit(0);
  }
  std::string inputFile = argv[1];
  std::string outputFile = argv[2];
  double q = atof(argv[3]);
  bool filterG = (argv[4][0] == 'G');
  bool filterL = (argv[4][0] == 'L');

  HFSAT::BulkFileReader reader;
  reader.open(inputFile);

  HFSAT::BulkFileWriter writer;
  writer.Open(outputFile);

  const int BUF_LEN = 8 * 1024;
  char line[BUF_LEN];
  std::vector<double> all_dep_data;
  std::vector<double> incremental_dep_data;

  const int NUM_LINES_TO_UPDATE = 500;

  int linesRead = 0;
  double start = 0;
  double end = 0;

  unsigned int len_read = 0;
  while ((len_read = reader.GetLine(line, BUF_LEN)) > 0) {
    ++linesRead;
    if (linesRead % NUM_LINES_TO_UPDATE == 0) {
      size_t startIndex = (size_t)(q * (all_dep_data.size() + incremental_dep_data.size()));
      size_t endIndex = (size_t)((1 - q) * (all_dep_data.size() + incremental_dep_data.size()));
      updateQuantileBoundaries(all_dep_data, incremental_dep_data, startIndex, endIndex, start, end);
      //          std::cout << "updatestartendCalled " << all_dep_data.size( ) <<  " " << incremental_dep_data.size( )
      //          <<
      //              " " << startIndex << " " << endIndex <<
      //              " " << start << " " << end << "\n";
    }

    double depVal = atof(line);  // since we only need the first variable
    incremental_dep_data.push_back(depVal);

    bool isBetweenStartEnd = start < depVal && depVal < end;
    if (linesRead < NUM_LINES_TO_UPDATE || (filterG && !isBetweenStartEnd) || (filterL && isBetweenStartEnd)) {
      writer.Write(line, len_read);  // Write the line read as is. We dont need to parse each element individually
      writer.CheckToFlushBuffer();
    }
  }

  writer.Close();
  reader.close();
}
