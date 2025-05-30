#include <fstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/Utils/spot_token_generator.hpp"

namespace HFSAT {

SpotTokenGenerator* SpotTokenGenerator::unique_instance_ = nullptr;

SpotTokenGenerator& SpotTokenGenerator::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new SpotTokenGenerator();
  }
  return *(unique_instance_);
}

// Reads last dumped saci generator from the file if present else starts from 1
SpotTokenGenerator::SpotTokenGenerator() {
  last_increment_value = 1;
  std::string spot_index_token_filename(SPOT_INDEX_TOKEN_FILE);
  if (FileUtils::ExistsAndReadable(spot_index_token_filename)) {
    std::ifstream spot_index_token_file;
    spot_index_token_file.open(spot_index_token_filename.c_str());

    if (!spot_index_token_file.is_open()) {
      std::cout << "Failed To Load The SPOT INDEX File : " << SPOT_INDEX_TOKEN_FILE << std::endl;
      exit(-1);
    }
    char buffer[1024];
    int MAX_LINE_SIZE = 1024;
    while (spot_index_token_file.good()) {
      spot_index_token_file.getline(buffer, MAX_LINE_SIZE);
      std::string line_buffer = buffer;
      HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();
      // We expect to read TOKEN, STRING
      if (tokens.size() != 2) continue;
      int token = atoi(tokens[0]);
      std::string spot_index_ = tokens[1];
      token_spot_index_[token] = spot_index_;
      spot_index_token_[spot_index_] = token;
      last_increment_value = token;
    }
  } else {
    std::cout << "Failed To Load The SPOT INDEX FILE : " << SPOT_INDEX_TOKEN_FILE << std::endl;
    exit(-1);
  }
}


std::unordered_map<std::string, int> SpotTokenGenerator::GetSpotIndexToTokenMap(){
  return spot_index_token_;
}

int SpotTokenGenerator::GetTokenOrUpdate(std::string spot_index) {
  if (spot_index_token_.find(spot_index) == spot_index_token_.end()) {
    return Update_New_token_generated(spot_index);
  } else {
    return spot_index_token_[spot_index];
  }
}

const char* SpotTokenGenerator::GetSpotIndexChar(int token) {
  if (token_spot_index_.find(token) == token_spot_index_.end()) {
    std::cout<<"SHOULD NOT COME HERE NOT FOUND, for token " << token <<std::endl;
    return NULL;
  } else {
    return token_spot_index_[token].c_str();;
  }
}

std::string SpotTokenGenerator::GetSpotIndex(int token) {
  if (token_spot_index_.find(token) == token_spot_index_.end()) {
    std::cout<<"SHOULD NOT COME HERE NOT FOUND, for token " << token <<std::endl;
    return "";
  } else {
    return token_spot_index_[token];
  }
}

// To recover from crashes, should be called in ors termination handler
int SpotTokenGenerator::Update_New_token_generated(std::string spot_index) {
  last_increment_value++;
  token_spot_index_[last_increment_value] = spot_index;
  spot_index_token_[spot_index] = last_increment_value;

  std::string spot_index_token_filename(SPOT_INDEX_TOKEN_FILE);
  if (!FileUtils::exists(spot_index_token_filename)) FileUtils::MkdirEnclosing(spot_index_token_filename);

  std::ofstream f(spot_index_token_filename, std::ios_base::app);
  f << last_increment_value << " " << spot_index << std::endl;
  f.close();
  return last_increment_value;
}
}  // namespace HFSAT
