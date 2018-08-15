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
        "/home/jakob/CLionProjects/mozquic_example/nss-config/";

void run_client() {
  Client client;
  client.run();
}

void run_server() {
  Server server;
  server.run();
}

int main(int argc, char** argv) {
  // check for nss_config
  if (mozquic_nss_config(const_cast<char*>(NSS_CONFIG)) != MOZQUIC_OK) {
    std::cout << "MOZQUIC_NSS_CONFIG FAILURE [" << NSS_CONFIG << "]" << std::endl;
    return -1;
  }

  // parse input arguments
  if (argc >= 2) {
    std::string arg(argv[1]);
    if (arg == "--server" || arg == "-s") {
      run_server();
    }
    else if (arg == "--client" || arg == "-c"){
      run_client();
    }
  }
  else {
    run_server();
  }
}
