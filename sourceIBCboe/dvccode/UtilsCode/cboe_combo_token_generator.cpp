#include <fstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/cboe_combo_token_generator.hpp"

namespace HFSAT {

CBOEComboTokenGenerator* CBOEComboTokenGenerator::unique_instance_ = nullptr;

CBOEComboTokenGenerator& CBOEComboTokenGenerator::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new CBOEComboTokenGenerator();
  }
  return *(unique_instance_);
}

// Reads last dumped saci generator from the file if present else starts from 1
CBOEComboTokenGenerator::CBOEComboTokenGenerator() {
  last_increment_value = 0;
  std::string combo_token_filename(CBOE_COMBO_TOKEN_FILE);
  if (FileUtils::ExistsAndReadable(combo_token_filename)) {
    std::ifstream combo_token_file;
    combo_token_file.open(combo_token_filename.c_str());

    if (!combo_token_file.is_open()) {
      std::cout << "Failed To Load The COMBO File : " << CBOE_COMBO_TOKEN_FILE << std::endl;
      exit(-1);
    }
    char buffer[1024];
    int MAX_LINE_SIZE = 1024;
    while (combo_token_file.good()) {
      combo_token_file.getline(buffer, MAX_LINE_SIZE);
      std::string line_buffer = buffer;
      HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();
      // We expect to read TOKEN, STRING
      if (tokens.size() != 2) continue;
      int token = atoi(tokens[0]);
      std::string combo_ = tokens[1];
      token_combo_[token] = combo_;
      combo_token_[combo_] = token;
      last_increment_value = token;
    }
  } else {
    std::cout << "Failed To Load The COMBO TOKEN FILE : " << CBOE_COMBO_TOKEN_FILE << std::endl;
    exit(-1);
  }
}

std::unordered_map<std::string, int> CBOEComboTokenGenerator::GetComboToTokenMap(){
  return combo_token_;
}

int CBOEComboTokenGenerator::GetTokenOrUpdate(std::string combo) {
  if (combo_token_.find(combo) == combo_token_.end()) {
    return Update_New_token_generated(combo);
  } else {
    return combo_token_[combo];
  }
}

const char* CBOEComboTokenGenerator::GetComboChar(int token) {
  if (token_combo_.find(token) == token_combo_.end()) {
    std::cout<<"SHOULD NOT COME HERE NOT FOUND, for token " << token <<std::endl;
    return NULL;
  } else {
    return token_combo_[token].c_str();;
  }
}

std::string CBOEComboTokenGenerator::GetCombo(int token) {
  if (token_combo_.find(token) == token_combo_.end()) {
    std::cout<<"SHOULD NOT COME HERE NOT FOUND, for token " << token <<std::endl;
    return "";
  } else {
    return token_combo_[token];
  }
}

// To recover from crashes, should be called in ors termination handler
int CBOEComboTokenGenerator::Update_New_token_generated(std::string combo) {
  last_increment_value++;
  token_combo_[last_increment_value] = combo;
  combo_token_[combo] = last_increment_value;

  std::string combo_token_filename(CBOE_COMBO_TOKEN_FILE);
  if (!FileUtils::exists(combo_token_filename)) FileUtils::MkdirEnclosing(combo_token_filename);

  std::ofstream f(combo_token_filename, std::ios_base::app);
  f << last_increment_value << " " << combo << std::endl;
  f.close();
  return last_increment_value;
}
}  // namespace HFSAT
