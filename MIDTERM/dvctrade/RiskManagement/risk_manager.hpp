/**
    \file RiskManagement/risk_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#ifndef RISK_MANAGER_HPP_
#define RISK_MANAGER_HPP_

#include <vector>
#include <math.h>
#include <algorithm>
#include <cstdlib>

#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/debug_logger.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"

namespace HFSAT {

class RiskManagerListener {
 public:
  virtual ~RiskManagerListener(){};
  virtual void OnRiskManagerUpdate(const unsigned int r_security_id_, double new_risk_) = 0;
};

// TODO:
// 1. Make this class listen to updates from POM's ... DONE
// 2. Calculate minimal risk ... DONE
// 3. Make exec logic listen to this class

class RiskManager : public GlobalPositionChangeListener {
 private:
  /// Added copy constructor to disable it
  RiskManager(const RiskManager&);

 public:
  static RiskManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const SecurityNameIndexer& _sec_name_indexer_,
                                        const std::vector<HFSAT::PromOrderManager*>& r_sid_to_prom_order_manager_map_,
                                        const std::string& _dep_shortcode_, const unsigned int _dep_security_id_) {
    static RiskManager* p_unique_instance_ = NULL;
    if (p_unique_instance_ == NULL) {
      p_unique_instance_ = new RiskManager(_dbglogger_, _watch_, _sec_name_indexer_, r_sid_to_prom_order_manager_map_,
                                           _dep_shortcode_, _dep_security_id_);
    }
    return p_unique_instance_;
  }

 private:
  RiskManager(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityNameIndexer& _sec_name_indexer_,
              const std::vector<HFSAT::PromOrderManager*>& r_sid_to_prom_order_manager_map_,
              const std::string& r_dep_shortcode_, const unsigned int _dep_security_id_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        sec_name_indexer_(_sec_name_indexer_),
        dep_shortcode_(r_dep_shortcode_),
        dep_security_id_(_dep_security_id_),
        global_position_map_(_sec_name_indexer_.NumSecurityId(), 0),
        current_risk_mapped_to_product_position_vec_(_sec_name_indexer_.NumSecurityId(), 0),
        dep_indep_correlation_vec_(_sec_name_indexer_.NumSecurityId(), 0.0),
        risk_manager_listener_vec_() {
    std::vector<std::string> security_shortcodes_;

    // Mark the security ids which constitute this security-family.
    // using the same function for consistency
    RiskManager::CollectORSShortCodes(_dbglogger_, r_dep_shortcode_, security_shortcodes_);

    // looping through the security ids if the security is in the family then
    // add this as a listener
    for (auto i = 0u; i < _sec_name_indexer_.NumSecurityId(); i++) {
      const std::string _this_shortcode_ = sec_name_indexer_.GetShortcodeFromId(i);
      if (HFSAT::VectorUtils::LinearSearchValue(security_shortcodes_, _this_shortcode_)) {
        if (r_sid_to_prom_order_manager_map_[i] != NULL) {
          r_sid_to_prom_order_manager_map_[i]->AddGlobalPositionChangeListener(this);
        }
        dep_indep_correlation_vec_[i] = OfflineReturnsLRDB::GetUniqueInstance(_dbglogger_, _watch_, r_dep_shortcode_)
                                            .GetLRCoeff(r_dep_shortcode_, _this_shortcode_)
                                            .lr_correlation_;
      }
    }
    dbglogger_ << " RiskManage::dep_indep_correlation_vec_ \n";
    for (auto i = 0u; i < dep_indep_correlation_vec_.size(); ++i)
      dbglogger_ << dep_indep_correlation_vec_[i] << " ";
    dbglogger_ << "\n";
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }

 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  const SecurityNameIndexer& sec_name_indexer_;

  const std::string dep_shortcode_;
  const unsigned int dep_security_id_;

  std::vector<int> global_position_map_;
  std::vector<double> current_risk_mapped_to_product_position_vec_;  // Approx. minimal risk representation
  std::vector<double> dep_indep_correlation_vec_;
  std::vector<RiskManagerListener*> risk_manager_listener_vec_;

 public:
  static void CollectORSShortCodes(DebugLogger& _dbglogger_, const std::string& r_dep_shortcode_,
                                   std::vector<std::string>& ors_source_needed_vec_) {
    if (r_dep_shortcode_.compare("LFL_0") == 0) {  // LFL family , consider LFL_*
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_0");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_2");
    } else if (r_dep_shortcode_.compare("LFL_1") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_0");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_2");
    } else if (r_dep_shortcode_.compare("LFL_2") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_2");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_3");
    } else if (r_dep_shortcode_.compare("LFL_3") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_2");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_3");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_5");
    } else if (r_dep_shortcode_.compare("LFL_4") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_2");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_3");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_5");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_6");
    } else if (r_dep_shortcode_.compare("LFL_5") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_3");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_5");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_6");
    } else if (r_dep_shortcode_.compare("LFL_6") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_5");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_6");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFL_7");
    } else if (r_dep_shortcode_.compare("LFI_0") == 0) {  // LFI family , consider LFI_*
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_0");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_2");
    } else if (r_dep_shortcode_.compare("LFI_1") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_0");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_2");
    } else if (r_dep_shortcode_.compare("LFI_2") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_2");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_3");
    } else if (r_dep_shortcode_.compare("LFI_3") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_2");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_3");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_5");
    } else if (r_dep_shortcode_.compare("LFI_4") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_2");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_3");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_5");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_6");
    } else if (r_dep_shortcode_.compare("LFI_5") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_3");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_5");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_6");
    } else if (r_dep_shortcode_.compare("LFI_6") == 0) {
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_5");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_6");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "LFI_7");
    } else if (!r_dep_shortcode_.compare(0, 3, "BAX")) {  // BAX family , consider BAX_*
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "BAX_0");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "BAX_1");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "BAX_2");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "BAX_3");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "BAX_4");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "BAX_5");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "BAX_6");
    } else if (!r_dep_shortcode_.compare(0, 3, "DI1")) {  // BR_DI family , consider BR_DI_*
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "DI1F15");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "DI1F16");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "DI1F17");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "DI1F18");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "DI1F19");
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, (std::string) "DI1F20");
    } else {  // by default just that product
      HFSAT::VectorUtils::UniqueVectorAdd(ors_source_needed_vec_, r_dep_shortcode_);
    }
  }

  inline void OnGlobalPositionChange(const unsigned int r_security_id_, int _new_global_position_) {
    if (global_position_map_[r_security_id_] != _new_global_position_) {
      global_position_map_[r_security_id_] = _new_global_position_;
      current_risk_mapped_to_product_position_vec_[dep_security_id_] = CalculateMinimalRisk();
      UpdateRiskManagerListeners();
    }
  }

  inline void UpdateRiskManagerListeners() {
    for (std::vector<RiskManagerListener*>::iterator pciter = risk_manager_listener_vec_.begin();
         pciter != risk_manager_listener_vec_.end(); pciter++) {
      (*pciter)->OnRiskManagerUpdate(dep_security_id_, current_risk_mapped_to_product_position_vec_[dep_security_id_]);
    }
  }

  inline double CalculateMinimalRisk() {
    double minimal_risk_ = 0.0;
    for (auto i = 0u; i < dep_indep_correlation_vec_.size(); i++) {
      minimal_risk_ += dep_indep_correlation_vec_[i] * global_position_map_[i];
    }
    minimal_risk_ += global_position_map_[dep_security_id_];
    return (minimal_risk_);
  }

  inline double CalculateMinimalRisk_OLD() {
    double positive_centre = 0.0;
    double negative_centre = 0.0;
    int positive_position = 0;
    int negative_position = 0;
    int no_of_spread = 0;
    int residual_position = 0;  // Using extra variables for understanding... reduce later

    for (size_t i = 0; i < global_position_map_.size(); ++i) {
      if (global_position_map_[i] > 0) {
        positive_position += global_position_map_[i];
        positive_centre += global_position_map_[i] * i;
      } else {
        negative_position += global_position_map_[i];
        negative_centre += global_position_map_[i] * i;
      }
    }
    if (positive_position)
      positive_centre /= positive_position;
    else
      return (negative_position);
    if (negative_position)
      negative_centre /= negative_position;
    else
      return (positive_position);
    positive_centre = positive_centre - negative_centre;          // distance b/w two centres
    if (positive_centre < 0) positive_centre = -positive_centre;  // taking abs()

    no_of_spread = std::min(positive_position, -negative_position);
    residual_position = std::abs(positive_position + negative_position);

    double new_risk = (double)(residual_position) + (double)(no_of_spread)*positive_centre * 0.1;
    return (new_risk);
  }

  inline double GetDeltaRisk_OLD(unsigned int t_security_id_, int r_delta_position_) {
    if (r_delta_position_ == 0) return (0.0);

    global_position_map_[t_security_id_] += r_delta_position_;
    double delta_risk =
        CalculateMinimalRisk() - current_risk_mapped_to_product_position_vec_[dep_security_id_];  // risk (curr_pos +
                                                                                                  // r_delta_position_)
                                                                                                  // - risk (curr_pos)
    global_position_map_[t_security_id_] -= r_delta_position_;

    return (delta_risk);
  }

  inline void AddRiskManagerListener(RiskManagerListener* _new_listener_) {
    VectorUtils::UniqueVectorAdd(risk_manager_listener_vec_, _new_listener_);
  }
};
}

#endif /* RISK_MANAGER_HPP_ */
