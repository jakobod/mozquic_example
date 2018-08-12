#ifndef EXAMPLE_SERVER_H
#define EXAMPLE_SERVER_H

#include "MozQuic.h"

// closure is per connection, state is per stream
struct closure_t
{
  int i;
  int state[128];
  char buf[128][1024];
  int accum[128];
  int shouldClose;
};

class Server {
  void setup();
  mozquic_config_t config;
  mozquic_connection_t* connection;
  mozquic_connection_t* connection_ip6;
  mozquic_connection_t* hrr;
  mozquic_connection_t* hrr6;

public:

  Server() :
  config(),
  connection(nullptr),
  connection_ip6(nullptr),
  hrr(nullptr),
  hrr6(nullptr) {};


  ~Server() = default;
  void run();
};

#endif //CLIENT_H
