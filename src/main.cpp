// =============================================================================
// Created by yangb on 2021/4/1.
// =============================================================================

#include "server/web_server.h"

int main(int argc, char** argv) {
  WebServer server(12345, 3, 60000, false,
                   3306, "damon", "123456", "webserver",
                   12, 6, true, 0, 1024);
  server.Start();
  return 0;
}