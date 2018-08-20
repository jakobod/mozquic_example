#ifndef EXAMPLE_SERVER_H
#define EXAMPLE_SERVER_H

#include "MozQuic.h"

static const uint8_t MAX_STREAM = 8;
static const uint16_t BUF_SIZE = 1024;

// closure is per connections, state is per stream
struct closure_t {
  int i;
  int shouldClose;
};

class Server {
  mozquic_connection_t* connection;
  mozquic_connection_t* connection_ip6;
  mozquic_connection_t* hrr;
  mozquic_connection_t* hrr6;

  void setup();

public:
  Server() :
    connection(nullptr),
    connection_ip6(nullptr),
    hrr(nullptr),
    hrr6(nullptr) {};
  ~Server() = default;

  void run();
};

#endif //CLIENT_H
