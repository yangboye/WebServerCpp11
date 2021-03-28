// =============================================================================
// Created by yangb on 2021/3/26.
// =============================================================================

#include "../test/test_my_db.h"


int main() {
  test::TestMyDB db;
  db.InitDB("127.0.0.1", "damon", "123456", "webserver");
  db.ExecSql("select * from user");

  return 0;
}