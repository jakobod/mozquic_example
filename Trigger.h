//
// Created by jakob on 8/15/18.
//

#ifndef MOZQUIC_EXAMPLE_TRIGGER_H
#define MOZQUIC_EXAMPLE_TRIGGER_H

#include "MozQuic.h"
#include <vector>

class Trigger {
  std::vector<mozquic_connection_t*>& connections;
  bool running;

public:
  explicit Trigger(std::vector<mozquic_connection_t*>& connections) :
  connections(connections),
  running(true){};
  ~Trigger() = default;

  void operator()();
  void stop();
};


#endif //MOZQUIC_EXAMPLE_TRIGGER_H
