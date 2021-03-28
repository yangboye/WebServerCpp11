// =============================================================================
// Created by yangb on 2021/3/28.
// Refernece: https://blog.csdn.net/qq_29449969/article/details/103493990?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-15.control&dist_request_id=&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-15.control
// =============================================================================

#ifndef WEBSERVERCPP11_TEST_TEST_MY_DB_H_
#define WEBSERVERCPP11_TEST_TEST_MY_DB_H_

#include <mysql/mysql.h>
#include <string>

namespace test {

class TestMyDB {
 public:
  TestMyDB();
  ~TestMyDB();

  bool InitDB(std::string host, std::string user, std::string pwd, std::string db_name);
  bool ExecSql(std::string sql);

 private:
  MYSQL *conn_;
  MYSQL_RES *result_;
  MYSQL_ROW row_;
};

} // namespace test


#endif //WEBSERVERCPP11_TEST_TEST_MY_DB_H_
