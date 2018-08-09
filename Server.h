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


public:
  Server() = default;
  ~Server() = default;
};

#endif //CLIENT_H
