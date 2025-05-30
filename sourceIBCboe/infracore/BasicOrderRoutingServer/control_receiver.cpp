#include "infracore/BasicOrderRoutingServer/control_receiver.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/IBUtils/ComboProductHandler.hpp"
#include "dvccode/Utils/get_todays_date_utc.hpp"
#include "dvccode/IBUtils/AddUpdateComboProductMap.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include <sstream>
#include <fstream>

namespace HFSAT {
namespace ORS {

// Static variable definitions
std::unordered_map<std::string, std::string> 
    ControlReceiver::combinedSymbToUniqueId;

std::unordered_map<std::string,std::string> 
    ControlReceiver::uniqueIdToCombinedSymb;
// std::string comboProductMap_;
HFSAT::Lock ControlReceiver::mapMutexForCombinedSymb;

ControlReceiver::ControlReceiver(DebugLogger& _dbglogger_, Settings& _settings_,
                                 HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_,
                                 std::string t_output_log_dir_, bool &_cancel_live_order_flag_)
    : dbglogger_(_dbglogger_),
      tcp_server_socket_(atoi(_settings_.getValue("Control_Port").c_str())),
      settings_(_settings_),
      cancel_live_order_flag_(_cancel_live_order_flag_),
      use_affinity_((_settings_.has("UseAffinity") && _settings_.getValue("UseAffinity") == "Y")),
      is_addts_thread_running_(new bool(false)),
      client_logging_segment_initializer_(_client_logging_segment_initializer_),
      output_log_dir_(t_output_log_dir_) {
  LoadComboProductsMapFile();
  LoadMarginFile();
}

ControlReceiver::~ControlReceiver() {
  tcp_server_socket_.Close();

  // Cleanup
  // Also we want to free the remaining control thread since recv. is dead
  delete is_addts_thread_running_;
  for (unsigned int i = 0; i < active_control_threads_.size(); i++) {
    if (active_control_threads_[i] != NULL) {
      delete active_control_threads_[i];
      active_control_threads_[i] = NULL;
    }
  }

  active_control_threads_.empty();
  active_control_threads_marking_.empty();
}

void ControlReceiver::LoadComboProductsMapFile(){
  // std::string 
  std::string tradingdate_ = DateUtility::getTodayDateUTC();
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/files/tmp/comboProductMap." << tradingdate_;
  std::string comboProductMap_ = t_temp_oss_.str();

  // Check if the file exists
  std::ifstream infile_check(comboProductMap_);
  if (!infile_check.good()) {
    // File does not exist, create it
    std::ofstream outfile(comboProductMap_);
    if (!outfile) {
        std::cerr << "Failed to create file: " << comboProductMap_ << std::endl;
        exit(-1);

    }
    std::cout << "File created: " << comboProductMap_ << std::endl;
    outfile.close();
  }
  infile_check.close();

  // Open the file for reading
  std::ifstream infile(comboProductMap_);
  if (!infile) {
      std::cerr << "Failed to open file: " << comboProductMap_ << std::endl;
      exit(-1);
  }

  std::string line;
  while (std::getline(infile, line)) {
    std::istringstream iss(line);
    std::string comboSymb, uniqueId;

    // Use ',' as the delimiter
    if (std::getline(iss, comboSymb, ',') && std::getline(iss, uniqueId)) {
      ControlReceiver::combinedSymbToUniqueId[comboSymb] = uniqueId;
      ControlReceiver::uniqueIdToCombinedSymb[uniqueId] = comboSymb;
      HFSAT::IBUtils::AddUpdateComboProductMap::updateTheInternalMaps(comboSymb,uniqueId,HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate());
      // addMaxPosAndOrdSzForComboProductCBOE(uniqueId,2,2);
    }
  }

  infile.close();

}
void ControlReceiver::addMaxPosAndOrdSzForComboProductCBOE(const std::string &shortcodeOfDay,const int max_pos,const int max_ord_sz){
  sec_to_max_pos_map_[shortcodeOfDay] = max_pos;
  sec_to_max_ord_sz_map_[shortcodeOfDay] = max_ord_sz; 
}
void ControlReceiver::LoadMarginFile() {
  std::vector<std::string> malformatted_lines;
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);

