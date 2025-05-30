#include <fstream>

#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/get_db_config.hpp"

namespace HFSAT {

GetDBConfig* GetDBConfig::unique_instance_ = nullptr;

GetDBConfig& GetDBConfig::GetUniqueInstance() {
  if (unique_instance_ == nullptr) {
    unique_instance_ = new GetDBConfig();
  }
  return *(unique_instance_);
}

GetDBConfig::GetDBConfig() {
  std::string db_gen_file(DB_CONF_FILE);
  if (FileUtils::ExistsAndReadable(db_gen_file)) {
    std::ifstream f(db_gen_file);
    char buffer[1024];
    f.getline(buffer, sizeof(buffer));
    HFSAT::PerishableStringTokenizer pst(buffer, 1024);
    std::string line_buffer = buffer;
    std::vector<char const*> const& tokens = pst.GetTokens();
    if (tokens.size() != 3) {
        std::cout<<"DB CONFIG FILE DOES WRONG FIELDS COUNT Size: " << tokens.size()<< tokens[0] <<" file: " << DB_CONF_FILE <<std::endl;
        exit(-1);
    }
    std::string tmp_ip = tokens[0];
    std::string tmp_user = tokens[1];
    std::string tmp_pass = tokens[2];
    db_ip = tmp_ip;
    db_user = tmp_user;
    db_password = tmp_pass;
    f.close();
  }
  else {
    std::cout<<"DB CONFIG FILE DOES NOT EXIST " << DB_CONF_FILE <<std::endl;
    exit(-1);
  }
}
}
