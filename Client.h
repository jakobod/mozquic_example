#ifndef EXAMPLE_CLIENT_H
#define EXAMPLE_CLIENT_H

#include "MozQuic.h"
#include <string>

class Client {
  mozquic_connection_t* connection;
  mozquic_config_t config;

public:
  explicit Client(mozquic_config_t config):
  connection(nullptr),
  config(config){};

  ~Client() = default;

  int run(std::string& adress, uint16_t port);
  int connect(std::string& adress, uint16_t port);
  // int disconnect();
};

#endif //CLIENT_H
