/**
        \file ModelMath/model_creator.hpp

        \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
*/
#ifndef BASE_MODELMATH_MODEL_CREATOR_H
#define BASE_MODELMATH_MODEL_CREATOR_H

#include <map>
#include <vector>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/ModelMath/indicator_logger.hpp"
#include "dvctrade/ModelMath/indicator_stats.hpp"
#include "dvctrade/ModelMath/sumvars_logger.hpp"
#include "dvctrade/ModelMath/base_model_math.hpp"
#include "dvctrade/ModelMath/non_linear_wrapper.hpp"

#include "dvctrade/ModelMath/signal_algo.hpp"
#include "dvccode/Utils/synthetic_security_manager.hpp"

#define NONLINEAR_COMPONENT_ARGS 4
#define INDICATOR_ORDER_FILENAME "/spare/local/tradeinfo/OfflineInfo/indicator_order_file.txt"

namespace HFSAT {

/** @brief this class is meant to see the modelaggregation arguments and create the right modelmath component
 * and then instantiate the indicators and add them to the modelmath component
 */
class ModelCreator {
 protected:
  typedef enum {
    kModelCreationPhaseInit,
    kModelCreationPhasePreModel,
    kModelCreationPhaseCreModel,
    kModelCreationPhasePostModel,
    kModelCreationPhasePostModelInfo,
    kModelCreationPhaseIndicatorStarted,
    kModelTreeCreationPhaseStarted,
    kModelCreationPhaseIndicatorEnded,
    kModelCreationPhaseMAX
  } ModelCreationPhases_t;  ///< internal enum for Finite State Machine during creation

  typedef enum {
    kMModelCreationPhaseModelInit,
    kMModelCreationPhasePostSubModelMath,
    kMModelCreationPhaseModelMath,
    kMModelCreationPhaseSubModelInit,
    kMModelCreationPhaseSubModelMath,
    kMModelCreationPhaseSubModelIndicatorStarted,
    kMModelCreationPhaseSubModelEnd,
    kMModelCreationPhaseShortCodes,
    kMModelCreationPhaseIIndicators,
    kMModelCreationPhaseGIndicators,
    kMModelCreationPhaseModelEnd,
    kMModelCreationPhaseEnd,
    kMModelCreationPhaseMAX
  } MModelCreationPhases_t;  ///< internal enum for Finite State Machine during creation

  typedef enum { kGreekDelta, kGreekGamma, kGreekVega, kGreekTheta, kGreekMax } GreekIndentifier_t;

  static std::map<std::string, BaseModelMath*> modelfilename_basemodelmath_map_;
  static std::vector<BaseModelMath*> modelfilename_basemodelmath_map_exclude_;

  static std::vector<std::vector<std::string> > indicator_token_strings_vec_;
  static std::vector<SignalAlgo_t> signal_algo_vec_;
  static std::vector<BaseModelMath*> model_math_vec_;
  static std::vector<std::string> indicator_order_vec_;
  static std::vector<SecurityMarketView*> dep_market_view_vec_;
  static std::vector<PriceType_t> baseprice_vec_;
  static std::vector<int> model_entry_type_vec_;  // 1 for INDICATOR 2 for NONLINEARCOMPONENT 3  intermediate, 4
                                                  // indicatorend, 5 intercept, 6 TreeStart, 7 TreeeEnd
  static std::vector<double> model_scaling_factor_vec_;

  static std::map<BaseModelMath*, std::vector<std::string> > modelmath_to_indicator_;

 public:
  static std::string CollectShortCodes(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _model_filename_,
                                       std::vector<std::string>& _collected_shortcodes_,
                                       std::vector<std::string>& ors_source_needed_vec_, bool allow_non_dependant_,
                                       bool check_regime_model_ = true);
  static bool CollectRegimeModelShortcodes(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _model_filename_,
                                           std::vector<std::string>& _collected_shortcodes_,
                                           std::vector<std::string>& ors_source_needed_vec_, bool allow_non_dependant_,
                                           std::string& _dep_shortcode_);
  static void CollectMDModelShortCodes(DebugLogger& _dbglogger_, const Watch& _watch_,
                                       std::string _instruction_filename_,
                                       std::vector<std::string>& _dependant_shortcodes_,
                                       std::vector<std::string>& _source_shortcodes_);

  static std::string GetOfflineMixMMSWtsFileName(std::string _model_filename_, int _trading_start_utc_mfm_,
                                                 std::string _dep_shc_);
  static bool ShcPresentInFile(std::string filename, std::string shortcode);
  static void CreateIndicatorInstances(DebugLogger& t_dbglogger_, const Watch& cr_watch_);