  std::string initial_margin_filename_;

  if (!strncmp(hostname, "SDV-HK-SRV", 10)) {
    if (!settings_.has("UseOverrideMarginFile")) {
      initial_margin_filename_ = "/home/newedge/limit.csv";
      dbglogger_ << "Using_default_limits_file  " << initial_margin_filename_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    } else {
      initial_margin_filename_ = settings_.getValue("UseOverrideMarginFile");
    }
  } else {
    std::ostringstream t_temp_oss;
    t_temp_oss << PROD_CONFIGS_DIR << "common_initial_margin_file.txt";

    initial_margin_filename_ = t_temp_oss.str();
  }

  std::ifstream initial_margin_file_;
  initial_margin_file_.open(initial_margin_filename_.c_str(), std::ifstream::in);
  if (!(initial_margin_file_.is_open())) {
    dbglogger_ << "Cannot open file " << initial_margin_filename_ << "\n";
    dbglogger_.DumpCurrentBuffer();
    exit(-1);
  }

  char line[1024];
  while (!initial_margin_file_.eof()) {
    bzero(line, 1024);
    initial_margin_file_.getline(line, 1024);
    if (strlen(line) == 0 || strstr(line, "#") != NULL) continue;
    std::string line_str = line;
    HFSAT::PerishableStringTokenizer st_(line, 1024);
    const std::vector<const char*>& tokens_ = st_.GetTokens();
    if (tokens_.size() != 4) {
      malformatted_lines.push_back(line_str);
      dbglogger_ << "Ignoring Malformatted line in file " << initial_margin_filename_.c_str() << "\n";
      dbglogger_.DumpCurrentBuffer();
    }

    // This sets up the limits on the positions for different instruments.
    // Uses the limit information provided by MFGlobal.
    if (tokens_.size() == 4)  {
      sec_to_max_pos_map_[tokens_[0]] = atoi(tokens_[2]);
      sec_to_max_ord_sz_map_[tokens_[0]] = atoi(tokens_[3]);
    }
  }

  if (malformatted_lines.size() > 0) {
    HFSAT::Email e;
    std::string subject = "Malformatted line in " + initial_margin_filename_ + " " + hostname;
    e.setSubject(subject);
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    for (auto i : malformatted_lines) {
      e.content_stream << i << "<br/>";
    }
    e.sendMail();
  }

