#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/ModelMath/implied_vol_adapter.hpp"

namespace HFSAT {

void ImpliedVolAdapter::AddListener(ImpliedVolAdapterListener* _new_model_math_listener_) {
  implied_vol_adapter_listener__ = _new_model_math_listener_;
}

bool ImpliedVolAdapter::UpdateTarget(double _new_target_, double _new_sum_vars_, int _modelmath_index_,
                                     int _product_index_) {
  // There is no need to check for readiness as it is being already handled in ModelMath class
  double future_price_ = SecurityMarketView::GetPriceFromType(
      underlying_pricetye_vec_[_modelmath_index_], underlying_smv_vec_[_modelmath_index_]->market_update_info_);
  SecurityMarketView* option_smv_ =
      (shc_const_smv_map_[underlying_smv_vec_[_modelmath_index_]->shortcode()][_product_index_]);
  double current_option_price_ = SecurityMarketView::GetPriceFromType(underlying_pricetye_vec_[_modelmath_index_],
                                                                      option_smv_->market_update_info_);
  double fair_price_ = 0;
  double change_price_ = 0;

  switch (conversion_method_) {
    case (kMethodBlackScholes):
      fair_price_ = option_matrix_[_modelmath_index_][_product_index_]->ModelImpliedPrice(future_price_, _new_target_);
      change_price_ = fair_price_ - current_option_price_;
      break;

    case (kMethodRandom):
      fair_price_ = _new_target_;
      change_price_ = _new_sum_vars_;
      break;
    default:
      break;
  }
  prev_value_vec_[_modelmath_index_][_product_index_] = fair_price_;
  prev_iv_vec_[_modelmath_index_][_product_index_] = _new_target_;
  // Condition can be put here to propogate only when some small amount of change happened
  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " IVChangeModelOptionPrice: " << fair_price_ << " Prod: " << option_smv_->shortcode()
                                << " "
                                << " ProdID: " << _product_index_ << DBGLOG_ENDL_FLUSH;
  }

  PropagateNewTargetPrice(fair_price_, change_price_, _modelmath_index_, _product_index_);
  return true;
}

void ImpliedVolAdapter::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  int _product_index_ = sec_id_index_map_[_security_id_].first;
  std::vector<SecurityMarketView*> const_smv_list_ =
      shc_const_smv_map_[underlying_smv_vec_[_product_index_]->shortcode()];
  double future_price_ =
      SecurityMarketView::GetPriceFromType(underlying_pricetye_vec_[_product_index_], _market_update_info_);

  for (unsigned int idx = 0; idx < const_smv_list_.size(); idx++) {
    // Here other sanity checks for implied vol can be put
    if (prev_iv_vec_[_product_index_][idx] <= 0) continue;

    double fair_price_ = 0;

    switch (conversion_method_) {
      case (kMethodBlackScholes):
        fair_price_ =
            option_matrix_[_product_index_][idx]->ModelImpliedPrice(future_price_, prev_iv_vec_[_product_index_][idx]);
        break;

      case (kMethodRandom):
      default:
        break;
    }
    prev_value_vec_[_product_index_][idx] = fair_price_;
  }
  current_sec_id_ = _security_id_;
}

void ImpliedVolAdapter::SMVOnReady() {
  if (current_sec_id_ < 0) return;

  int _product_index_ = sec_id_index_map_[current_sec_id_].first;
  std::vector<SecurityMarketView*> const_smv_list_ =
      shc_const_smv_map_[underlying_smv_vec_[_product_index_]->shortcode()];
  for (unsigned int idx = 0; idx < const_smv_list_.size(); idx++) {
    // Here other sanity checks for implied vol can be put
    if (prev_value_vec_[_product_index_][idx] <= 0) continue;

    double change_price_ = prev_value_vec_[_product_index_][idx] -
                           SecurityMarketView::GetPriceFromType(underlying_pricetye_vec_[_product_index_],
                                                                const_smv_list_[idx]->market_update_info_);
    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      DBGLOG_TIME_CLASS_FUNC_LINE << " FutPxChangeModelOptionPrice: " << prev_value_vec_[_product_index_][idx]
                                  << " Prod: " << const_smv_list_[idx]->shortcode()
                                  << " Mk: " << const_smv_list_[idx]->bestbid_price() << " -- "
                                  << const_smv_list_[idx]->bestask_price() << DBGLOG_ENDL_FLUSH;
    }

    PropagateNewTargetPrice(prev_value_vec_[_product_index_][idx], change_price_, _product_index_, idx);
  }
  current_sec_id_ = -1;
}
}
