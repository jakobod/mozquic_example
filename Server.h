#ifndef EXAMPLE_SERVER_H
#define EXAMPLE_SERVER_H

#include "MozQuic.h"

class Server {
  void setup();

public:
  Server() = default;
  ~Server() = default;

  void run();
};

#endif //CLIENT_H
