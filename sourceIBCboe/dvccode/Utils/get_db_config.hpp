#ifndef _DB_WORKER_CONFIG_
#define _DB_WORKER_CONFIG_

namespace HFSAT {

#define DB_CONF_FILE "/home/pengine/prod/live_configs/db_worker_config.txt"

class GetDBConfig {
 public:
  static GetDBConfig& GetUniqueInstance();
  std::string  GetDbIp() { return db_ip; }
  std::string  GetDbUser() { return db_user; }
  std::string  GetDbPassword() { return db_password; }

 private:
  std::string db_ip, db_user,db_password;
  GetDBConfig();
  static GetDBConfig* unique_instance_;
  
};
}

#endif
