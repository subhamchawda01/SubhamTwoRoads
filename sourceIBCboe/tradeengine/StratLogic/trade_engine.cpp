/**
   \file StratLogic/trade_engine.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

//#define _DBGLOGGER_TRADE_ENGINE_INFO_
#include <boost/program_options.hpp>
#include "baseinfra/SimPnls/sim_base_pnl.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/Utils/thread.hpp"
#include "baseinfra/OrderRouting/broadcast_manager_sim.hpp"
#include "tradeengine/Utils/Parser.hpp"
#include "tradeengine/CommonInitializer/common_initializer.hpp"
#include "tradeengine/StratLogic/portfolio_risk_manager.hpp"
#include "tradeengine/StratLogic/user_control_manager.hpp"
#include "tradeengine/TheoCalc/HedgeTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MRTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MACDTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MasterTheoCalculator.hpp"
#include "tradeengine/TheoCalc/CorrTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MidTermTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MispriceTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MomentumTheoCalculator.hpp"
#include "tradeengine/TheoCalc/HighMoveTheoCalculator.hpp"
#include "tradeengine/TheoCalc/GapTheoCalculator.hpp"
#include "tradeengine/TheoCalc/GapRevertTheoCalculator.hpp"
#include "tradeengine/TheoCalc/MATheoCalculator.hpp"
#include "tradeengine/TheoCalc/RatioTheoCalculator.hpp"
#include "tradeengine/TheoCalc/VWAPTheoCalculator.hpp"
#include <inttypes.h>

#define MIN_YYYYMMDD 20090920
#define MAX_YYYYMMDD 22201225

typedef std::vector<std::map<std::string, std::string>*> VEC_KEY_VAL_MAP;
typedef std::map<std::string, std::string> KEY_VAL_MAP;
HFSAT::DebugLogger dbglogger_(1024000, 1);
int tradingdate_;
bool is_asm_filter_enable_;
void InitTradesLogger(int tradingdate, int progid, bool livetrading, const std::string& logs_directory,
                      HFSAT::BulkFileWriter& trades_writer_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << logs_directory << "/trades." << tradingdate << "." << progid;
  std::string tradesfilename = t_temp_oss_.str();
  std::cout << "tradesfilename " << tradesfilename << std::endl;

  // open trades file in append mode in livetrading_
  trades_writer_.Open(tradesfilename.c_str(), (livetrading ? std::ios::app : std::ios::out));
}

void InitDbglogger(int tradingdate, int progid, std::vector<std::string>& dbg_code_vec,
                   const std::string& logs_directory, bool livetrading_) {
  std::ostringstream t_temp_oss;
  t_temp_oss << logs_directory << "/log." << tradingdate << "." << progid;
  std::string logfilename_ = t_temp_oss.str();

  dbglogger_.OpenLogFile(logfilename_.c_str(), (livetrading_ ? std::ios::app : std::ios::out));

  for (auto i = 0u; i < dbg_code_vec.size(); i++) {
    // TODO .. add ability to write "WATCH_INFO" instead of 110, and making it
    int dbg_code_to_be_logged = HFSAT::DebugLogger::TextToLogLevel(dbg_code_vec[i].c_str());
    if (dbg_code_to_be_logged <= 0) {
      dbglogger_.SetNoLogs();
      break;
    } else {
      dbglogger_.AddLogLevel(dbg_code_to_be_logged);
    }
  }

  if (dbg_code_vec.size() <= 0) {
    dbglogger_.SetNoLogs();
  }

  // Though we do not exit here, but since it is a very very rare case and important to detect,
  // we are logging OM_ERROR and WATCH_ERROR for every default SIM run.
  dbglogger_.AddLogLevel(WATCH_ERROR);
  dbglogger_.AddLogLevel(OM_ERROR);
  dbglogger_.AddLogLevel(PLSMM_ERROR);

  // dbglogger_.AddLogLevel(TRADING_ERROR);
  // dbglogger_.AddLogLevel(BOOK_ERROR);
  // dbglogger_.AddLogLevel(LRDB_ERROR);
  dbglogger_.AddLogLevel(DBG_MODEL_ERROR);
  //  dbglogger_.AddLogLevel(SMVSELF_ERROR);
}

bool CollectTheoSourceShortCode(KEY_VAL_MAP* key_val_map_, std::vector<std::string>& source_shortcode_list, std::vector<std::string>& asm_symbols_list_) {

  uint32_t count = 0;
  std::string primary_symbol = std::string("PRIMARY") + std::to_string(count);
  std::vector<std::string> theo_shortcode_list_;
  while (key_val_map_->find(primary_symbol) != key_val_map_->end()) {
    std::string shc_ = (*key_val_map_)[primary_symbol];
    if ((HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(shc_) ||
        (HFSAT::BSESecurityDefinitions::IsShortcode(shc_))) &&
        find(asm_symbols_list_.begin(),asm_symbols_list_.end(),shc_) == asm_symbols_list_.end()) {
      theo_shortcode_list_.push_back(shc_);
      count++;
      primary_symbol = std::string("PRIMARY") + std::to_string(count);
    } else {
      dbglogger_ << "WRONG SHORTCODE[ASM MAY BE]: " << shc_ << DBGLOG_ENDL_FLUSH;
      dbglogger_.DumpCurrentBuffer();
      return false;
    }
  }
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_list, theo_shortcode_list_);
  HFSAT::VectorUtils::UniqueVectorAdd(source_shortcode_list, (*key_val_map_)["SECONDARY"]);
  return true;
}

void CollectTheoORSShortCode(KEY_VAL_MAP* key_val_map_, std::vector<std::string>& ors_shortcode_list) {
  HFSAT::VectorUtils::UniqueVectorAdd(ors_shortcode_list, (*key_val_map_)["SECONDARY"]);
  // Will need to add Hedge symbols at a later stage
}

void CreateShortcodeLists(KEY_VAL_MAP& live_file_key_val_map_, std::vector<std::string>& source_shortcode_list_,
                          std::vector<std::string>& ors_shortcode_list_, VEC_KEY_VAL_MAP& vec_key_val_map_,
                          std::string& live_folder_name, std::vector<std::string>& asm_symbols_list_) {

  uint64_t i = 1;
  std::string theo_number = std::string("THEO") + std::to_string(i);
  while (live_file_key_val_map_.find(theo_number) != live_file_key_val_map_.end()) {
    std::string theo_folder_name = live_folder_name + std::string("/") + live_file_key_val_map_[theo_number] + "/";
    std::string theo_cal = theo_folder_name + std::string("MainConfig.cfg");

    // std::cout << " Trying THEOCAL" << i << " " << theo_cal << std::endl;
    std::map<std::string, std::string>* key_val_map = new std::map<std::string, std::string>();
    Parser::ParseConfig(theo_cal, *key_val_map, 1, tradingdate_);
    (*key_val_map)["THEO_FOLDER"] = theo_folder_name;
    std::string theo_sec_shc_ = Parser::GetString(key_val_map, "SECONDARY", "DEFAULT");

    if ((Parser::GetBool(key_val_map, "STATUS", false)) &&
        (HFSAT::ExchangeSymbolManager::CheckIfContractSpecExists(theo_sec_shc_)) &&
	(find(asm_symbols_list_.begin(),asm_symbols_list_.end(), theo_sec_shc_) == asm_symbols_list_.end())) {
      bool is_valid_shc_config_ = CollectTheoSourceShortCode(key_val_map, source_shortcode_list_, asm_symbols_list_);
      if (is_valid_shc_config_) {
        if (key_val_map->find("THEO_IDENTIFIER") == key_val_map->end()) {
          std::cerr << "THEO IDENTIFIER missing for a theo!! (exiting)" << std::endl;
          exit(-1);
        }
        std::string theo_identifier_ = (*key_val_map)["THEO_IDENTIFIER"];
        for (unsigned int i = 0; i < vec_key_val_map_.size(); i++) {
          if (theo_identifier_ == (*vec_key_val_map_[i])["THEO_IDENTIFIER"]) {
            std::cerr << "SAME THEO IDENTIFIER for multiple theos " << theo_identifier_ << " !! (exiting)" << std::endl;
            exit(-1);
          }
        }
        vec_key_val_map_.push_back(key_val_map);
        CollectTheoORSShortCode(key_val_map, ors_shortcode_list_);
      }

    } else if ("MASTER_THEO" == (*key_val_map)["THEO_TYPE"]) {
      vec_key_val_map_.push_back(key_val_map);
    }
    i++;
    theo_number = std::string("THEO") + std::to_string(i);
  }
}

void CreateTheoCalculators(VEC_KEY_VAL_MAP& vec_key_val_map_, std::vector<BaseTheoCalculator*>& theo_vec_,
                           std::vector<BaseTheoCalculator*>& sqoff_theo_vec_,
                           std::map<std::string, BaseTheoCalculator*>& theo_map_,
                           std::map<int, std::deque<SquareOffTheoCalculator*> >& sqoff_theo_map_, HFSAT::Watch& watch_,
                           HFSAT::DebugLogger& dbglogger_, int _trading_start_utc_mfm_, int start_window_msec,
                           int _trading_end_utc_mfm_, int end_window_msec, int aggressive_get_flat_mfm_,
                           int eff_squareoff_start_utc_mfm_, double bid_multiplier_, double ask_multiplier_) {
  int num_theos = vec_key_val_map_.size();
  int start_theo_diff = (num_theos > 0) ? start_window_msec / num_theos : 0;
  int end_theo_diff = (num_theos > 0) ? end_window_msec / num_theos : 0;
  int count = 0;
  for (auto key_val_map : vec_key_val_map_) {
    if (!Parser::GetBool(key_val_map, "STATUS", false)) return;
    if (key_val_map->find("THEO_TYPE") != key_val_map->end()) {
      std::string theo_type = (*key_val_map)["THEO_TYPE"];
      // std::cout << "THEO: " << theo_type << std::endl;
      // if (key_val_map->find("THEO_IDENTIFIER") == key_val_map->end()) {
      // 	std::cerr << "THEO IDENTIFIER missing for a theo!! (exiting)" << std::endl;
      // 	exit(-1);
      // }
      if (theo_type == "RATIO_THEO") {
        // std::cout << "Creating RatioTheo starttime " << _trading_start_utc_mfm_+ (count*start_theo_diff) << " endtime
        // " << _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new RatioTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_ + (count * start_theo_diff),
            _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_,
            bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "MISPRICE_THEO") {
        // std::cout << "Creating MispriceTheo starttime " << _trading_start_utc_mfm_<< " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new MispriceTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_ + (count * start_theo_diff),
            _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_,
            bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "SQUAREOFF_THEO") {
        // std::cout << "Creating SquareoffTheo starttime " << _trading_start_utc_mfm_<< " endtime " <<
        // _trading_end_utc_mfm_ + end_window_msec << std::endl;
        SquareOffTheoCalculator* theo_calc =
            new SquareOffTheoCalculator(key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_,
                                        _trading_end_utc_mfm_ + end_window_msec, aggressive_get_flat_mfm_);
        int sec_id = theo_calc->GetSecondaryID();
        sqoff_theo_vec_.push_back(theo_calc);
        sqoff_theo_map_[sec_id].push_back(theo_calc);
        // Create Delta Theo
      } else if (theo_type == "HEDGE_THEO") {
        // std::cout << "Creating HedgeTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new HedgeTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "VWAP_THEO") {
        // std::cout << "Creating HedgeTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new VWAPTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Ratio Theo
      } else if (theo_type == "MIDTERM_THEO") {
        // Create MidTerm Theo
        // std::cout << "Creating MidTermTheo starttime " << (*key_val_map)["THEO_IDENTIFIER"] << std::endl;
        // std::string theo_type = (*key_val_map)["THEO_TYPE"];
        BaseTheoCalculator* theo_calc = new MidTermTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        MidTermTheoCalculator* midterm_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(theo_calc);
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        if (key_val_map->find("MOMENTUM1") != key_val_map->end()) {
          std::string mom_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MOMENTUM1"];
          std::map<std::string, std::string>* mom_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(mom_param_file_, *mom_key_val_map_);
          BaseTheoCalculator* mom_theo_calc = new MomentumTheoCalculator(
            mom_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(mom_theo_calc);
          MidTermTheoCalculator* mom_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(mom_theo_calc);
          mom_theo_calc_->setStratParamFile(mom_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(mom_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*mom_key_val_map_)["THEO_IDENTIFIER"]] = mom_theo_calc;
        }

        if (key_val_map->find("MACD1") != key_val_map->end()) {
          std::string macd_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MACD1"];
          std::map<std::string, std::string>* macd_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(macd_param_file_, *macd_key_val_map_);
          BaseTheoCalculator* macd_theo_calc = new MACDTheoCalculator(
            macd_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(macd_theo_calc);
          MidTermTheoCalculator* macd_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(macd_theo_calc);
          macd_theo_calc_->setStratParamFile(macd_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(macd_theo_calc));
          // macd_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*macd_key_val_map_)["THEO_IDENTIFIER"]] = macd_theo_calc;
        }

        if (key_val_map->find("MR1") != key_val_map->end()) {
          std::string mr_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MR1"];
          std::map<std::string, std::string>* mr_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(mr_param_file_, *mr_key_val_map_);
          BaseTheoCalculator* mr_theo_calc = new MRTheoCalculator(
            mr_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(mr_theo_calc);
          MidTermTheoCalculator* mr_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(mr_theo_calc);
          mr_theo_calc_->setStratParamFile(mr_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(mr_theo_calc));
          // mr_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*mr_key_val_map_)["THEO_IDENTIFIER"]] = mr_theo_calc;
        }

        if (key_val_map->find("GAP1") != key_val_map->end()) {
          std::string gap_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["GAP1"];
          std::map<std::string, std::string>* gap_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(gap_param_file_, *gap_key_val_map_);
          BaseTheoCalculator* gap_theo_calc = new GapTheoCalculator(
            gap_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(gap_theo_calc);
          MidTermTheoCalculator* gap_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(gap_theo_calc);
          gap_theo_calc_->setStratParamFile(gap_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(gap_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*gap_key_val_map_)["THEO_IDENTIFIER"]] = gap_theo_calc;
        }

        if (key_val_map->find("GAPREVERT1") != key_val_map->end()) {
          std::string gaprevert_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["GAPREVERT1"];
          std::map<std::string, std::string>* gaprevert_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(gaprevert_param_file_, *gaprevert_key_val_map_);
          BaseTheoCalculator* gaprevert_theo_calc = new GapRevertTheoCalculator(
            gaprevert_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(gaprevert_theo_calc);
          MidTermTheoCalculator* gaprevert_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(gaprevert_theo_calc);
          gaprevert_theo_calc_->setStratParamFile(gaprevert_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(gaprevert_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*gaprevert_key_val_map_)["THEO_IDENTIFIER"]] = gaprevert_theo_calc;
        }

        if (key_val_map->find("MA1") != key_val_map->end()) {
          std::string ma_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["MA1"];
          std::map<std::string, std::string>* ma_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(ma_param_file_, *ma_key_val_map_);
          BaseTheoCalculator* ma_theo_calc = new MATheoCalculator(
            ma_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(ma_theo_calc);
          MidTermTheoCalculator* ma_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(ma_theo_calc);
          ma_theo_calc_->setStratParamFile(ma_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(ma_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*ma_key_val_map_)["THEO_IDENTIFIER"]] = ma_theo_calc;
        }

         if (key_val_map->find("HMT1") != key_val_map->end()) {
          std::string hmt_param_file_ = (*key_val_map)["THEO_FOLDER"] + (*key_val_map)["HMT1"];
          std::map<std::string, std::string>* hmt_key_val_map_ = new std::map<std::string, std::string>();
          Parser::ParseConfig(hmt_param_file_, *hmt_key_val_map_);
          BaseTheoCalculator* hmt_theo_calc = new HighMoveTheoCalculator(
            hmt_key_val_map_, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
          theo_vec_.push_back(hmt_theo_calc);
          MidTermTheoCalculator* hmt_theo_calc_ = dynamic_cast<MidTermTheoCalculator*>(hmt_theo_calc);
          hmt_theo_calc_->setStratParamFile(hmt_param_file_);

          midterm_theo_calc_->ConfigureComponentDetails(dynamic_cast<MidTermTheoCalculator*>(hmt_theo_calc));
          // mom_theo_calc->SetParentTheo(theo_calc);
          theo_map_[(*hmt_key_val_map_)["THEO_IDENTIFIER"]] = hmt_theo_calc;
        }

      } else if (theo_type == "MOMENTUM_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new MomentumTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MACD_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new MACDTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MR_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new MRTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "GAP_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new GapTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "GAPREVERT_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new GapRevertTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MA_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new MATheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "HMT_THEO") {
        // std::cout << "Creating MidTermTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc = new HighMoveTheoCalculator(
            key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_ + (count * end_theo_diff),
            aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create MidTerm Theo
      } else if (theo_type == "MASTER_THEO") {
        // std::cout << "Creating MasterTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc =
            new MasterTheoCalculator(key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_,
                                     _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Master Theo
      } else if (theo_type == "CORR_THEO") {
        // std::cout << "Creating MasterTheo starttime " << _trading_start_utc_mfm_ << " endtime " <<
        // _trading_end_utc_mfm_ + (count*end_theo_diff) << std::endl;
        BaseTheoCalculator* theo_calc =
            new CorrTheoCalculator(key_val_map, watch_, dbglogger_, _trading_start_utc_mfm_,
                                     _trading_end_utc_mfm_ + (count * end_theo_diff), aggressive_get_flat_mfm_);
        theo_vec_.push_back(theo_calc);
        theo_map_[(*key_val_map)["THEO_IDENTIFIER"]] = theo_calc;
        // Create Master Theo
      } else {
        std::cerr << __func__ << " THEO_TYPE not supported " << theo_type << std::endl;
        exit(-1);
      }
    } else {
      std::cerr << "THEO_TYPE missing in MainConfig! cant construct theo" << std::endl;
      exit(-1);
    }
    ++count;
  }
}

int main(int argc, char** argv) {
  // Assume we get a file source list and we have to print Market Updates
  // Arg1 : Strat File
  // Arg2 : Trading date
  // Arg3 : start time
  // Arg4 : end  time
  // Arg5 : Prog ID
  // Arg6(Optional) : Exchange

  if (argc < 6) {  // 5 is min of live and sim
    std::cerr
        << "expecting :\n"
        << " $tradeengineexec STRAT_LIVE_FILE TRADINGDATE START_TIME END_TIME PROGID [ADD_DBG_CODE CODE1 CODE2 ......] --exchange=[NSE]/BSE/BSE_NSE"
        << '\n';
    HFSAT::ExitVerbose(HFSAT::kTradeInitCommandLineLessArgs);
  }

  tradingdate_ = atoi(argv[2]);

  int start_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[3] + 4), argv[3]);
  int end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(argv[4] + 4), argv[4]);

  std::string _live_file = argv[1];
  int progid_ = atoi(argv[5]);
  std::vector<std::string> dbg_code_vec = {"FILL_TIME_INFO"};
  std::string exchange_ = "NSE";
  
  //Unused as of now, so keeping disabled permanently 
  is_asm_filter_enable_ = false;

  if (argc > 6) {
    if (strcmp(argv[6], "ADD_DBG_CODE") == 0) {
      for (int i = 7; i < argc; i++) {
        dbg_code_vec.push_back(std::string(argv[i]));
      }
    } 
  }

  boost::program_options::options_description desc("Allowed Options");
  desc.add_options()("help", "produce help message.")("preptime", boost::program_options::value<std::string>()->default_value("0"))("exchange", boost::program_options::value<std::string>()->default_value("NSE"));

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  std::string prep_time = vm["preptime"].as<std::string>();
  int prep_time_int = atoi(prep_time.c_str());
  std::string exchange_flag = vm["exchange"].as<std::string>();
  exchange_ = exchange_flag;


  int seek_process_start_utc_hhmm_ = start_utc_hhmm_;
  {
    seek_process_start_utc_hhmm_ =
        ((seek_process_start_utc_hhmm_ / 100) * 60) + (seek_process_start_utc_hhmm_ % 100);
    seek_process_start_utc_hhmm_ = std::max(0, seek_process_start_utc_hhmm_ - prep_time_int);
    seek_process_start_utc_hhmm_ =
        (seek_process_start_utc_hhmm_ % 60) + ((seek_process_start_utc_hhmm_ / 60) * 100);
  }

  HFSAT::ttime_t seek_time_after_prep_time = HFSAT::ttime_t(HFSAT::DateTime::GetTimeUTC(tradingdate_, seek_process_start_utc_hhmm_), 0);

  int day_end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, 1530, "IST_1530");
  if (strcmp(exchange_.c_str(), "NSE") == 0 || strcmp(exchange_.c_str(), "BSE") == 0 || strcmp(exchange_.c_str(), "BSE_NSE") == 0 ) {
    if (end_utc_hhmm_ > 959) {
      end_utc_hhmm_ = 959;
    }
  } else if (strcmp(exchange_.c_str(), "BMF") == 0) {
    day_end_utc_hhmm_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, 1755, "BRT_1755");
  }

  bool livetrading_ = false;
  bool ignore_user_msg_ = true;
  // std::vector<std::string> dbg_code_vec = {"OM_INFO","SIM_ORDER_INFO","ORS_DATA_INFO"};
  HFSAT::BulkFileWriter trades_writer_(256 * 1024);
  UserControlManager* user_control_manager_;
  PortfolioRiskManager* risk_manager_;

  std::map<std::string, std::string> _live_file_key_val_map;
  Parser::ParseConfig(_live_file, _live_file_key_val_map);

  std::string live_folder_name_ = ".";

  const size_t last_slash_idx = _live_file.rfind('/');
  if (std::string::npos != last_slash_idx) {
    live_folder_name_ = _live_file.substr(0, last_slash_idx);
  }

  HFSAT::TradingLocation_t dep_trading_location_ = HFSAT::kTLocNSE;

  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  //Must be called before any further exchange specific calls
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).SetExchangeType(exchange_);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadSecurityDefinitions();

  dep_trading_location_ = ("BSE" == exchange_ || "BSE_NSE" == exchange_) ? HFSAT::kTLocBSE : HFSAT::kTLocNSE;

  //LOAD ASM symbols
  std::vector<std::string> asm_symbols_list_;
  if(is_asm_filter_enable_){
    std::string asm_filename_ = "/spare/local/tradeinfo/" + exchange_ + "_Files/ASMSecurities/short_term_asm.csv_" + std::string(argv[2]);
    ifstream asm_file_(asm_filename_, ifstream::in);
    if(asm_file_.is_open()){
      std::string shortcode_;
      while(asm_file_.good()){
        getline(asm_file_, shortcode_);
        if(shortcode_.length() >0){
          asm_symbols_list_.push_back(exchange_+"_"+shortcode_);
        }
      }
    }else{
      std::cerr << "ASM_FILE: " << asm_filename_ << " is not READABLE. EXITING ............\n";
    }
  }

  std::vector<std::string> source_shortcode_list_;
  std::vector<std::string> ors_shortcode_list_;
  VEC_KEY_VAL_MAP vec_key_val_map_;
  std::string logs_directory = "/spare/local/logs/tradelogs";

  // Setup Logger
  InitDbglogger(tradingdate_, progid_, dbg_code_vec, logs_directory, livetrading_);
  CreateShortcodeLists(_live_file_key_val_map, source_shortcode_list_, ors_shortcode_list_, vec_key_val_map_,
                       live_folder_name_, asm_symbols_list_);

  for ( auto i : ors_shortcode_list_) std::cout<< i <<std::endl;

  int trading_start_utc_mfm_ =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, start_utc_hhmm_, "UTC_");
  int trading_end_utc_mfm_ = HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, end_utc_hhmm_, "UTC_");
  int aggressive_get_flat_mfm_ =
      HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, day_end_utc_hhmm_, "UTC_") - 30000;

  CommonInitializer* common_intializer =
      new CommonInitializer(source_shortcode_list_, ors_shortcode_list_, tradingdate_, dbglogger_, dep_trading_location_, livetrading_);
  InitTradesLogger(tradingdate_, progid_, livetrading_, logs_directory, trades_writer_);

  common_intializer->SetStartEndTime(start_utc_hhmm_, end_utc_hhmm_);
  common_intializer->SetRuntimeID(progid_);

  // Initialize the smv source after setting the required variables
  common_intializer->Initialize();
  std::vector<HFSAT::SecurityMarketView*>& p_smv_vec_ = common_intializer->getSMVMap();
  HFSAT::Watch& watch_ = common_intializer->getWatch();
  HFSAT::BroadcastManagerSim *bcast_manager_ = HFSAT::BroadcastManagerSim::GetUniqueInstance(
    dbglogger_, watch_, progid_);
  bcast_manager_->SetId(progid_);

  /*	for (auto scode : source_shortcode_list_) {
                  std::cout << " Source shortcode " << scode << std::endl;
          }

          for (auto ocode : ors_shortcode_list_) {
                  std::cout << " ORS shortcode " << ocode << std::endl;
          }*/
  
  HFSAT::SecurityNameIndexer& sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();
  // Get the smv and watch after creating the source

  std::vector<BaseTheoCalculator*> theo_vec_;
  std::vector<BaseTheoCalculator*> sqoff_theo_vec_;
  std::map<std::string, BaseTheoCalculator*> theo_map_;
  std::map<int, std::deque<SquareOffTheoCalculator*> > sqoff_theo_map_;
  int start_window_msec = 0;
  int end_window_msec = 0;
  int eff_squareoff_start_utc_mfm_ = 0;
  double bid_multiplier_ = 1;
  double ask_multiplier_ = 1;
  if (_live_file_key_val_map.find("START_TRADING_WINDOW") != _live_file_key_val_map.end()) {
    start_window_msec = std::atoi(_live_file_key_val_map["START_TRADING_WINDOW"].c_str());
  }
  if (_live_file_key_val_map.find("SQUAREOFF_START_WINDOW") != _live_file_key_val_map.end()) {
    end_window_msec = std::atoi(_live_file_key_val_map["SQUAREOFF_START_WINDOW"].c_str());
  }

  if (_live_file_key_val_map.find("EFF_SQUAREOFF_START_TIME") != _live_file_key_val_map.end()) {
    char* eff_squareoff_time_ = new char[strlen(_live_file_key_val_map["EFF_SQUAREOFF_START_TIME"].c_str()) + 1];
    std::strcpy(eff_squareoff_time_, _live_file_key_val_map["EFF_SQUAREOFF_START_TIME"].c_str());
    int eff_squareoff_start_utc_hhmm_ =
        HFSAT::DateTime::GetUTCHHMMFromTZHHMM(tradingdate_, atoi(eff_squareoff_time_ + 4), eff_squareoff_time_);
    eff_squareoff_start_utc_mfm_ =
        HFSAT::DateTime::GetUTCMsecsFromMidnightFromTZHHMM(tradingdate_, eff_squareoff_start_utc_hhmm_, "UTC_");
  }

  if (_live_file_key_val_map.find("BID_MULTIPLIER") != _live_file_key_val_map.end()) {
    bid_multiplier_ = std::atof(_live_file_key_val_map["BID_MULTIPLIER"].c_str());
  }

  if (_live_file_key_val_map.find("ASK_MULTIPLIER") != _live_file_key_val_map.end()) {
    ask_multiplier_ = std::atof(_live_file_key_val_map["ASK_MULTIPLIER"].c_str());
  }
  // std::cout << "Eff squareoff " << eff_squareoff_start_utc_mfm_ << std::endl;
  // std::cout << bid_multiplier_ << " " << ask_multiplier_<<std::endl;
  CreateTheoCalculators(vec_key_val_map_, theo_vec_, sqoff_theo_vec_, theo_map_, sqoff_theo_map_, watch_, dbglogger_,
                        trading_start_utc_mfm_, start_window_msec, trading_end_utc_mfm_, end_window_msec,
                        aggressive_get_flat_mfm_, eff_squareoff_start_utc_mfm_, bid_multiplier_, ask_multiplier_);
  std::vector<HFSAT::BaseSimMarketMaker*>& sid_to_smm_map_ = common_intializer->getSMMMap();
  int runtime_id_ = progid_;

  std::string pos_file_tag_ = _live_file_key_val_map["POSLIMIT"];
  std::string position_file_ = live_folder_name_ + "/" + pos_file_tag_;

  int prev_date_ = HelperFunctions::GetPrevDayforSHC("NSE_NIFTY_FUT0",tradingdate_);
  std::string dated_position_filename_ = live_folder_name_ + "/PosLimits_bkp/PositionLimits." + std::to_string(prev_date_) ;
  ifstream dated_position_file_(dated_position_filename_, ifstream::in);
  if(dated_position_file_.is_open()){
    position_file_ = dated_position_filename_;
  }

  std::map<std::string, std::string> pos_key_val_map_;
  Parser::ParseConfig(position_file_, pos_key_val_map_);

  std::map<std::string, HFSAT::ExternalDataListener*>& _shortcode_filesource_map_ =
      common_intializer->getShcToFileSrcMap();
  HFSAT::IndexedNSEMarketViewManager2* indexed_nse_market_view_manager =
     common_intializer->indexed_nse_market_view_manager();
  HFSAT::IndexedBSEMarketViewManager2* indexed_bse_market_view_manager = 
      common_intializer->indexed_bse_market_view_manager();

  for (auto theo_calc_ : theo_vec_) {
    theo_calc_->SetRuntimeID(runtime_id_);
    int sec_id_ = theo_calc_->GetSecondaryID();
    bool is_modify_before_confirmation_ = theo_calc_->IsOnFlyModifyAllowed();
    bool is_cancellable_before_confirmation_ = theo_calc_->IsOnFlyCancelAllowed();
    bool is_sqoff_needed_ =
        theo_calc_->IsSecondarySqOffNeeded();  // In case hedge is already there but we also need a squareoff of primary
                                               // in case of remaining positions
    theo_calc_->ConfigureHedgeDetails(theo_map_);
    if ("MASTER_THEO" == theo_calc_->GetTheoType()) {
      MasterTheoCalculator* master_theo_calc_ = dynamic_cast<MasterTheoCalculator*>(theo_calc_);
      master_theo_calc_->ConfigureMidTermDetails(theo_map_);
    }
    else if ("CORR_THEO" == theo_calc_->GetTheoType()) {
      CorrTheoCalculator* corr_theo_calc_ = dynamic_cast<CorrTheoCalculator*>(theo_calc_);
      corr_theo_calc_->ConfigureMidTermDetails(theo_map_);
    }
    SquareOffTheoCalculator* sqoff_theo_calc_ = NULL;
    if ((!theo_calc_->IsNeedToHedge()) || (is_sqoff_needed_)) {
      if ((sqoff_theo_map_.find(sec_id_) != sqoff_theo_map_.end() && (sqoff_theo_map_[sec_id_].size() > 0))) {

        sqoff_theo_calc_ = sqoff_theo_map_[sec_id_].front();
        sqoff_theo_map_[sec_id_].pop_front();
        theo_calc_->SetSquareOffTheo(sqoff_theo_calc_);
      }
      if (!sqoff_theo_calc_) {
        std::cerr << "SQUAREOFF THEO not give for secID " << sec_id_ << std::endl;
        exit(-1);
      }
    }

    HFSAT::BaseSimMarketMaker* smm_ = sid_to_smm_map_[sec_id_];
    auto dep_smv_ = p_smv_vec_[sec_id_];
    assert(dep_smv_ != nullptr && smm_ != nullptr);
    std::string trade_info = "nullptr";
    HFSAT::BaseTrader* base_trader_ = HFSAT::SimTraderHelper::GetSimTrader(trade_info, smm_);
    if (strcmp(exchange_.c_str(), "NSE") == 0) {
      if (theo_calc_->IsBigTradesListener()) {
        indexed_nse_market_view_manager->AddBigTradesListener(theo_calc_, theo_calc_->GetPrimaryID());
      }
    }
    else if (strcmp(exchange_.c_str(), "BSE") == 0 || strcmp(exchange_.c_str(), "BSE_NSE") == 0 ) {
      if (theo_calc_->IsBigTradesListener()) {
        indexed_bse_market_view_manager->AddBigTradesListener(theo_calc_, theo_calc_->GetPrimaryID());
      }
    }
    auto basic_om_ = new HFSAT::BasicOrderManager(dbglogger_, watch_, sec_name_indexer_, *base_trader_, *dep_smv_, 1,
                                                  pos_key_val_map_, livetrading_, is_modify_before_confirmation_,
                                                  is_cancellable_before_confirmation_);

    if (!basic_om_) {
      std::cerr << "Not able to create order manager for " << theo_calc_->GetSecondaryShc() << std::endl;
      exit(-1);
    }

    int saci = basic_om_->server_assigned_client_id_;

    for (auto filesource_iter_ = _shortcode_filesource_map_.begin();
         filesource_iter_ != _shortcode_filesource_map_.end(); filesource_iter_++) {
      filesource_iter_->second->AddExternalDataListenerListener(smm_);
    }

    for (auto shc : source_shortcode_list_) {
      int sec_id_ = sec_name_indexer_.GetIdFromString(shc);
      smm_->AddSecIdToSACI(saci, sec_id_);
    }

    basic_om_->AddExecutionListener(theo_calc_);
    HFSAT::BasePNL* sim_base_pnl = nullptr;
    sim_base_pnl = new HFSAT::SimBasePNL(dbglogger_, watch_, *dep_smv_, theo_calc_->GetRuntimeID(), trades_writer_);

    basic_om_->SetBasePNL(sim_base_pnl);
    // sim_base_pnl->AddListener(0, theo_calc_);

    if ((smm_ != nullptr) && (basic_om_ != nullptr)) {
      smm_->AddOrderNotFoundListener(basic_om_);
      // smm_->AddOrderSequencedListener(basic_om_);
      smm_->AddOrderConfirmedListener(basic_om_);
      smm_->AddOrderConfCxlReplaceRejectedListener(basic_om_);
      smm_->AddOrderConfCxlReplacedListener(basic_om_);
      smm_->AddOrderCanceledListener(basic_om_);
      smm_->AddOrderExecutedListener(basic_om_);
      smm_->AddOrderRejectedListener(basic_om_);
    }

    theo_calc_->SetBasePNL(sim_base_pnl);
    theo_calc_->SetupPNLHooks();
    theo_calc_->CreateAllExecutioners(basic_om_, livetrading_);
    theo_calc_->SetOMSubscriptions();
    // TODO Create new om for Square off theo
    if (sqoff_theo_calc_) {
      sqoff_theo_calc_->CreateAllExecutioners(basic_om_, livetrading_);
    }

    runtime_id_++;
  }

  if("BSE" == exchange_){
    HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_); 
  }else{
    HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(tradingdate_);
  }

  HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).SetTradingDate(tradingdate_);

  if(prep_time != "0"){
    common_intializer->SetToActualSeek(true);
    common_intializer->Seek(seek_time_after_prep_time);
  }else{
    common_intializer->Seek();
  }

  risk_manager_ = new PortfolioRiskManager(dbglogger_, watch_, theo_vec_, sqoff_theo_vec_, pos_key_val_map_);
  user_control_manager_ =
      new UserControlManager(dbglogger_, watch_, theo_vec_, sqoff_theo_vec_, position_file_, risk_manager_);
  bool control_source_found_ = false;
  if (false && !ignore_user_msg_) {
    HFSAT::ControlMessageFileSource* control_messasge_filesource_ = nullptr;
    bool control_file_present = true;
    control_messasge_filesource_ = new HFSAT::ControlMessageFileSource(
        dbglogger_, sec_name_indexer_, tradingdate_, 0, sec_name_indexer_.GetSecurityNameFromId(0),
        common_intializer->getDepTradingLocation(), progid_, control_file_present);

    if (control_messasge_filesource_ != nullptr && control_file_present) {
      HFSAT::HistoricalDispatcher& historical_dispatcher_ = common_intializer->getHistoricalDispatcher();
      historical_dispatcher_.AddExternalDataListener(control_messasge_filesource_);
      control_messasge_filesource_->SetExternalTimeListener(&watch_);
      control_messasge_filesource_->AddControlMessageListener(user_control_manager_);
    }
  }
  if (!control_source_found_) {
    for (auto theo_calc_ : theo_vec_) {
      theo_calc_->StartTrading();
    }
  }


  // Setting end time to run 4 minutes after strat stop time in order for it to getflat
  HFSAT::ttime_t end_time_ =
      HFSAT::ttime_t(HFSAT::DateTime::GetTimeFromTZHHMMSS(tradingdate_, end_utc_hhmm_ * 100, "UTC_"), 0) +
      HFSAT::ttime_t(900, 0);

  HFSAT::ttime_t day_end_time_ =
      HFSAT::ttime_t(HFSAT::DateTime::GetTimeFromTZHHMMSS(tradingdate_, day_end_utc_hhmm_ * 100, "UTC_"), 0);
  if (end_time_ > day_end_time_) {
    end_time_ = day_end_time_;
  }
  common_intializer->Run(end_time_);
  dbglogger_ << "END OF RUN" << DBGLOG_ENDL_FLUSH;
  int sum_pnl_ = 0;
  int total_orders_ = 0;
  double sum_net_exp_ = 0;
  double sum_gross_exp_ = 0;
  double sum_total_traded_value_ = 0;
  double sum_total_traded_value_nohedge_ = 0;
  double sum_total_traded_mkt_quantity_nohedge_ = 0;
  for (auto theo_calc_ : theo_vec_) {
    theo_calc_->PNLStats(&trades_writer_);
    sum_pnl_ += theo_calc_->GetPNL();
    total_orders_ += theo_calc_->GetTotalOrderCount();
    sum_net_exp_ += theo_calc_->GetExposure();
    sum_gross_exp_ += std::abs(theo_calc_->GetExposure());
    sum_total_traded_value_ += theo_calc_->GetTTV();
    if (!theo_calc_->IsParentTheoPresent()) {
      sum_total_traded_value_nohedge_ += theo_calc_->GetTTV();
      sum_total_traded_mkt_quantity_nohedge_ += theo_calc_->GetTotalTradedTTV();
    }
  }
  double mkt_volume_traded_ = (sum_total_traded_mkt_quantity_nohedge_ != 0)
                                  ? (sum_total_traded_value_nohedge_ / sum_total_traded_mkt_quantity_nohedge_) * 100
                                  : 0;
  dbglogger_ << "PORTFOLIO PNL: " << sum_pnl_ << " NETEXP: " << sum_net_exp_ << " GROSSEXP: " << sum_gross_exp_
             << " TTV: " << sum_total_traded_value_ << " NUM_ORDERS: " << total_orders_ << "\n"
             << DBGLOG_ENDL_FLUSH;
  // std::cerr << "Hope you made money!" << std::endl;
  trades_writer_ << "TOTALRESULT ALL " << sum_pnl_ << " " << sum_gross_exp_ << " 0 0 " << mkt_volume_traded_ << "  0 "
                 << sum_net_exp_ << " " << sum_total_traded_value_ / 10000000 << " 0 " << total_orders_ << "\n";
  std::cout << "TOTALRESULT ALL " << sum_pnl_ << " " << sum_gross_exp_ << " 0 0 " << mkt_volume_traded_ << " 0 "
            << sum_net_exp_ << " " << sum_total_traded_value_ / 10000000 << " 0 " << total_orders_ << std::endl;
  return 0;
}
