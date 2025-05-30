#include <iostream>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost-git/boost/accumulators/accumulators.hpp>

using namespace boost::accumulators;

int main() {
  accumulator_set<double, stats<tag::mean, tag::moment<2>>> acc;
  acc(1.2);
  acc(1.2);
  acc(1.2);
  acc(1.2);
  acc(1.2);

  std::cout << "Mean: " << mean(acc) << std::endl;
  std::cout << "Moment: " << accumulators::moment<2>(acc) << std::endl;
}
