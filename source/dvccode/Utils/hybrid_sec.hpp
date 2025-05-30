#ifndef HYBRID_SEC_MANAGER
#define HYBRID_SEC_MANAGER

#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <iostream>
#include <fstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/safe_array.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#define BASEP2YINFODIR "/spare/local/tradeinfo/p2y/"

namespace HFSAT {

class HybridSecurityManager {
 protected:
  const int trading_date_;
  bool load_dated_files_;

  std::tr1::unordered_set<std::string> hybrid_secs_;
  std::tr1::unordered_set<std::string> secs_added_to_secname_;

  static std::tr1::unordered_set<std::string> hybrid_exch_secs_;
  static std::tr1::unordered_map<std::string, std::string> hybrid_exch_to_actual_exch_map_;
  static std::tr1::unordered_map<std::string, std::string> actual_exch_to_hybrid_exch_map_;
  std::tr1::unordered_map<std::string, std::string> hybrid_to_actual_map_;
  std::tr1::unordered_map<std::string, std::string> actual_to_hybrid_map_;
  std::map<int, int> actual_to_hybrid_id_map_;
  std::vector<std::tr1::unordered_map<int, double> > price_to_yield_maps_;
  std::vector<HFSAT::FastPriceConvertor *> price_convertors_;

  std::tr1::unordered_map<std::string, unsigned int> sec_to_idx_map_;

 public:
  /**
   * This is only used for non CME shortcodes,
   * for CME shortcodes we will have different yield for different days for given prices
   * @param _trading_date_
   */
  HybridSecurityManager(int trading_date);
  HybridSecurityManager(int trading_date, bool load_dated_files);

  virtual ~HybridSecurityManager() {}

  void LoadSecurityMaps();
  void LoadFromDefaultFiles(std::string actual_security, int index, HFSAT::FastPriceConvertor *t_fast_price_convertor);
  bool LoadFromContractDateFiles(std::string actual_securty, int index,
                                 HFSAT::FastPriceConvertor *t_fast_price_convertor);

  bool AddActualSecurity(std::string _shc_) {
    if (hybrid_secs_.find(_shc_) != hybrid_secs_.end()) {
      std::string act_sec_ = hybrid_to_actual_map_[_shc_];
      if (secs_added_to_secname_.find(act_sec_) != secs_added_to_secname_.end()) {
        return false;
      } else {
        secs_added_to_secname_.insert(act_sec_);
        return true;
      }
    }
    return false;
  }

  bool IsHybridSecurity(std::string _shc_) { return (hybrid_secs_.find(_shc_) != hybrid_secs_.end()); }

  double ReturnYieldFromPrice(std::string _shc_, double _price_) {
    if (hybrid_secs_.find(_shc_) != hybrid_secs_.end()) {
      unsigned int idx_ = sec_to_idx_map_[_shc_];
      return price_to_yield_maps_[idx_][price_convertors_[idx_]->GetFastIntPx(_price_)];
    } else {
      std::string t_hyb_sec_ = actual_to_hybrid_map_[_shc_];
      if (hybrid_secs_.find(t_hyb_sec_) != hybrid_secs_.end()) {
        unsigned int idx_ = sec_to_idx_map_[t_hyb_sec_];
        // std::cerr << idx_ << " " << t_hyb_sec_ << " " << _price_ << " " <<
        // price_to_yield_maps_[idx_][price_convertors_[idx_]->GetFastIntPx(_price_)] << "\n";
        return price_to_yield_maps_[idx_][price_convertors_[idx_]->GetFastIntPx(_price_)];
      }
    }

    return 0;  // some value which does not make sense?
  }

  std::string GetActualSecurityFromHybrid(std::string _hyb_shc_) {
    if (hybrid_to_actual_map_.find(_hyb_shc_) != hybrid_to_actual_map_.end()) {
      return hybrid_to_actual_map_[_hyb_shc_];
    }
    return "";
  }

  std::string GetHybridSecurityFromActual(std::string _act_shc_) {
    if (actual_to_hybrid_map_.find(_act_shc_) != actual_to_hybrid_map_.end()) {
      return actual_to_hybrid_map_[_act_shc_];
    }
    return "";
  }

  void AddActualToHybridSecurityId(int _act_shc_id_, int _hyb_shc_id_) {
    actual_to_hybrid_id_map_[_act_shc_id_] = _hyb_shc_id_;
  }

  int GetHybridSecurityIdFromActual(int _act_shc_id_) {
    auto map_itr = actual_to_hybrid_id_map_.find(_act_shc_id_);
    if (map_itr != actual_to_hybrid_id_map_.end()) {
      return map_itr->second;
    }
    return -1;
  }

  SafeArray<double> GetYieldsForHybridShortcode(std::string _hyb_shc_) {
    if (hybrid_secs_.find(_hyb_shc_) != hybrid_secs_.end()) {
      unsigned int idx_ = sec_to_idx_map_[_hyb_shc_];
      int min_int_price_ = 20000;
      for (auto &it : price_to_yield_maps_[idx_]) {
        if (it.first < min_int_price_) {
          min_int_price_ = it.first;
        }
      }
      HFSAT::SafeArray<double> price_to_yield_array_(min_int_price_, price_to_yield_maps_[idx_]);
      return price_to_yield_array_;
    } else {
      HFSAT::SafeArray<double> price_to_yield_array_;
      return price_to_yield_array_;
    }
  }

  static void AddHybridToActualExch(std::string _hyb_exch_, std::string _act_exch_) {
    hybrid_exch_secs_.insert(_hyb_exch_);
    hybrid_exch_to_actual_exch_map_[_hyb_exch_] = _act_exch_;
    actual_exch_to_hybrid_exch_map_[_act_exch_] = _hyb_exch_;
  }

  static bool IsHybridExch(std::string _exch_) { return hybrid_exch_secs_.find(_exch_) != hybrid_exch_secs_.end(); }

  static std::string GetHybridExchFromActual(std::string _act_exch_) {
    return actual_exch_to_hybrid_exch_map_[_act_exch_];
  }

  static std::string GetActualExchFromHybrid(std::string _hyb_exch_) {
    return hybrid_exch_to_actual_exch_map_[_hyb_exch_];
  }
};
}

#endif
