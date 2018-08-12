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
  mozquic_stream_t* stream;
  mozquic_config_t config;

  int streamtest1(Closure& closure);
  int connect(Closure& closure);

public:
  explicit Client():
                  connection(nullptr),
                  stream(nullptr),
                  config() {};
  ~Client() = default;
  void run();
};

#endif //CLIENT_H
