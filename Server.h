#ifndef EXAMPLE_SERVER_H
#define EXAMPLE_SERVER_H

#include "MozQuic.h"
#include <vector>

class Server {
  void setup();

  std::vector<mozquic_connection_t*> server_connections;


public:
  Server() = default;
  ~Server() = default;

  void run();
};

#endif //CLIENT_H
