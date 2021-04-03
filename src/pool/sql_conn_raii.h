// =============================================================================
// Created by yangb on 2021/4/3.
// 数据库连接RAII类
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_POOL_SQL_CONN_RAII_H_
#define WEBSERVERCPP11_SRC_POOL_SQL_CONN_RAII_H_

#include <cassert>
#include "sql_conn_pool.h"

/// @brief 资源在对象构造时初始化, 资源在对象析构时释放
class SqlConnRAII {
 public:
  SqlConnRAII(MYSQL** sql, SqlConnPool* conn_pool) {
    assert(conn_pool);
    *sql = conn_pool->GetConn();
    sql_ = *sql;
    conn_pool_ = conn_pool;
  }

 private:
  MYSQL* sql_;
  SqlConnPool* conn_pool_;
};

#endif //WEBSERVERCPP11_SRC_POOL_SQL_CONN_RAII_H_
