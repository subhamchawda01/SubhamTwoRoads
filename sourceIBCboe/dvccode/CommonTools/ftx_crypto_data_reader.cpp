
#include <string>
#include <vector>
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#define CRYPTO_DATA_FILE_DIR "/NAS1/data/CryptoData/ftx/"

typedef struct {

  double ask_price;
  double ask_size;
  double bid_price;
  double bid_size;

} AskBidPriceSize;

typedef struct {
  
  std::string product;
  uint64_t timestamp;
  double price;
  double size;
  std::string buy_sell;

} TradeData;


class ReadFXTCryptoFile {

 private:
   std::string trades_file_, book_file_;
   std::vector<AskBidPriceSize> ask_bid_book_vec_;
   TradeData trade_data;
   bool last_trade_flag;

 public: 
   ReadFXTCryptoFile(std::string trades_file, std::string book_file)
     : trades_file_(trades_file),
       book_file_(book_file),
       last_trade_flag(false) { }

   void ReadDumpTradeBook(uint32_t level_) {
     // create vector size of level_
     for ( uint16_t size_vec = 0 ; size_vec < level_ ; ++size_vec) {
       AskBidPriceSize inst_bid_ask_;
       inst_bid_ask_.ask_price = -1; 
       inst_bid_ask_.ask_size = -1;
       inst_bid_ask_.bid_price = -1;
       inst_bid_ask_.bid_size = -1; 
       ask_bid_book_vec_.push_back(inst_bid_ask_);
     }
     if (HFSAT::FileUtils::ExistsAndReadable(book_file_) && HFSAT::FileUtils::ExistsAndReadable(trades_file_)) {
       std::cout << book_file_ << " " << trades_file_ << std::endl;
       HFSAT::BulkFileReader bulk_file_reader ;
       HFSAT::BulkFileReader book_file_stream_, trades_file_stream_;
       book_file_stream_.open(book_file_);
       trades_file_stream_.open(trades_file_);
       char book_readline_buffer_[2048];
       char trades_readline_buffer_[2048];
       uint64_t book_timestamp = 0 ,trades_timestamp = 0;

       if (book_file_stream_.is_open() && trades_file_stream_.is_open()) {
         while (true) {
           memset(book_readline_buffer_, 0, sizeof(book_readline_buffer_));
           unsigned int book_read_length = book_file_stream_.GetLine(book_readline_buffer_, sizeof(book_readline_buffer_));
           if (book_read_length <= 0) break ;

           std::vector<char*> book_tokens_;
           // create a copy of line read before using non-const tokenizer
           char readline_buffer_copy_[2048];
           memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
           strcpy(readline_buffer_copy_, book_readline_buffer_);

           HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", book_tokens_);
           if (strcmp(book_tokens_[0],"exchange") != 0 ) {
             book_timestamp = std::atol(book_tokens_[2]);
             for (uint16_t field = 4 ,size_vec = 0; size_vec < level_; field += 4, ++size_vec) {
               ask_bid_book_vec_[size_vec].ask_price = std::atof(book_tokens_[field]);
               ask_bid_book_vec_[size_vec].ask_size = std::atof(book_tokens_[field + 1]);
               ask_bid_book_vec_[size_vec].bid_price = std::atof(book_tokens_[field + 2]);
               ask_bid_book_vec_[size_vec].bid_size = std::atof(book_tokens_[field + 3]);
             }
           }

           if (last_trade_flag) {
             std::cout << trade_data.product << " " << trade_data.timestamp << " OnTrade@ " << trade_data.price
                       << " of size " << trade_data.size << " " << trade_data.buy_sell << std::endl;
             last_trade_flag = false;
           }

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
               trades_timestamp = std::atol(trades_tokens_[2]);
               std::string trimmed_str_;
               HFSAT::PerishableStringTokenizer::TrimString(trades_tokens_[7], trimmed_str_, '\n');
               if ( trades_timestamp <= book_timestamp ) {
                 std::cout << trades_tokens_[1] << " " << trades_tokens_[2] << " OnTrade@ " << trades_tokens_[6]
                           << " of size " << trimmed_str_ << " " << trades_tokens_[5] << std::endl;
               }
               else {
                 trade_data.product = trades_tokens_[1];
                 trade_data.timestamp = std::stol(trades_tokens_[2]);
                 trade_data.price = std::stof(trades_tokens_[6]);
                 trade_data.size = std::stof(trimmed_str_);
                 trade_data.buy_sell = trades_tokens_[5];
                 last_trade_flag = true;
                 break;
               }
             }
           }
           for ( uint16_t size_vec = 0 ; size_vec < level_ ; ++size_vec) {
             std::cout << "\t" << ask_bid_book_vec_[size_vec].bid_size << " " 
                       << ask_bid_book_vec_[size_vec].bid_price << "  X  "
                       << ask_bid_book_vec_[size_vec].ask_price << "  "
                       << ask_bid_book_vec_[size_vec].ask_size << std::endl;
           }
           std::cout << "\n" << std::endl;
         }  // end while
       }
       book_file_stream_.close();
       trades_file_stream_.close();
     } else {
       std::cerr << "Fatal error - could not read files " << book_file_ << "\n" << trades_file_ << ".Exiting.\n";
       exit(0);
     }
   }

};


int main ( int argc, char *argv[] )
{

  if ( argc != 4 ) {
    std::cerr << "USAGE : <YYYYMMDD> <PRODUCT[BTC]> <level(max 25)>" << std::endl ;
    std::exit(-1);
  }

  std::string YYYYMMDD = argv[1] ;
  std::string product = argv[2];
  uint32_t level = atoi(argv[3]);

  std::ostringstream trade_file_path, book_file_path;
  std::string trades_file, book_file; 

  trade_file_path << CRYPTO_DATA_FILE_DIR << "ftx_trades_" 
                  << YYYYMMDD.substr(0, 4) << "-" << YYYYMMDD.substr(4, 2) << "-" << YYYYMMDD.substr(6, 2)
                  << "_" << product << "-PERP.csv.gz";
  trades_file = trade_file_path.str();
  
  if (level == 1) {
    book_file_path << CRYPTO_DATA_FILE_DIR << "ftx_quotes_" 
                    << YYYYMMDD.substr(0, 4) << "-" << YYYYMMDD.substr(4, 2) << "-" << YYYYMMDD.substr(6, 2)
                    << "_" << product << "-PERP.csv.gz";
    book_file = book_file_path.str();

  } 
  else if ((level > 1) && (level < 6)) {
    book_file_path << CRYPTO_DATA_FILE_DIR << "ftx_book_snapshot_5_" 
                    << YYYYMMDD.substr(0, 4) << "-" << YYYYMMDD.substr(4, 2) << "-" << YYYYMMDD.substr(6, 2)
                    << "_" << product << "-PERP.csv.gz";
    book_file = book_file_path.str();
  }
  else if ((level > 5) && (level < 26)) {
    book_file_path << CRYPTO_DATA_FILE_DIR << "ftx_book_snapshot_25_" 
                    << YYYYMMDD.substr(0, 4) << "-" << YYYYMMDD.substr(4, 2) << "-" << YYYYMMDD.substr(6, 2)
                    << "_" << product << "-PERP.csv.gz";
    book_file = book_file_path.str();
  }
  else
    std::cout << "INVALID LEVEL VALUE" << std::endl;

  ReadFXTCryptoFile readcryptofiles(trades_file, book_file);
  readcryptofiles.ReadDumpTradeBook(level);

  return EXIT_SUCCESS;
}                       
