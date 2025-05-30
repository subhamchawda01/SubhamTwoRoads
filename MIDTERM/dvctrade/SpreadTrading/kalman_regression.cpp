#include "dvctrade/SpreadTrading/kalman_regression.hpp"
#include "dvccode/CDef/debug_logger.hpp"
namespace Kalman {
KalmanReg::KalmanReg(uint32_t x_dim, uint32_t u_dim, uint32_t w_dim, uint32_t z_dim, uint32_t v_dim,
                     HFSAT::DebugLogger& t_dbglogger_)
    : dbglog_(t_dbglogger_) {
  dim_x = x_dim;
  dim_u = u_dim;
  dim_w = w_dim;
  dim_z = z_dim;
  dim_v = v_dim;
  a_.resize(dim_x * dim_x, 0.0);
  u_.resize(dim_u, 0.0);
  r_.resize(dim_v * dim_v, 0.0);
  w_.resize(dim_x * dim_w, 0.0);
  v_.resize(dim_z * dim_v, 0.0);
  h_.resize(dim_z * dim_x, 0.0);
  q_.resize(dim_w * dim_w, 0.0);
  observed_val_.resize(dim_z, 0.0);
  control_inputs_.resize(dim_u, 0.0);
}

// call init function of initial class
void KalmanReg::init_Kalman(std::vector<double>& init_estimate_, std::vector<double>& init_covar_) {
  setDim(dim_x, dim_u, dim_w, dim_z, dim_v);
  if (init_estimate_.size() != dim_x || init_covar_.size() != dim_x * dim_x) {
    dbglog_ << "Kalman::init_Kalman called with vectors of incorrect dimensions \n";
    dbglog_.DumpCurrentBuffer();
    exit(-1);
  }

  Vector temp_x(dim_x);
  Matrix temp_covar(dim_x, dim_x);

  for (auto i = 0u; i < dim_x; i++) {
    temp_x(i) = init_estimate_[i];
    for (unsigned int j = 0; j < dim_x; j++) {
      temp_covar(i, j) = init_covar_[i * dim_x + j];
    }
  }
  init(temp_x, temp_covar);
}

// utility function
void KalmanReg::CopyVector(std::vector<double>& src_, std::vector<double>& tgt_, size_t tgt_size_) {
  if (src_.size() != tgt_size_) {
    dbglog_ << "Kalman::CopyVector called with vector of incorrect dimension \n";
    dbglog_.DumpCurrentBuffer();
    exit(-1);
  }

  tgt_ = src_;
  /*    dbglog_ << "Initializing Target with\n";
      for( size_t i = 0; i < src_.size(); i++ )
      {
        dbglog_ << src_[i] << ' ';
      }
      dbglog_ << '\n'; */
}

// A is dim_x*dim_x matrix
void KalmanReg::makeA() {
  for (size_t i = 0; i < dim_x; i++) {
    for (size_t j = 0; j < dim_x; j++) {
      A(i, j) = a_[dim_x * i + j];
    }
  }
}

// H is dim_z*dim_x matrix
void KalmanReg::makeH() {
  for (size_t i = 0; i < dim_z; i++) {
    for (size_t j = 0; j < dim_x; j++) {
      H(i, j) = h_[dim_z * i + j];
    }
  }
}

// V is a dim_z*dim_v matrix
void KalmanReg::makeV() {
  for (size_t i = 0; i < dim_z; i++) {
    for (size_t j = 0; j < dim_v; j++) {
      V(i, j) = v_[dim_z * i + j];
    }
  }
}

// R is dim_v*dim_v matrix of observation error covariance
void KalmanReg::makeR() {
  for (size_t i = 0; i < dim_v; i++) {
    for (size_t j = 0; j < dim_v; j++) {
      R(i, j) = r_[dim_v * i + j];
    }
  }
}

// W is dim_x*dim_w matrix
void KalmanReg::makeW() {
  for (size_t i = 0; i < dim_x; i++) {
    for (size_t j = 0; j < dim_w; j++) {
      W(i, j) = w_[dim_x * i + j];
    }
  }
}

// Q is a dim_w*dim_w matrix of process error covariance
void KalmanReg::makeQ() {
  for (size_t i = 0; i < dim_w; i++) {
    for (size_t j = 0; j < dim_w; j++) {
      Q(i, j) = q_[dim_w * i + j];
    }
  }
}

// This equals Hx
void KalmanReg::makeMeasure() {
  for (size_t i = 0; i < dim_z; i++) {
    z(i) = 0;
    for (size_t j = 0; j < dim_x; j++) {
      z(i) = z(i) + H(i, j) * x(j);
    }
  }
}

void KalmanReg::updateWeights() {
  Vector temp_z(dim_z);
  for (size_t i = 0; i < dim_z; i++) {
    temp_z(i) = observed_val_[i];
  }
  Vector temp_u(dim_u);
  for (size_t i = 0; i < dim_u; i++) {
    temp_u(i) = control_inputs_[i];
  }
  step(temp_u, temp_z);
}
}
