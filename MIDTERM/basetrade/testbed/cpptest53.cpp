#include <iostream>

#define QCB_SBGF 0x00000001
#define QCB_GFDTC 0x00000002
#define QCB_GFDTMP 0x00000004
#define QCB_GFDTML 0x00000008
#define QCB_GFDTMOTL 0x00000010
#define QCB_GFDTET 0x00000020
#define QCB_EGF 0x00000040
#define QCB_EFT 0x00000080
#define QCB_ECAOO 0x00000100

int main() {
  bool should_be_getting_flat_ = false;
  bool getflat_due_to_external_getflat_ = false;
  bool getflat_due_to_close_ = false;
  bool getflat_due_to_max_position_ = false;
  bool getflat_due_to_max_loss_ = false;
  bool getflat_due_to_max_opentrade_loss_ = false;
  bool getflat_due_to_economic_times_ = false;
  bool external_getflat_ = true;
  bool external_freeze_trading_ = true;  //
  bool external_cancel_all_outstanding_orders_ = true;

  int query_control_bits_ = 0;
  if (should_be_getting_flat_) {
    query_control_bits_ |= QCB_SBGF;
  }
  if (getflat_due_to_close_) {
    query_control_bits_ |= QCB_GFDTC;
  }
  if (getflat_due_to_max_position_) {
    query_control_bits_ |= QCB_GFDTMP;
  }
  if (getflat_due_to_max_loss_) {
    query_control_bits_ |= QCB_GFDTML;
  }
  if (getflat_due_to_max_opentrade_loss_) {
    query_control_bits_ |= QCB_GFDTMOTL;
  }
  if (getflat_due_to_economic_times_) {
    query_control_bits_ |= QCB_GFDTET;
  }
  if (external_getflat_) {
    query_control_bits_ |= QCB_EGF;
  }
  if (external_freeze_trading_) {
    query_control_bits_ |= QCB_EFT;
  }
  if (external_cancel_all_outstanding_orders_) {
    query_control_bits_ |= QCB_ECAOO;
  }

  if (query_control_bits_ & QCB_SBGF) {
    std::cout << "QCB_SBGF " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTC) {
    std::cout << "QCB_GFDTC " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTMP) {
    std::cout << "QCB_GFDTMP " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTML) {
    std::cout << "QCB_GFDTML " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTMOTL) {
    std::cout << "QCB_GFDTMOTL " << '\n';
  }
  if (query_control_bits_ & QCB_GFDTET) {
    std::cout << "QCB_GFDTET " << '\n';
  }
  if (query_control_bits_ & QCB_EGF) {
    std::cout << "QCB_EGF " << '\n';
  }
  if (query_control_bits_ & QCB_EFT) {
    std::cout << "QCB_EFT " << '\n';
  }
  if (query_control_bits_ & QCB_ECAOO) {
    std::cout << "QCB_ECAOO " << '\n';
  }

  std::cout << std::endl;
  return 0;
}
