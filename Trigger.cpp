//
// Created by jakob on 8/15/18.
//

#include <unistd.h>
#include "Trigger.h"

void Trigger::operator()() {
  while (running){
    for(auto c : connections) {
      mozquic_IO(c);
    }
    usleep(5000);
  }
}

void Trigger::stop() {
  running = false;
}