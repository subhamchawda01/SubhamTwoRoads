/**
    \file PCA.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once
#include <set>
#include <string.h>
#include <algorithm>
#include "dvctrade/linal/Matrix.hpp"
#include "dvctrade/linal/SingularValueDecomposition.hpp"

struct PrincipleComponent {
  double eigenValue;
  std::vector<double> eigenVector;
  PrincipleComponent() : eigenValue(0), eigenVector() {}

  PrincipleComponent(double eigenValue_, std::vector<double> eigenVector_)
      : eigenValue(eigenValue_), eigenVector(eigenVector_) {}

  bool operator<(const PrincipleComponent& o) const { return eigenValue > o.eigenValue; }
};

class PCA {
  std::vector<double> std_original_components;
  std::vector<double> std_principal_components;
  std::vector<double> var_original_components;
  std::vector<double> var_principal_components;
  std::vector<double> percent_cumulative_variance;
  std::vector<PrincipleComponent> principleComponents;

  LINAL::Matrix U;
  LINAL::Matrix S;
  LINAL::Matrix V;

  int numRecords;

  void compute_original_std() {
    std_original_components.resize(V.getColumnDimension());
    var_original_components.resize(V.getColumnDimension());

    for (size_t i = 0; i < V.getColumnDimension(); ++i) {
      double s = 0;
      for (size_t j = 0; j < V.getColumnDimension(); ++j) {
        double t = V(i, j) * S(j, j);
        s += t * t;
      }
      var_original_components[i] = (s / std::max(1, numRecords - 1));
      std_original_components[i] = sqrt(var_original_components[i]);
    }
  }

 public:
  PCA(LINAL::Matrix& input, bool mean_removed = false)
      : std_original_components(),
        std_principal_components(),
        var_original_components(),
        var_principal_components(),
        percent_cumulative_variance(),
        principleComponents(),
        U(),
        S(),
        V(),
        numRecords(input.getRowDimension()) {
    assert(input.getRowDimension() >= input.getColumnDimension());
    if (!mean_removed) {  // Mean removal is necessary for correct PCA using SVD
      for (size_t j = 0; j < input.getColumnDimension(); ++j) {
        double sum = 0;
        for (size_t i = 0; i < input.getRowDimension(); ++i) {
          sum += input.get(i, j);
        }
        double mean = sum / input.getRowDimension();

        for (size_t i = 0; i < input.getRowDimension(); ++i) {
          input(i, j) -= mean;
        }
      }
    }

    LINAL::SingularValueDecomposition(input, U, S, V);

    principleComponents.resize(V.getColumnDimension());

    for (size_t j = 0; j < V.getColumnDimension(); j++) {
      principleComponents[j].eigenValue = pow(S.get(j, j), 2);
      principleComponents[j].eigenVector.resize(V.getRowDimension());
      for (size_t i = 0; i < V.getRowDimension(); ++i) {
        principleComponents[j].eigenVector[i] = V.get(i, j);
      }
    }
    std_principal_components.resize(V.getColumnDimension());
    var_principal_components.resize(V.getColumnDimension());
    percent_cumulative_variance.resize(V.getColumnDimension());

    double cum_sum = 0;
    for (size_t i = 0; i < V.getColumnDimension(); ++i) {
      double d_i = S(i, i);
      var_principal_components[i] = (d_i * d_i / std::max(1.0, input.getRowDimension() - 1.0));
      std_principal_components[i] = sqrt(var_principal_components[i]);
      cum_sum += var_principal_components[i];
      percent_cumulative_variance[i] = cum_sum;
    }
    for (size_t i = 0; i < V.getColumnDimension(); ++i) {
      percent_cumulative_variance[i] *= 100.0 / cum_sum;
    }

    // sorting vector of principal components in order of descending eigen value
    // is not required because the svd decomposition algo takes care of this requirement
  }

  ~PCA() {
    for (auto i = 0u; i < principleComponents.size(); ++i) {
      principleComponents[i].eigenVector.clear();
    }
    principleComponents.clear();
  }

  LINAL::Matrix getDiagMatrix() {
    LINAL::Matrix mat(principleComponents.size(), principleComponents.size());
    mat.fill(0);
    std::vector<PrincipleComponent>::iterator it = principleComponents.begin();
    for (auto i = 0u; it != principleComponents.end(); it++, ++i) {
      mat(i, i) = it->eigenValue;
    }
    return mat;
  }

  LINAL::Matrix getFeatureMatrix(unsigned int k) { return V.subMatrix(0, 0, V.getRowDimension(), k); }

  std::vector<PrincipleComponent> getPrincipalComponents() { return principleComponents; }

  // Function that returns the eigen vectors that
  // explain a certain fraction of the variance
  // But we should return atleast one if it exists
  void getSignificantEigenVectors(const double minIndividualVarianceThresholdPct,
                                  const double maxCumulativeThreshholdPct,
                                  std::vector<std::vector<double> >& eigenVectors) {
    for (size_t i = 0; i < V.getColumnDimension(); ++i) {
      if (percent_cumulative_variance[i] > maxCumulativeThreshholdPct) return;
      if (i > 0 &&
          (percent_cumulative_variance[i] - percent_cumulative_variance[i - 1]) < minIndividualVarianceThresholdPct)
        return;
      std::vector<double> ev;
      for (size_t j = 0; j < V.getRowDimension(); ++j) {
        ev.push_back(V(i, j));
      }
      eigenVectors.push_back(ev);
    }
  }

  void GetSignificanceSortedEigenVectors(std::vector<std::vector<double> >& eigenVectors) {
    for (size_t i = 0; i < V.getColumnDimension(); ++i) {
      std::vector<double> ev;
      for (size_t j = 0; j < V.getRowDimension(); ++j) {
        ev.push_back(V(i, j));
      }
      eigenVectors.push_back(ev);
    }
  }

  std::vector<PrincipleComponent> getKPrincipalComponents(const unsigned int k) {
    std::vector<PrincipleComponent> toRet;
    toRet.resize(k);
    std::vector<PrincipleComponent>::iterator it = principleComponents.begin();
    for (auto i = 0u; it != principleComponents.end() && i < k; it++, i++) {
      toRet[i] = *it;
    }
    return toRet;
  }

  std::vector<double> get_original_std() {
    if (std_original_components.size() == 0) compute_original_std();
    return std_original_components;
  }

  std::vector<double> get_principal_std() { return std_principal_components; }

  std::vector<double> get_original_var() {
    if (var_original_components.size() == 0) compute_original_std();
    return var_original_components;
  }

  std::vector<double> get_principal_var() { return var_principal_components; }

  std::vector<double> get_pct_cum_var() { return percent_cumulative_variance; }
};
