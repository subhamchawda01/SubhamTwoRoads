/**
    \file MTools/earth_model_stats.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_MTOOLS_EARTH_MODEL_STATS_H
#define BASE_MTOOLS_EARTH_MODEL_STATS_H

#include <vector>
#include <stdio.h>

namespace HFSAT {

#define square(a) a* a

class StatStream {
  double xy, sx, sy, s2x, s2y, tx;
  unsigned int n;

 public:
  StatStream() : xy(0), sx(0), sy(0), s2x(0), s2y(0), n(0) {}
  void insert(double x_, double y_) {
    xy += x_ * y_;
    s2x += square(x_);
    s2y += square(y_);
    sx += x_;
    sy += y_;
    n++;
  }
  double meanX() { return sx / n; }
  double meanY() { return sy / n; }
  double covXY() { return xy / n - meanX() * meanY(); }
  double varX() { return s2x / n - square(meanX()); }
  double varY() { return s2y / n - square(meanY()); }
  double corrXY() { return covXY() / sqrt(varX() * varY()); }
  double beta() { return covXY() / varX(); }
  double sharpeX() {
    double vx = varX();
    return (vx > 0) ? meanX() / sqrt(vx) : -1;
  }
  double sharpeY() {
    double vy = varY();
    return (vy > 0) ? meanY() / sqrt(vy) : -1;
  }
  void Info() {
    printf("Count: %u\nMeanX: %f, MeanY: %f\nVarX: %f, VarY: %f\nCovXY: %f, Coeff: %f\n", n, meanX(), meanY(), varX(),
           varY(), covXY(), beta());
  }
};

struct BasisFunc {
  int index, s;  // s is reflection, takes vaules 1 and -1
  double knot;

  BasisFunc() {}

  BasisFunc(int ind_, int s_, double knot_) : index(ind_), s(s_), knot(knot_) {}
  double val(double x) {
    double tx = s * (x - knot);
    return tx > 0 ? tx : 0;
  }
  std::string toString() {
    char tt[20];
    if (s == 1)
      sprintf(tt, "max(0, x_%-2d - %.6f)", index, knot);
    else
      sprintf(tt, "max(0, %.6f - x_%-2d)", knot, index);
    return std::string(tt);
  }
};

class Term {
 public:
  int index;
  double beta[2];
  double knot[2];
  // just for debug
  int knot_index[2];
  char tt_[100];
  Term(int ind_, double knot1_, double knot2_) {
    index = ind_;
    knot[0] = knot1_;
    knot[1] = knot2_;
    beta[0] = beta[1] = 0.0;
    // std::cerr<< toString() << std::endl;
  }
  Term(int ind_, int knot_index1_, double knot1_, int knot_index2_, double knot2_) {
    index = ind_;
    knot[0] = knot1_;
    knot[1] = knot2_;
    beta[0] = beta[1] = 0.0;
    knot_index[0] = knot_index1_;
    knot_index[1] = knot_index2_;
  }
  double val(double x) {
    if (x > knot[0]) return x - knot[0];
    if (x < knot[1]) return knot[1] - x;
    return 0;
  }
  double val2(double x) {
    if (x > knot[0]) return (x - knot[0]) * beta[0];
    if (x < knot[1]) return (knot[1] - x) * beta[1];
    return 0;
  }
  double val_s(double x, int s) {
    if (s == 1) return std::max(0.0, x - knot[0]);
    if (s == -1) return std::max(0.0, knot[1] - x);
    return 0.0;
  }
  double val(std::vector<std::vector<double> >& x, int j) { return val(x[index][j]); }
  double val2(std::vector<std::vector<double> >& x, int j) { return val2(x[index][j]); }
  double val_s(std::vector<std::vector<double> >& x, int j, int s) { return val_s(x[index][j], s); }
  void setBeta(double b1_, double b2_) {
    beta[0] = b1_;
    beta[1] = b2_;
  }
  void removeProjection(std::vector<double>& y, std::vector<std::vector<double> >& x) {
    for (uint32_t i = 0; i < y.size(); i++) y[i] -= val2(x, i);
  }
  char* toString() {
    sprintf(tt_, "%f*(x_%d - %f(%d)) + %f*(%f(%d)-x_%d)", beta[0], index, knot[0], knot_index[0], beta[1], knot[1],
            knot_index[1], index);
    return tt_;
  }
};

class Term1 {
#define MAX_DEGREE 2  // currently supports upto degree two only.
 public:
  int degree;
  BasisFunc b[MAX_DEGREE];
  double beta;
  Term1() : degree(0){};
  Term1(BasisFunc& f1) {
    degree = 1;
    b[0] = f1;
  }
  Term1(BasisFunc& f1, BasisFunc& f2) {
    degree = 2;
    b[0] = f1;
    b[1] = f2;
  }
  Term1(int index, int reflection, double val) {
    degree = 1;
    b[0] = *(new BasisFunc(index, reflection, (double)val));
  }

  void setBasis(BasisFunc& f1, BasisFunc& f2) {
    degree = 2;
    b[0] = f1;
    b[1] = f2;
  }
  bool addBasis(BasisFunc& f) {
    if (degree == 2)
      return false;
    else
      b[degree++] = f;
    return true;
  }
  Term1(const Term1& t) {
    degree = t.degree;
    for (int i = 0; i < degree; i++) b[i] = t.b[i];
    beta = t.beta;
  }

  Term1 addBasisNew(BasisFunc& f) {
    Term1* t = new Term1(*this);
    t->addBasis(f);
    return *t;
  }

  double val2(std::vector<std::vector<double> >& x, int j) { return val(x, j) * beta; }

  double val(std::vector<std::vector<double> >& x, int j) {
    if (degree == 0) return 1;
    if (degree == 1) return b[0].val(x[b[0].index][j]);
    if (degree == 2)
      return b[0].val(x[b[0].index][j]) * b[1].val(x[b[1].index][j]);
    else
      return 0;
  }

  double getMarsCov(std::vector<double>& y, std::vector<std::vector<double> >& x) {
    StatStream s;
    for (uint32_t i = 0; i < y.size(); i++) s.insert(val(x, i), y[i]);
    beta = s.beta();
    double meanx = s.meanX();
    double varx = s.varX();
    return square(beta) * (varx - square(meanx));
  }

  void removeProjection(std::vector<double>& y, std::vector<std::vector<double> >& x) {
    for (uint32_t i = 0; i < y.size(); i++) y[i] -= val(x, i) * beta;
  }

  std::string toString() {
    char tt[120] = "";
    if (degree == 0)
      sprintf(tt, "1 ");
    else if (degree == 1)
      sprintf(tt, "%6.3f * %s ", beta, (b[0].toString()).c_str());
    else if (degree == 2)
      sprintf(tt, "%6.3f * %s * %s ", beta, (b[0].toString()).c_str(), (b[1].toString()).c_str());
    return std::string(tt);
  }
};

template <typename T>
struct ModelStats {
  double model_mean_, rsquared_, correlation_, stdev_final_dep_, stdev_;
  void getModelStats(std::vector<T> model_, std::vector<std::vector<double> >& x, std::vector<double>& y) {
    StatStream s;
    for (uint32_t i = 0; i < y.size(); i++) {
      double m_val_ = 0.0;
      for (uint32_t j = 0; j < model_.size(); j++) m_val_ += model_[j].val2(x, i);
      s.insert(y[i], m_val_);
    }
    correlation_ = s.corrXY();
    // stdev_final_dep_ = SUM{(yi-fi)^2} = V(y) + V(f) - 2 * COV(f,y) + ( mf-my )^2
    std::cout << "DepMean: " << s.meanX() << "\tModelMean: " << s.meanY() << std::endl;
    stdev_final_dep_ = sqrt(s.varX() + s.varY() - 2 * s.covXY() + square((s.meanX() - s.meanY())));
    stdev_ = sqrt(s.varY());  // model stdev
    rsquared_ = 1 - square(stdev_final_dep_) / s.varX();
    model_mean_ = s.meanY();
  }
};
#ifdef MARS_DEG_2_TERMS
typedef ModelStats<Term1> ModelStats_t;
#else
typedef ModelStats<Term> ModelStats_t;
#endif
}
#endif  // BASE_MTOOLS_EARTH_MODEL_STATS_H
