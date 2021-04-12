// =============================================================================
// Created by yangb on 2021/4/2.
// =============================================================================

#include <iostream>
#include <cassert>
#include "sql_conn_pool.h"

SqlConnPool::SqlConnPool() : max_conn_(0) {}

SqlConnPool::~SqlConnPool() {
  ClosePool();
}

SqlConnPool* SqlConnPool::Instance() {
  static SqlConnPool conn_pool;
  return &conn_pool;
}

MYSQL* SqlConnPool::GetConn() {
  MYSQL* sql = nullptr;
  if (conn_que_.empty()) {
    std::cerr << "SqlConnPool busy!" << std::endl;
    return nullptr;
  }
  sem_wait(&sem_id_);
  {
    std::lock_guard<std::mutex> locker(mtx_);
    sql = conn_que_.front();
    conn_que_.pop();
  }

  return sql;
}

void SqlConnPool::FreeConn(MYSQL* conn) {
  assert(conn);
  std::lock_guard<std::mutex> locker(mtx_);
  conn_que_.push(conn);
  sem_post(&sem_id_);
}

int SqlConnPool::GetFreeConnCount() const {
  std::lock_guard<std::mutex> locker(mtx_);
  return conn_que_.size();
}

void SqlConnPool::Init(const char* host,
                       int port,
                       const char* user,
                       const char* passwd,
                       const char* db_name,
                       int conn_size) {
  assert(host);
  assert(port > 0 && port < 65536);
  assert(user);
  assert(passwd);
  assert(db_name);
  assert(conn_size > 0);

  for (int i = 0; i < conn_size; ++i) {
    MYSQL* sql = nullptr;
    sql = mysql_init(sql);
    assert(sql);

    sql = mysql_real_connect(sql, host, user, passwd, db_name, port, nullptr, 0);
    assert(sql);

    conn_que_.push(sql);
  }
  max_conn_ = conn_size;
  sem_init(&sem_id_, 0, max_conn_);

}

void SqlConnPool::ClosePool() {
  std::lock_guard<std::mutex> locker(mtx_);
  while (!conn_que_.empty()) {
    auto item = conn_que_.front();
    conn_que_.pop();
    mysql_close(item);
  }
  mysql_library_end();
}