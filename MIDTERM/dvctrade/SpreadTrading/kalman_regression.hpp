#ifndef HFTRAP_KALMAN_REGRESSION_H
#define HFTRAP_KALMAN_REGRESSION_H

#include "dvctrade/kalman/ekfilter.hpp"
#include "dvccode/CDef/debug_logger.hpp"

namespace Kalman {

class KalmanReg : public Kalman::EKFilter<double, 0> {
 public:
  // making it slightly generic so that it can be used for other purposes as well
  KalmanReg(uint32_t x_dim, uint32_t u_dim, uint32_t w_dim, uint32_t z_dim, uint32_t v_dim,
            HFSAT::DebugLogger& t_dbglogger);
  virtual ~KalmanReg() {}

  // data structures and functions to set vector values to be used to populate kalman matrices/vectors
  std::vector<double> a_;
  void setA(std::vector<double>& t_a_) { CopyVector(t_a_, a_, dim_x * dim_x); }

  std::vector<double> u_;
  void setU(std::vector<double>& t_u_) { CopyVector(t_u_, u_, dim_u); }

  std::vector<double> r_;
  void setR(std::vector<double>& t_r_) { CopyVector(t_r_, r_, dim_v * dim_v); }

  std::vector<double> w_;
  void setW(std::vector<double>& t_w_) { CopyVector(t_w_, w_, dim_x * dim_w); }

  std::vector<double> v_;
  void setV(std::vector<double>& t_v_) { CopyVector(t_v_, v_, dim_z * dim_v); }

  // only vector that keeps on changing frequently
  std::vector<double> h_;
  void setH(std::vector<double>& t_h_) { CopyVector(t_h_, h_, dim_z * dim_x); }

  std::vector<double> q_;
  void setQ(std::vector<double>& t_q_) { CopyVector(t_q_, q_, dim_w * dim_w); }

  std::vector<double> observed_val_;
  void setObservation(std::vector<double>& t_o_) { CopyVector(t_o_, observed_val_, dim_z); }

  std::vector<double> control_inputs_;
  void setControl(std::vector<double>& t_control_) { CopyVector(t_control_, control_inputs_, dim_u); }

  void updateWeights();

  // function to initialize kalman class
  void init_Kalman(std::vector<double>& init_estimate_, std::vector<double>& init_error_covar);

  double getKalmanParam(int offset) { return x(offset); }

  double getKalmanP(int offset_x, int offset_y) { return calculateP()(offset_x, offset_y); }

  double getKalmanQ(int offset_x, int offset_y) { return Q(offset_x, offset_y); }

  double getKalmanR(int offset_x, int offset_y) { return R(offset_x, offset_y); }

 protected:
  void makeA();
  void makeH();
  void makeV();
  void makeR();
  void makeW();
  void makeQ();
  // for our case makeProcess is empty function
  void makeProcess() {}
  void makeMeasure();

  // dimensions of objects
  uint32_t dim_x;
  uint32_t dim_u;
  uint32_t dim_w;
  uint32_t dim_z;
  uint32_t dim_v;

  // Logging class
  HFSAT::DebugLogger& dbglog_;

  // utility function
  void CopyVector(std::vector<double>& src_, std::vector<double>& tgt_, size_t dim);
};
}
#endif