  static bool IsShcPresentInOnlineFilename(std::string filename, std::string shortcode);
  static std::string GetOnlinePriceConstFilename(std::string _model_filename_, int _trading_start_utc_mfm_,
                                                 std::string _dep_shc_);
  static std::string GetOnlineBetaKalmanConstFilename(std::string _model_filename_, int _trading_start_utc_mfm_,
                                                      std::string _dep_shc_);

  static bool NeedsAflashFeed();

  static void GetModelMathVec(std::vector<BaseModelMath*>& base_model_math_vec_);

  // static CommonIndicator * GetIndicatorFromTokens ( DebugLogger & _dbglogger_, const Watch & _watch_, const
  // std::vector < const char * > & tokens_ ) ;
  static CommonIndicator* GetIndicatorFromTokens(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::vector<const char*>& tokens_, PriceType_t _basepx_pxtype_);
  static NonLinearWrapper* GetNonLinearComponentFromTokens(BaseModelMath* p_this_model_math_,
                                                           const double this_component_weight_,
                                                           DebugLogger& _dbglogger_, const Watch& _watch_,
                                                           const std::vector<const char*>& tokens_);

  static BaseModelMath* CreateMDModelMath(DebugLogger& _dbglogger_, const Watch& _watch_,
                                          const std::string& _model_filename_);

  static BaseModelMath* CreateModelMathComponent(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                 const std::string& _model_filename_,
                                                 std::vector<double>* _p_scaling_model_vec_, int trading_start_mfm,
                                                 int trading_end_mfm, int runtime_id);
  static BaseModelMath* CreateRegimeModelMath(DebugLogger& _dbglogger_, const Watch& _watch_,
                                              const std::string& _model_filename_,
                                              std::vector<double>* _p_scaling_model_vec_, int trading_start_mfm,
                                              int trading_end_mfm, int runtime_id);

  static void LinkupModelMathToOnReadySources(BaseModelMath* p_this_model_math_,
                                              std::vector<std::string>& shortcodes_affecting_this_model_,
                                              std::vector<std::string>& ors_source_needed_vec_);

  static IndicatorLogger* CreateIndicatorLogger(
      DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
      EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
      const std::string& _output_filename_, const unsigned int t_msecs_to_wait_to_print_again_,
      const unsigned long long t_l1events_timeout_, const unsigned int num_trades_to_wait_print_again_,
      const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
      const std::vector<SecurityMarketView*>& core_indep_smv_vec_, const std::vector<double>& c3_required_cutoffs_,
      unsigned int regime_mode_to_print_ = 0, bool weighted_ = false, bool pbat_bias_ = false);
  static IndicatorStats* CreateIndicatorStats(
      DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
      EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
      const std::string& _output_filename_, const unsigned int t_msecs_to_wait_to_print_again_,
      const unsigned long long t_l1events_timeout_, const unsigned int num_trades_to_wait_print_again_,
      const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
      const std::vector<SecurityMarketView*>& core_indep_smv_vec_, const std::vector<double>& c3_required_cutoffs_,
      unsigned int regime_mode_to_print_ = 0,
      HFSAT::IndicatorLogger::DatagenStats_t samples_print_ = HFSAT::IndicatorLogger::kIndStats,
      bool live_trading = false, std::map<int, std::pair<std::string, std::string> > t_stat_samples_map_live =
                                     std::map<int, std::pair<std::string, std::string> >());

  // hoping to be generic logger, please use param
  static IndicatorLogger* CreateLogger(
      DebugLogger& _dbglogger_, const Watch& _watch_, BulkFileWriter& _bulk_file_writer_,
      EconomicEventsManager& r_economic_events_manager_, const std::string& _model_filename_,
      const std::string& _output_filename_, const unsigned int t_msecs_to_wait_to_print_again_,
      const unsigned long long t_l1events_timeout_, const unsigned int num_trades_to_wait_print_again_,
      const unsigned int t_to_print_on_economic_times_, const unsigned int t_sample_on_core_shortcodes_,
      const std::vector<SecurityMarketView*>& core_indep_smv_vec_, const std::vector<double>& c3_required_cutoffs_,
      unsigned int regime_mode_to_print_ = 0, int type_ = 1);

  static void LinkupIndicatorLoggerToOnReadySources(IndicatorLogger* p_indicator_logger_,
                                                    std::vector<std::string>& shortcodes_affecting_this_model_,
                                                    std::vector<std::string>& ors_source_needed_vec_);
  static void LinkupIndicatorStatsToOnReadySources(IndicatorStats* p_indicator_stats_,
                                                   std::vector<std::string>& shortcodes_affecting_this_model_,
                                                   std::vector<std::string>& ors_source_needed_vec_);
  static void LinkupLoggerToOnReadySources(IndicatorLogger* p_logger_,
                                           std::vector<std::string>& shortcodes_affecting_this_model_,
                                           std::vector<std::string>& ors_source_needed_vec_);
};
}

#endif  // BASE_MODELMATH_MODEL_CREATOR_H
