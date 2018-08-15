//
// Created by jakob on 8/15/18.
//

#include <unistd.h>
#include "Trigger.h"

void Trigger::operator()() {
  bool running = true;
  while (running){
    for(auto c : connections) {
      mozquic_IO(c);
    }
    usleep(1000);
  }
}