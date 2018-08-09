#ifndef EXAMPLE_CLIENT_H
#define EXAMPLE_CLIENT_H

#include "MozQuic.h"
#include <string>


struct Closure {
  FILE* fd[256] = {nullptr};
  int getCount = 0;
  bool recvFin = false;
};

class Client {
  mozquic_connection_t* connection;
  mozquic_config_t config;

public:
  explicit Client(mozquic_config_t& config):
  connection(nullptr),
  config(config){};

  ~Client() = default;

  void run();
  int connect(Closure& closure);
  // int disconnect();
};

#endif //CLIENT_H
