//
// Created by jakob on 8/15/18.
//

#ifndef MOZQUIC_EXAMPLE_TRIGGER_H
#define MOZQUIC_EXAMPLE_TRIGGER_H

#include "MozQuic.h"
#include <vector>

class Trigger {
  mozquic_connection_t* connection;
  bool running;

public:
  Trigger(mozquic_connection_t* connection) :
  connection(connection),
  running(true){};
  ~Trigger() = default;

  void operator()();
  void stop();
};


#endif //MOZQUIC_EXAMPLE_TRIGGER_H
