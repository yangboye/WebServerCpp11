// =============================================================================
// Created by yangb on 2021/4/2.
// =============================================================================

#ifndef WEBSERVERCPP11_SRC_POOL_SQL_CONN_POOL_H_
#define WEBSERVERCPP11_SRC_POOL_SQL_CONN_POOL_H_

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>

/// @brief 数据库连接池, 采用单例模式
class SqlConnPool {
 public:
  /// @brief 单例模式
  static SqlConnPool* Instance();

  /// @brief 获取一个连接
  MYSQL* GetConn();

  /// @brief 释放一个连接(将连接放回连接池中)
  void FreeConn(MYSQL* conn);

  /// @brief 获取当前连接池中的连接资源数量
  int GetFreeConnCount() const;

  /// @brief 初始化
  /// @param conn_size 数据库连接池中的连接资源数量
  void Init(const char* host, int port,
            const char* user, const char* passwd,
            const char* db_name, int conn_size = 10);

  /// @brief 关闭连接池
  void ClosePool();

  ~SqlConnPool();

 private:
  SqlConnPool();

 private:
  int max_conn_;    // 数据库连接池中的资源总数量

  std::queue<MYSQL*> conn_que_;
  mutable std::mutex mtx_;
  sem_t sem_id_;
};

#endif //WEBSERVERCPP11_SRC_POOL_SQL_CONN_POOL_H_
