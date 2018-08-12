//
// Created by boss on 8/11/18.
//
#include <iostream>
#include "MozQuic.h"

#ifndef MOZQUIC_EXAMPLE_MOZQUIC_HELPER_H
#define MOZQUIC_EXAMPLE_MOZQUIC_HELPER_H

#define CHECK_MOZQUIC_ERR(err, msg) \
        if (err) {\
          if (err == MOZQUIC_ERR_GENERAL)\
            std::cerr << msg << ": MOZQUIC_ERR_GENERAL" << std::endl;\
          else if (err == MOZQUIC_ERR_INVALID)\
            std::cerr << msg << ": MOZQUIC_ERR_INVALID" << std::endl;\
          else if (err == MOZQUIC_ERR_MEMORY)\
            std::cerr << msg << ": MOZQUIC_ERR_MEMORY" << std::endl;\
          else if (err == MOZQUIC_ERR_IO)\
            std::cerr << msg << ": MOZQUIC_ERR_IO" << std::endl;\
          else if (err == MOZQUIC_ERR_CRYPTO)\
            std::cerr << msg << ": MOZQUIC_ERR_CRYPTO" << std::endl;\
          else if (err == MOZQUIC_ERR_VERSION)\
            std::cerr << msg << ": MOZQUIC_ERR_VERSION" << std::endl;\
          else if (err == MOZQUIC_ERR_ALREADY_FINISHED)\
            std::cerr << msg << ": MOZQUIC_ERR_ALREADY_FINISHED" << std::endl;\
          else if (err == MOZQUIC_ERR_DEFERRED)\
            std::cerr << msg << ": MOZQUIC_ERR_DEFERRED" << std::endl;\
          else \
            std::cerr << msg << ": UNRECOGNIZED MOZQUIC_ERR" << std::endl;\
          exit(err);\
        }\

#endif //MOZQUIC_EXAMPLE_MOZQUIC_HELPER_H
