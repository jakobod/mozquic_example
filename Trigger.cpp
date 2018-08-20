//
// Created by jakob on 8/15/18.
//

#include <unistd.h>
#include "Trigger.h"

void Trigger::operator()() {
  while (running){
    mozquic_IO(connection);
    usleep(5000);
  }
}

void Trigger::stop() {
  running = false;
}