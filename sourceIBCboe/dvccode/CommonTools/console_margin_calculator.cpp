#include "dvccode/SpanMargin/span_calculator.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <dirent.h>


void console_input(std::string sc, int position, HFSAT::SpanCalculator& margin, HFSAT::SecurityNameIndexer &sec_name_indexer_) {
  int sec_id = sec_name_indexer_.GetIdFromString(sc);
  if (-1 != sec_id)
    margin.AddPosition(sec_id, position, 0);
}

void read_from_strat_file(std::string file_name,  HFSAT::SpanCalculator &margin, HFSAT::SecurityNameIndexer &sec_name_indexer_) {
  std::ifstream file;
  file.open(file_name, std::ios::in);

  if( !file )
    std::cerr << "Cant open " << file_name << std::endl;

  std::string line;
  
  while(std::getline(file, line)) {
    std::stringstream ss(line);
    std::string dummy;
    std::string sc = "";
    std::string time_;
    std::string buysell;
    int position = 0;
    int t_positions = 0;
    double trade_price = 0;

    if (ss >> time_ >> dummy >> sc >> buysell >> position >> trade_price >> t_positions) {
      if (buysell == "S")
        position *= -1;
      
      std::stringstream ss_sc(sc);
      std::getline(ss_sc, dummy, '.');
      sc = dummy;

      int sec_id = sec_name_indexer_.GetIdFromString(sc);

      if (-1 != sec_id) {
        margin.AddPosition(sec_id, position, trade_price);
      }
      else
        std::cerr << "sec id not present for " << sc << "\n";
    }
  }
}

void read_from_sc_pos_file(std::string file_name, HFSAT::SpanCalculator& margin, HFSAT::SecurityNameIndexer &sec_name_indexer_) {
  std::ifstream file;
  file.open(file_name, std::ios::in);

  if( !file )
    std::cerr << "Cant open " << file_name << std::endl;

  std::string line;
  
  while(std::getline(file, line)) {
    std::stringstream ss(line);
    std::string sc = "";
    std::string dummy;
    int position = 0;
    
    if (ss >> sc >> position) {
      std::stringstream ss_sc(sc);
      std::getline(ss_sc, dummy, '.');
      sc = dummy;

      int sec_id = sec_name_indexer_.GetIdFromString(sc);
      if (-1 != sec_id)
        margin.AddPosition(sec_id, position, 0); // trade_price = 0 for now. need to fix this      
    }
  }
}


/*
// void get_positions(HFSAT::SPANMargin& margin) {
//   margin.getPositions();
// }

*/
int main(int argc, char** argv) {
  // Check if there are enough parameters
  if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " <tradingdate> <shortcode_list>" << std::endl;
      return 1; // indicate error
  }

  // cli arguments to the exec.  
  int tradingdate_ = atoi(argv[1]);
  std::string filename = argv[2];
  std::string exchange = argv[3];

  HFSAT::CpucycleProfiler::SetUniqueInstance(1);
  HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "AddPosition");


  // Creating SecNameIndexer Object using shortcode list file.
  std::ifstream inputFile(filename);
  std::vector<std::string> source_shortcode_vec_;

  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).SetExchangeType(exchange);
  HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_).LoadSecurityDefinitions();
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

  HFSAT::SecurityNameIndexer &sec_name_indexer_ = HFSAT::SecurityNameIndexer::GetUniqueInstance();

  if (!inputFile.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return 1;
  }

  // Start reading the file.
  std::string input_shortcode = "";
  while (inputFile >> input_shortcode) {
    if (!sec_name_indexer_.HasString(input_shortcode)) {
      char const* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(input_shortcode);
      sec_name_indexer_.AddString(exchange_symbol_, input_shortcode);
    }
  }

  HFSAT::SpanCalculator margin(tradingdate_, sec_name_indexer_, exchange);

  while(true) {
    int in;
    std::cout << "1. Strat trade file" << std::endl
              << "2. Strat trade files directory" << std::endl
              << "3. Shortcode & position" << std::endl
              << "4. Shortcode & position file" << std::endl
              << "5. Get Margin" << std::endl
              << "6. Get Positions" << std::endl
              << "7. Clear positions map" << std::endl
              << "8. Exit" << std::endl;

    std::cin >> in;

    switch(in)  {
      // Using strat file.
      case 1: {
        std::string filename;
        std::cout << "Filepath: ";
        std::cin >> filename;

        read_from_strat_file(filename, margin, sec_name_indexer_);
      } break;

      case 2: {
        std::string filename;
        std::cout << "Filepath: ";
        std::cin >> filename;

        read_from_strat_file(filename, margin, sec_name_indexer_);
      } break;

      // Single shortcode and position.
      case 3: {
        std::string sc;
        int position = 0;

        std::cout << "shortcode: ";
        std::cin >> sc;

        std::cout << "position: ";
        std::cin >> position;

        console_input(sc, position, margin, sec_name_indexer_);

      } break;

      // Using shortcode position file.
      case 4: {
        std::string filename;
        std::cout << "Filepath: ";
        std::cin >> filename;

        read_from_sc_pos_file(filename, margin, sec_name_indexer_);
      } break;

      // Get margin.
      case 5: {
        std::cout << "Margin: "
                  << (margin.GetMargin()/10000000.00)
                  << std::endl;
        std::cout << "=====================================================================\n";
        std::cout << margin.GetMarginD().str() << "\n";
        std::cout << "=====================================================================\n";
      } break;

      // Get positions.
      case 6: {
        std::cout << "========== Positions ==========\n";
        margin.GetPositionsVec();
        std::cout << "===============================\n";
      } break;

      // Clear positions map.
      case 7: {
        // margin.clearPositionsMap();
      } break;

      // Exit.
      case 8: {
        return 0;
      } break;

      default:
        break;
    }
    std::cout << std::endl;
  }
  return 0;
}
  // DIR* dir;
  // struct dirent* entry;
  // if ((dir = opendir(directory_path.c_str())) != nullptr) {
  //   while ((entry = readdir(dir)) != nullptr) {
  //     if (entry->d_type == DT_REG) {
  //       std::string filename = entry->d_name;
  //       std::string trade_file_path = directory_path + "/" + filename;
  //       read_from_sc_pos_file(trade_file_path, margin, sec_name_indexer_);
  //       std::istringstream iss(trade_file_path);
  //       std::string word;
  //       while (getline(iss, word, '/')) {}
  //       std::istringstream word_stream(word);
  //       std::string id;
  //       while (getline(word_stream, id, '.')) {};
  //       double margin_ = margin.GetMargin()/10000000.00;
  //       if (margin_ < 0)
  //         margin_ = 0;
  //       std::cout << id << " " << margin_ << "\n";
  //       margin.Clear();
  //       // margin.GetMarginD();
  //       // std::cout << "-----------------------------------\n";
  //     }
  //   }
  // }

//   return 0;
// }