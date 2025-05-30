#include "baseinfra/MinuteBar/db_cash_corporate_action.hpp"

namespace HFSAT {

CashCorporateAction *unique_instance_ = nullptr;

CashCorporateAction &CashCorporateAction::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new CashCorporateAction();
  }
  return *(unique_instance_);
}

// reading cash corporate action file and storing it in map
CashCorporateAction::CashCorporateAction() {
  std::string corporate_action_file_ = "/spare/local/tradeinfo/NSE_Files/Cash_DB_Corporate_Actions.csv";
  if (HFSAT::FileUtils::ExistsAndReadable(corporate_action_file_)) {
    std::ifstream corporate_action_;
    corporate_action_.open(corporate_action_file_, std::ifstream::in);
    char readline_buffer_[1024];
    if (corporate_action_.is_open()) {
      while (corporate_action_.good()) {
        //std::cout << "crash" << std::endl;
        memset(readline_buffer_, 0, sizeof(readline_buffer_));
        corporate_action_.getline(readline_buffer_, sizeof(readline_buffer_));
        //std::cout << readline_buffer_ << std::endl;

        std::vector<char *> tokens_;
        // create a copy of line read before using non-const tokenizer
        char readline_buffer_copy_[1024];
        memset(readline_buffer_copy_, 0, sizeof(readline_buffer_copy_));
        strcpy(readline_buffer_copy_, readline_buffer_);
        //std::cout << "copy: " << readline_buffer_copy_ << std::endl;

        HFSAT::PerishableStringTokenizer::NonConstStringTokenizer(readline_buffer_copy_, ",", tokens_);
        //std::cout << "NonConstStringTokenizer" << std::endl;
        if (tokens_.size() == 5) {
          std::ostringstream date_product_;
          date_product_ << tokens_[0] << "_" << tokens_[2] ;
          std::string map_key_date_product_ = date_product_.str(); // map key = yyyymmdd_product
          double map_val_adjustment_ = atof(tokens_[4]); // map value = adjustment factor
          //std::cout << map_key_date_product_ << " " << map_val_adjustment_ << std::endl;
          //std::cout << "before assign map" << std::endl;
          productDate_adjustmentFactor[map_key_date_product_] = map_val_adjustment_;
          std::cout << map_key_date_product_ << " " << productDate_adjustmentFactor[map_key_date_product_] << " " << map_val_adjustment_ << std::endl;
        }
      }
      //std::cout << "reading done" << std::endl;
      corporate_action_.close();
    }  // end while
  }
  else {
    std::cerr << "Fatal error - could not read Corporate Action file " << corporate_action_file_ << ".Exiting.\n";
    exit(0);
  }
}

// will check if there is any corporate action on the input date of the product.
// if yes then it returns adjustment factor of that corporate action.
double CashCorporateAction::CheckCorporateAction(std::string date_, std::string product_) {
  std::string map_key_ = date_ + "_" + product_;
  //std::cout << "CashCorporateAction::CheckCorporateAction: " << map_key_ << std::endl;
  if (productDate_adjustmentFactor.find(map_key_) != productDate_adjustmentFactor.end()) {
    //std::cout << map_key_ << productDate_adjustmentFactor[map_key_] << std::endl;
    return productDate_adjustmentFactor[map_key_];
  }
  else
    return 1; // returns 1 is there is not corporate action for the product on input date.
}

}  // namespace HFSAT
