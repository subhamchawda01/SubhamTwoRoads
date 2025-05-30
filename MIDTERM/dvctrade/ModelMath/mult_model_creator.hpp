/**
        \file ModelMath/mult_model_creator.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_MULT_MODEL_CREATOR_H
#define BASE_MODELMATH_MULT_MODEL_CREATOR_H

#include <map>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/ModelMath/indicator_logger.hpp"
#include "dvctrade/ModelMath/indicator_stats.hpp"
#include "dvctrade/ModelMath/sumvars_logger.hpp"
#include "dvctrade/ModelMath/model_creator.hpp"
#include "dvctrade/ModelMath/base_multiple_model_math.hpp"
#include "dvctrade/ModelMath/signal_algo.hpp"
#include "dvccode/Utils/synthetic_security_manager.hpp"
#include "dvctrade/ModelMath/linear_mult_model_aggregator.hpp"

#define INDICATOR_ORDER_FILENAME "/spare/local/tradeinfo/OfflineInfo/indicator_order_file.txt"

namespace HFSAT {

/** @brief this class is meant to see the modelaggregation arguments and create the right modelmath component
 * and then instantiate the indicators and add them to the modelmath component
 */
class MultModelCreator {
 protected:
  typedef enum {
    kModelCreationPhaseInit,
    kModelCreationPhasePreModel,
    kModelCreationPhaseShortcodeCollection,
    kModelCreationPhaseIndividualIndicatorsStarted,
    kModelCreationPhaseGlobalIndicatorsStarted,
    kModelCreationPhaseEnded,
    kModelCreationPhaseMAX
  } MultModelCreationPhases_t;  ///< internal enum for Finite State Machine during creation

  static std::map<std::string, BaseMultipleModelMath*> shortcode_basemodelmath_map_;
  static std::map<std::string, BaseModelMath*> shortcode_futmodelmath_map_;
  static std::map<unsigned int, std::pair<int, int> > sec_id_product_index_map_;
  static std::vector<std::vector<std::string> > global_indicator_token_strings_vec_;
  static std::vector<std::vector<std::string> > individual_indicator_token_strings_vec_;
  static std::vector<BaseMultipleModelMath*> multiple_model_math_vec_;
  static std::vector<BaseModelMath*> fut_model_math_vec_;
  static SignalAlgo_t signal_algo_;
  static std::vector<std::string> indicator_order_vec_;
  static std::vector<std::string> underlying_vec_;
  static std::vector<PriceType_t> dep_pricetype_vec_;
  static std::vector<SecurityMarketView*> dep_market_view_vec_;
  static std::map<std::string, std::vector<std::string> > shortcode_const_map_;
  static std::map<std::string, std::vector<SecurityMarketView*> > shortocde_const_smv_map_;
  static std::map<std::string, std::string> shortcode_paramfile_map_;
  static std::map<std::string, std::string> shortcode_model_weights_map_;
  static std::vector<std::string> banned_securities_list_;

 public:
  static void CollectShortCodes(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _model_filename_,
                                std::vector<std::string>& _collected_shortcodes_,
                                std::vector<std::string>& ors_source_needed_vec_,
                                std::vector<std::string>& dependent_vec_);

  static std::map<std::string, std::vector<SecurityMarketView*> > GetShcToConstSMVMap() {
    return shortocde_const_smv_map_;
  }
  static std::vector<SecurityMarketView*> GetUnderlyingSMVVector() { return dep_market_view_vec_; }
  static std::map<unsigned int, std::pair<int, int> > GetSecIdMap() { return sec_id_product_index_map_; }
  static std::map<std::string, std::string> GetParamFileMap() { return shortcode_paramfile_map_; }
  static std::vector<PriceType_t> GetPriceTypeVec() { return dep_pricetype_vec_; }
  static void GetModelMathVec(std::vector<BaseMultipleModelMath*>& mult_model_math_vec_) {
    mult_model_math_vec_ = multiple_model_math_vec_;
  }

  static void GetFutModelMathVec(std::vector<BaseModelMath*>& _fut_model_math_vec_) {
    _fut_model_math_vec_ = fut_model_math_vec_;
  }

  static void CreateIndicatorInstances(DebugLogger& t_dbglogger_, const Watch& cr_watch_);

  static CommonIndicator* GetIndicatorFromTokens(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& tokens_, PriceType_t _basepx_pxtype_);

  static std::vector<BaseMultipleModelMath*> CreateModelMathComponent(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                                      const std::string& _model_filename_,
                                                                      int trading_start_mfm,int trading_end_mfm,
                                                                      int runtime_id);

  static void LinkupModelMathToOnReadySources(BaseMultipleModelMath* p_this_model_math_);

  static std::string GetParamFromDate(std::string _model_filename_, int _date_);

  static std::vector<std::string> GetOptionsToTrade(const std::vector<const char*>& tokens_);

  static double GetTokenFromFile(std::string, unsigned int, int, std::vector<double>&);

};
}

#endif  // BASE_MODELMATH_MULT_MODEL_CREATOR_H