  initial_margin_file_.close();
}
//It is assumed that the exchange and base product is same for all the combo products
std::string ControlReceiver::getThePrefixForComboProductId(const std::string firstShortCode){
  std::string prefix=""; //Exchange_Product_      CBOE_SPXW_

  std::istringstream ss(firstShortCode);
  std::string token="";
  // Get the exchange  CBOE
  if (!std::getline(ss, token, '_')) {
      throw std::invalid_argument("Invalid format: missing value for exchange " + firstShortCode);
  }
  prefix+=token+"_";
  // Get the product   SPXW
  if (!std::getline(ss, token, '_')) {
      throw std::invalid_argument("Invalid format: missing value for product " + firstShortCode);
  }
  prefix+=token+"_";
  prefix+="COMBO_";
  return prefix;

}
inline std::string ControlReceiver::convertToComboUniqueId(const int32_t id, const std::string prefix) {
    std::ostringstream oss;
    oss << prefix << id;
    return oss.str();
}

inline void ControlReceiver::AddEntryForComboProduct(const std::string& comboStringProduct, const std::string uniqueId){
  ControlReceiver::combinedSymbToUniqueId[comboStringProduct]=uniqueId;
  ControlReceiver::uniqueIdToCombinedSymb[uniqueId]=comboStringProduct;
}
//Will give ComboProduct for uniqueId
std::vector<std::pair<std::string,int32_t>> ControlReceiver::getComboSymbolForUniqueId(const std::string uniqueId){
  std::string comboString="";
  ControlReceiver::mapMutexForCombinedSymb.LockMutex();
  // Insert the key with default value if it doesn't exist otherwise get the existing one
  if(ControlReceiver::uniqueIdToCombinedSymb.find(uniqueId)!=ControlReceiver::uniqueIdToCombinedSymb.end()){
    comboString=ControlReceiver::uniqueIdToCombinedSymb[uniqueId];
  }else{
    std::cerr<<"This unique id: "<<uniqueId<< " does not have any corresponding Combined Product\n";
  }
  ControlReceiver::mapMutexForCombinedSymb.UnlockMutex();
  return ComboProductHandler::convertComboStringToVector(comboString);
}

//Will give unique id for Combo products make sure to send a normalized/sorted vector 
std::string ControlReceiver::getUniqueIdForComboSymbol(const std::vector<std::pair<std::string,int32_t>> comboProducts){
  std::string comboString=ComboProductHandler::convertComboVectorToString(comboProducts);
  std::string uniqueId="";
  ControlReceiver::mapMutexForCombinedSymb.LockMutex();
  //Check if this vector already has an id
  if(ControlReceiver::combinedSymbToUniqueId.find(comboString)!=ControlReceiver::combinedSymbToUniqueId.end())
    uniqueId=ControlReceiver::combinedSymbToUniqueId[comboString];
  else {
    // Insert the key with new value if it doesn't exist 
    ControlReceiver::combinedSymbToUniqueId[comboString]=ControlReceiver::convertToComboUniqueId(combinedSymbToUniqueId.size(),ControlReceiver::getThePrefixForComboProductId(comboProducts[0].first));
    uniqueId=ControlReceiver::combinedSymbToUniqueId[comboString];
    ControlReceiver::uniqueIdToCombinedSymb[uniqueId]=comboString;

    std::string tradingdate_ = DateUtility::getTodayDateUTC();
    
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/files/tmp/comboProductMap." << tradingdate_;
    std::string comboProductMap_ = t_temp_oss_.str();
    // Write the new key-value pair to the file
    std::ofstream outfile(comboProductMap_, std::ios::app); // Open in append mode
    if (!outfile) {
        std::cerr << "Failed to open file for writing: " << comboProductMap_ << std::endl;
        exit(-1);
    } else {
        outfile << comboString << "," << uniqueId << "\n";
        outfile.close();
    }
  }
  
  ControlReceiver::mapMutexForCombinedSymb.UnlockMutex();
  return uniqueId;
}

// Should be thread-safe
void ControlReceiver::RemoveThread(int32_t const& thread_id) { active_control_threads_marking_[thread_id] = false; }

void ControlReceiver::thread_main() {
  int32_t thread_id = 0;

  if (use_affinity_) {
    setName("ControlReceiver");
    //        AllocateCPUOrExit ();
  }
  // std::cout<<"Control Re ceiver thread started\n";

  while (int connected_socket_file_descriptor_ = tcp_server_socket_.Accept()) {
    if (connected_socket_file_descriptor_ < 0) {
      break;
    }

    for (auto& itr : active_control_threads_marking_) {
      if (false == itr.second) {
        if (NULL != active_control_threads_[itr.first]) {
          std::cout << " Deleting : " << itr.first << " " << std::endl;
          active_control_threads_[itr.first]->StopControlThread();
          std::cout << " Waiting to stop control thread..." << std::endl;
          active_control_threads_[itr.first]->stop();
          std::cout << " Stopped control thread for..." << itr.first << std::endl;
          // Doesn't make any difference anymore here, Will prevent further stop calls
          // TODO : Check Why should a stopped thread wait call matter
          itr.second = true;
          //          delete active_control_threads_[ itr.first ] ;
          //          active_control_threads_[ itr.first ] = NULL ;
        }
      }
    }

    // std::cout<<"Connection Accepted\n";
    ControlThread* _new_control_thread_ =
        new ControlThread(dbglogger_, connected_socket_file_descriptor_, settings_, client_logging_segment_initializer_,
                          output_log_dir_,cancel_live_order_flag_, sec_to_max_pos_map_, 
                          sec_to_max_ord_sz_map_, this, thread_id, is_addts_thread_running_);

    active_control_threads_[thread_id] = _new_control_thread_;
    active_control_threads_marking_[thread_id] = true;

    _new_control_thread_->run();  // starts the thread

    thread_id++;
  }
}

void ControlReceiver::StopControlThreads() {
  for (unsigned int i = 0; i < active_control_threads_.size(); i++) {
    active_control_threads_[i]->StopControlThread();
  }
}
}
}
