#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cassert>
#include "Client.h"
#include "Server.h"
#include "MozQuic.h"

static const char* NSS_CONFIG =
        "/home/jakob/Desktop/mozilla/mozquic/sample/nss-config/";

void run_client() {
  Client client;
  client.run();
}

void run_server() {
  Server server;
  server.run();
}

int main(int argc, char** argv) {
  bool server = false;
  bool client = false;

  for (int i = 0; i < argc; ++i) {
    std::string arg(argv[i]);

    if (arg == "--server" || arg == "-s") {
      server = true;
    }
    else if (arg == "--client" || arg == "-c"){
      client = true;
    } else if (arg == "--log" || arg == "-l") {
      // log everything
      setenv("MOZQUIC_LOG", "all:9", 0);
    }
  }

  // check for nss_config
  if (mozquic_nss_config(const_cast<char*>(NSS_CONFIG)) != MOZQUIC_OK) {
    std::cout << "MOZQUIC_NSS_CONFIG FAILURE [" << NSS_CONFIG << "]" << std::endl;
    return -1;
  }

  // parse input arguments
  if (server) {
    run_server();
  }
  else if (client) {
    run_client();
  }
  else {
    setenv("MOZQUIC_LOG", "all:9", 0);
    run_client();
  }
}
