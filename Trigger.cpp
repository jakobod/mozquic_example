//
// Created by jakob on 8/15/18.
//

#include <unistd.h>
#include "Trigger.h"

void Trigger::operator()() {
  while (running){
    for(mozquic_connection_t* conn : connections) {
      mozquic_IO(conn);
    }
    usleep(1000);
  }
}

void Trigger::stop() {
  running = false;
}