/*
 *  @author Jakob Otto
 */

#ifndef EXAMPLE_CLIENT_H
#define EXAMPLE_CLIENT_H

#include "MozQuic.h"
#include <string>

class Client {
  mozquic_connection_t* connection;
  mozquic_stream_t* stream;

  void connect(std::string host, uint16_t port);
  void chat();
public:
  explicit Client():
    connection(nullptr),
    stream(nullptr) {};
  ~Client() = default;
  void run(std::string& host, uint16_t port);

  void try_connect(mozquic_config_t& config);
};

#endif //CLIENT_H
