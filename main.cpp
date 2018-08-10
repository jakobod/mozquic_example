#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cassert>
#include "Client.h"
#include "Server.h"
#include "MozQuic.h"

static const char* SERVER_NAME = "localhost";
static const uint16_t SERVER_PORT = 4242;

mozquic_config_t* make_config(int argc, char** argv){
  auto cfg = new mozquic_config_t;
  memset(cfg, 0, sizeof(mozquic_config_t));
  cfg->originName = SERVER_NAME;
  cfg->originPort = SERVER_PORT;

  for (int i = 0; i < argc; ++i) {
    std::string comp(argv[i]);
    if (comp == "-port" || comp == "-p") {
      cfg->originPort = atoi(argv[i+1]);
      ++i;
      continue;
    } else if (comp == "-host" || comp == "-h") {
      cfg->originName = argv[i+1];
      ++i;
      continue;
    } else if (comp == "-0rtt") {
      assert(mozquic_unstable_api1(cfg, "enable0RTT", 1, nullptr) == MOZQUIC_OK);
    }
  }

  assert(mozquic_unstable_api1(cfg, "greaseVersionNegotiation", 0, nullptr) == MOZQUIC_OK);
  assert(mozquic_unstable_api1(cfg, "tolerateBadALPN", 1, nullptr) == MOZQUIC_OK);
  assert(mozquic_unstable_api1(cfg, "tolerateNoTransportParams", 1, nullptr) == MOZQUIC_OK);
  assert(mozquic_unstable_api1(cfg, "maxSizeAllowed", 1452, nullptr) == MOZQUIC_OK);

  return cfg;
}

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string arg(argv[1]);
    if (arg == "-server" || arg == "-s") {
      auto config = *make_config(argc, argv);
      fprintf(stderr,"server using certificate for %s on port %d\n", config.originName, config.originPort);

      assert(mozquic_unstable_api1(&config, "tolerateBadALPN", 1, nullptr) == MOZQUIC_OK);
      assert(mozquic_unstable_api1(&config, "tolerateNoTransportParams", 1, nullptr) == MOZQUIC_OK);
      assert(mozquic_unstable_api1(&config, "sabotageVN", 0, nullptr) == MOZQUIC_OK);
      assert(mozquic_unstable_api1(&config, "forceAddressValidation", 0, nullptr) == MOZQUIC_OK);
      assert(mozquic_unstable_api1(&config, "streamWindow", 4906, nullptr) == MOZQUIC_OK);
      assert(mozquic_unstable_api1(&config, "connWindow", 8192, nullptr) == MOZQUIC_OK);
      assert(mozquic_unstable_api1(&config, "enable0RTT", 1, nullptr) == MOZQUIC_OK);

      // assert(mozquic_unstable_api1(&config, "dropRate", 5, 0) == MOZQUIC_OK);

      Server server(config);
      server.run();
    }
    else if (arg == "-client" || arg == "-c"){
      auto config = *make_config(argc, argv);
      Client client(config);
      client.run();
    }
  }
  else {
    auto config = *make_config(argc, argv);
    Client client(config);
    client.run();
  }
}