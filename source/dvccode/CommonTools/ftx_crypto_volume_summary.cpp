
#include <cstring>
#include <string>
#include <vector>
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

std::string dt = "";
std::vector < std::string > files_list_vec;

class ReadFXTCryptoTradeFile {

 private:
   std::map < std::string, std::map<uint64_t, std::string> > volume_per_day_map;
   std::map < std::string, uint64_t > volume_overall_map;
   std::map < std::string, uint64_t > date_volume_overall_map;
   uint64_t overall_volume;

 public: 
   ReadFXTCryptoTradeFile()
   : overall_volume(0) { }

  void PrintTopVolumeProducts(){
    for (auto per_day: volume_per_day_map) {
      std::map<uint64_t, std::string> temp_map = per_day.second;
      uint64_t total_volume_per_day_map = date_volume_overall_map[per_day.first];
      uint64_t sum_volume_per_day_map = 0 ;
      std::cout << per_day.first << " Total " << total_volume_per_day_map << std::endl;
      std::map<uint64_t, std::string>::reverse_iterator day_volume_itr;
      for (day_volume_itr = temp_map.rbegin(); day_volume_itr != temp_map.rend(); day_volume_itr++) {
        sum_volume_per_day_map += day_volume_itr->first;
        int volume_percent = sum_volume_per_day_map * 100 / total_volume_per_day_map;
        if (volume_percent <= 90) {
          std::cout << per_day.first << " " << day_volume_itr->second << " " << day_volume_itr->first << std::endl;
        }
      }
    }
    std::map < uint64_t, std::string > temp_volume_overall_map;
    std::cout << "Overall Total " << overall_volume << std::endl;
    for (auto temp_itr : volume_overall_map) {
      temp_volume_overall_map[temp_itr.second] = temp_itr.first;
    }
    std::map<uint64_t, std::string>::reverse_iterator overall_itr;
    uint64_t sum_volume_overall = 0;
    for (overall_itr = temp_volume_overall_map.rbegin(); overall_itr != temp_volume_overall_map.rend(); overall_itr++) {
      sum_volume_overall += overall_itr->first;
      int volume_percent = sum_volume_overall * 100 / overall_volume;
      if (volume_percent <= 90) {
        std::cout << "Overall " << overall_itr->second << " " << overall_itr->first << std::endl;
      }
    }
  }

   void ReadDumpTradeVolume() {

     for ( auto & itr : files_list_vec ) {
       std::string file_name = itr ;
       char temp_file_name[file_name.length() + 1];
       strcpy(temp_file_name, file_name.c_str());
       std::vector<char*> file_name_tokens_;
       // create a copy of line read before using non-const tokenizer

       HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(temp_file_name, "_", file_name_tokens_);
       int vec_size = file_name_tokens_.size();
       std::string file_date = file_name_tokens_[vec_size - 2];
       std::string product = file_name_tokens_[vec_size - 1];

       std::string trim_str = ".csv.gz";
       std::string::size_type temp_str_size = product.find(trim_str);

       if (temp_str_size != std::string::npos)
         product.erase(temp_str_size, trim_str.length());

       if (HFSAT::FileUtils::ExistsAndReadable(file_name)) {
         HFSAT::BulkFileReader trades_file_stream_;
         trades_file_stream_.open(file_name);
         char trades_readline_buffer_[2048];

         if (trades_file_stream_.is_open()) {
             uint64_t total_volume = 0;
             while (true) {
               memset(trades_readline_buffer_, 0, sizeof(trades_readline_buffer_));
               unsigned int trades_read_length = trades_file_stream_.GetLine(trades_readline_buffer_, sizeof(trades_readline_buffer_));
               if (trades_read_length <= 0) break ;
  
               std::vector<char*> trades_tokens_;
               // create a copy of line read before using non-const tokenizer
               char readline_buffer_copy_[2048];
               memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
               strcpy(readline_buffer_copy_, trades_readline_buffer_);
    
               HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", trades_tokens_);
               if (strcmp(trades_tokens_[0],"exchange") != 0 ) {
                 std::string trimmed_str_;
                 HFSAT::PerishableStringTokenizer::TrimString(trades_tokens_[7], trimmed_str_, '\n');
                 uint64_t total = std::stof(trades_tokens_[6]) * std::stof(trimmed_str_);
                 total_volume += total;
               } else { 
                 continue;
               }
             }
           trades_file_stream_.close();
           volume_per_day_map[file_date][total_volume] = product;
           date_volume_overall_map[file_date] += total_volume;
           volume_overall_map[product] += total_volume;
           overall_volume += total_volume;
        }
      }
    }
    PrintTopVolumeProducts();
  }

};


int main ( int argc, char *argv[] )
{

  if ( argc < 1 ) {
    std::cerr << "USAGE : <file/s>" << std::endl ;
    std::exit(-1);
  }

  for ( int32_t counter = 1 ; counter < argc ; counter++ ) {
    files_list_vec.push_back(argv[counter]) ;
  }

  dt = argv[1];
  ReadFXTCryptoTradeFile readcryptotradefiles;
  readcryptotradefiles.ReadDumpTradeVolume();

  return EXIT_SUCCESS;
}                       
