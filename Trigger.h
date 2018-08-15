//
// Created by jakob on 8/15/18.
//

#ifndef MOZQUIC_EXAMPLE_TRIGGER_H
#define MOZQUIC_EXAMPLE_TRIGGER_H

#include "MozQuic.h"
#include <vector>

class Trigger {
  std::vector<mozquic_connection_t*>& connections;

public:
  Trigger(std::vector<mozquic_connection_t*>& connections) :
  connections(connections) {};
  ~Trigger() = default;

  void operator()();
};


#endif //MOZQUIC_EXAMPLE_TRIGGER_H
