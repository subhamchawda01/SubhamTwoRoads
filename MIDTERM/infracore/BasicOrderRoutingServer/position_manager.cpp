/**
    \file BasicOrderRoutingServer/position_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */

#include <sstream>

#include "dvccode/CommonDataStructures/simple_security_symbol_indexer.hpp"  // Needed to perform position dump on crash/exit and again on startup

#include "infracore/BasicOrderRoutingServer/position_manager.hpp"

namespace HFSAT {
namespace ORS {

void EmailForPositionsLoaded(std::string _mail_body_) {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  HFSAT::Email e;

  e.setSubject("Positions Loaded by ORS at " + std::string(hostname));
  e.addRecepient("nseall@tworoads.co.in");
  e.addSender("nseall@tworoads.co.in");
  e.content_stream << "host_machine: " << hostname << "<br/>";
  e.content_stream << _mail_body_ << "<br/>";
  // e.content_stream << " <br/> Will not be loading any live orders <br/>" ;

  e.sendMail();
}

// A slightly modified version of the original DumpPMState --
// This outputs symbol names instead of security ids.
// This is used for position recovery on crash/exit and restart.
std::string PositionManager::DumpPMState() const {
  std::ostringstream temp_oss;

  // Need this to convert security ids to security symbols --
  HFSAT::SimpleSecuritySymbolIndexer &simple_security_symbol_indexer_ =
      HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance();

  temp_oss << "No. of securities: " << simple_security_symbol_indexer_.NumSecurityId() << "\n";
  temp_oss << "Security Position Map\n";
  for (unsigned int i = 0; i < simple_security_symbol_indexer_.NumSecurityId(); i++) {
    temp_oss << " Security_Id:" << simple_security_symbol_indexer_.GetSecuritySymbolFromId(i)
             << ":\tPosition:" << security_id_to_pos_info_[i].position << ":\n";
  }
  temp_oss << "\n";

  temp_oss << "Cumulative Bid Size Map\n";
  for (unsigned int i = 0; i < simple_security_symbol_indexer_.NumSecurityId(); i++) {
    temp_oss << " Security_Id:" << simple_security_symbol_indexer_.GetSecuritySymbolFromId(i)
             << ":\tPosition:" << security_id_to_pos_info_[i].bid_size << ":\n";
  }
  temp_oss << "\n";

  temp_oss << "Cumulative Ask Size Map\n";
  for (unsigned int i = 0; i < simple_security_symbol_indexer_.NumSecurityId(); i++) {
    temp_oss << " Security_Id:" << simple_security_symbol_indexer_.GetSecuritySymbolFromId(i)
             << ":\tPosition:" << security_id_to_pos_info_[i].ask_size << ":\n";
  }
  temp_oss << "\n";

  return temp_oss.str();
}

std::string PositionManager::DumpSecurityPositions() const {
  std::ostringstream temp_oss;
  HFSAT::SimpleSecuritySymbolIndexer &simple_security_symbol_indexer_ =
      HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance();

  temp_oss << "Security Position Map\n";
  for (unsigned int i = 0; i < simple_security_symbol_indexer_.NumSecurityId(); i++) {
    temp_oss << " Security_Id:" << simple_security_symbol_indexer_.GetSecuritySymbolFromId(i)
             << ":\tPosition:" << security_id_to_pos_info_[i].position << ":\n";
  }
  temp_oss << "\n\n";

  return temp_oss.str();
}

std::string PositionManager::DumpSACIPosition() const {
  std::ostringstream temp_oss;

  temp_oss << "SACI based Position \n";
  for (unsigned int i = 0; i < ORS_MAX_NUM_OF_CLIENTS; ++i) {
    if (saci_to_position_info_[i].position != 0) {
      int saci = base_writer_id_ + i;
      temp_oss << "SACI:" << saci << "\tPosition:" << saci_to_position_info_[i].position << "\n";
    }
  }

  return temp_oss.str();
}

// Setup the recovery file for the position manager to write to
void PositionManager::SetRecoveryFile(std::string _position_filename_) {
  position_filename_ = _position_filename_;

  // Load the last saved position, if available --
  std::ifstream position_ifstream_;
  position_ifstream_.open(position_filename_.c_str());

  char line_char_[1024], temp_line_char_[1024];
  int state = 1;  // 1 -> Update Position, 2 -> Update Bid Sizes, 3 -> Update Ask Sizes

  char *security_symbol_ = NULL;

  // We will need to setup the simple security symbol indexer to map the symbols read in
  HFSAT::SimpleSecuritySymbolIndexer &simple_security_symbol_indexer_ =
      HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance();
  std::ostringstream t_temp_oss;
  bool is_positions_loaded = false;

  while (position_ifstream_.good()) {
    position_ifstream_.getline(line_char_, 1023);

    if (strlen(line_char_) < 10) {  // Blank line
      continue;
    }

    if (!strncmp(line_char_, "No. of", 6)) {
      continue;
    }

    // Simply set the p_map_int_ pointer to point to the correct array.
    if (!strcmp(line_char_, "Security Position Map")) {
      state = 1;
      t_temp_oss << "<br/> Security Position Map <br/>";
      continue;
    } else if (!strcmp(line_char_, "Cumulative Bid Size Map")) {
      state = 2;
      continue;
    } else if (!strcmp(line_char_, "Cumulative Ask Size Map")) {
      state = 3;
      continue;
    } else if (!strcmp(line_char_, "Cumulative MiFid Values Map")) {
      state = 4;
      continue;
    }

    strcpy(temp_line_char_, line_char_);
    strtok(temp_line_char_, ":");  // Security_Id:

    // Must allocate memory.
    // simple_security_symbol_indexer DOES NOT allocate memory, it will simply use this address. --
    security_symbol_ = (char *)malloc(16 * sizeof(char));
    memset((void *)security_symbol_, 0, 16);

    strcpy(security_symbol_, strtok(NULL, ":\t\n"));  // Get the security symbol
    strtok(NULL, ":");                                // Position:
    int value_ = 0;

    if (state != 4) {
      value_ = atoi(strtok(NULL, ":"));  // Get the position
    }
    // Now we need to add this symbol to the simple_security_symbol_indexer_ map.
    simple_security_symbol_indexer_.AddString((const char *)security_symbol_);

    // Get the id that was assigned to this symbol and use it to index the map.
    const int security_id_ = simple_security_symbol_indexer_.GetIdFromSecname((const char *)security_symbol_);
    if (security_id_ >= 0) {
      switch (state) {
        case 1: {
          security_id_to_pos_info_[security_id_].position = value_;
          if (value_ != 0) {
            t_temp_oss << security_symbol_ << " --> " << value_ << "<br/>";
            is_positions_loaded = true;
          }
        } break;
        case 2: {
        } break;
        case 3: {
        } break;
        default: {
        } break;
      }
    }
    // free( security_symbol_);
  }

  position_ifstream_.close();

  if (is_positions_loaded) EmailForPositionsLoaded(t_temp_oss.str());
}

void PositionManager::SetBaseWriterId(int client_base) { base_writer_id_ = client_base << 16; }

// Write the position manager's state to the specified file, to be used for recovery at next startup --
void PositionManager::DumpPMRecovery() {
  std::string position_state_ = DumpPMState();

  // Used to log the position
  DebugLogger position_logger_(4 * 1024 * 1024, 5000);
  // Now log positions for this session --
  position_logger_.OpenLogFile(position_filename_.c_str(), std::ofstream::out);
  position_logger_ << position_state_;
  position_logger_.Close();
}

#if USING_STC
std::string PositionManager::DumpSelfTradeCheckState() const {
  std::ostringstream temp_oss;

  HFSAT::SimpleSecuritySymbolIndexer &simple_security_symbol_indexer_ =
      HFSAT::SimpleSecuritySymbolIndexer::GetUniqueInstance();

  temp_oss << " SELF TRADE CHECK No. of securities: " << simple_security_symbol_indexer_.NumSecurityId() << "\n";
  temp_oss << "SELF TRADE BID MAP  Map\n";
  for (unsigned int i = 0; i < simple_security_symbol_indexer_.NumSecurityId(); i++)  // --
  {
    temp_oss << " Security_Id:" << simple_security_symbol_indexer_.GetSecuritySymbolFromId(i)
             << ":\t SIDTOBIDPRICEMAP : " << security_id_to_bid_price_map_[i]
             << ":\t SIDTOBIDSIZEMAP : " << security_id_to_bid_size_map_[i] << ":\n";  // --
  }
  temp_oss << "\n";
  temp_oss << "SELF TRADE ASK MAP  Map\n";
  for (unsigned int i = 0; i < simple_security_symbol_indexer_.NumSecurityId(); i++)  // --
  {
    temp_oss << " Security_Id:" << simple_security_symbol_indexer_.GetSecuritySymbolFromId(i)
             << ":\t SIDTOASKPRICEMAP : " << security_id_to_ask_price_map_[i]
             << ":\t SIDTOASKSIZEMAP : " << security_id_to_ask_size_map_[i] << ":\n";  // --
  }
  temp_oss << "\n";

  temp_oss << "\n\n";

  return temp_oss.str();
}
#endif
}
}
