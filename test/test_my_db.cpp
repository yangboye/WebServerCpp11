// =============================================================================
// Created by yangb on 2021/3/28.
// =============================================================================

#include "test_my_db.h"

#include <iostream>

test::TestMyDB::TestMyDB() {
  conn_ = mysql_init(nullptr);
  if (conn_ == nullptr) {
    std::cerr << "error occurs: " << mysql_error(conn_);
    exit(1);
  }
}

test::TestMyDB::~TestMyDB() {
  if (conn_) {
    mysql_close(conn_);
  }
}

bool test::TestMyDB::InitDB(std::string host, std::string user, std::string pwd, std::string db_name) {
  conn_ = mysql_real_connect(conn_, host.c_str(), user.c_str(), pwd.c_str(), db_name.c_str(), 0, NULL, 0);

  if (conn_ == nullptr) {
    std::cerr << "error occurs: " << mysql_error(conn_);
    exit(1);
  }

  return true;
}

bool test::TestMyDB::ExecSql(std::string sql) {
  if (mysql_query(conn_, sql.c_str())) {
    std::cerr << "query error: " << mysql_error(conn_) << std::endl;
    return false;
  } else {
    result_ = mysql_use_result(conn_);
    if(result_) {
      int num_fields = mysql_num_fields(result_);
      int num_rows = mysql_field_count(conn_);
      for(int i = 0; i < num_rows; ++i) {
        row_ = mysql_fetch_row(result_);
        if(!row_) {
          break;
        }
        for(int j = 0; j < num_fields; ++j) {
          std::cout << row_[j] << "\t";
        }
        std::cout << std::endl;
      }
    }
    mysql_free_result(result_);
  }

  return true;
}
