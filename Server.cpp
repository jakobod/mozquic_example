#include <string>
#include <set>
#include <iostream>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <vector>
#include <thread>
#include <algorithm>
#include "Server.h"
#include "MozQuic.h"
#include "mozquic_helper.h"
#include "Trigger.h"

using namespace std;

static const uint16_t SERVER_PORT = 44444;
static const char* SERVER_NAME = "foo.example.com";

int connEventCB(void *closure, uint32_t event, void *param);
int accept_new_connection(mozquic_connection_t *new_connection);
int close_connection(mozquic_connection_t *c);
void pass_to_clients(char* msg, mozquic_stream_t* stream);

vector<mozquic_connection_t *> connections;
vector<mozquic_stream_t*> streams;

void Server::run() {
  // set up server
  setup();

  Trigger trigger(server_connections);
  thread t_trigger(ref(trigger));

  string dummy;
  cout << "press [enter] to quit server" << endl;
  getline(cin, dummy);

  for(mozquic_connection_t* conn : connections) {
    mozquic_destroy_connection(conn);
  }

  trigger.stop();
  t_trigger.join();

  for(mozquic_connection_t* conn : server_connections) {
    mozquic_destroy_connection(conn);
  }
}

void Server::setup() {
  mozquic_config_t config;
  memset(&config, 0, sizeof(mozquic_config_t));

  config.originName = SERVER_NAME;
  config.originPort = SERVER_PORT;

  cout << "server using certificate for " << config.originName << ":"
       << config.originPort << endl;

  config.handleIO = 0;
  config.appHandlesLogging = 0;

  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateBadALPN", 1,
          nullptr), "setup-bad_ALPN");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateNoTransportParams",
          1, nullptr), "setup-noTransport");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "sabotageVN", 0, nullptr),
          "setup-sabotage");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "forceAddressValidation", 0,
          nullptr), "setup-addrValidation->0");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "streamWindow", 4906,
          nullptr), "setup-streamWindow");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "connWindow", 8192, nullptr),
          "setup-connWindow");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "enable0RTT", 1, nullptr),
          "setup-0rtt");

  mozquic_connection_t* conn;
  // set up connections
  config.ipv6 = 0;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&conn, &config),
          "setup-new_conn_ip4");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(conn, connEventCB),
          "setup-event_cb_ip4");
  CHECK_MOZQUIC_ERR(mozquic_start_server(conn), "setup-start_server_ip4");
  server_connections.push_back(conn);
  conn = nullptr;

  config.ipv6 = 1;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&conn, &config),
          "setup-new_conn_ip6");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(conn, connEventCB),
          "setup-event_cb_ip6");
  CHECK_MOZQUIC_ERR(mozquic_start_server(conn),
          "setup-start_server_ip6");
  server_connections.push_back(conn);
  conn = nullptr;

  config.originPort = SERVER_PORT + 1;
  config.ipv6 = 0;
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "forceAddressValidation", 1,
          nullptr), "setup-addr_Val_hrr");
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&conn, &config),
          "setup-new_conn_hrr");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(conn, connEventCB),
          "setup-event_cb_hrr");
  CHECK_MOZQUIC_ERR(mozquic_start_server(conn), "setup-start_server_hrr");
  server_connections.push_back(conn);
  conn = nullptr;

  cout << "server using certificate (HRR) for " << config.originName << ":"
       << config.originPort << endl;

  config.ipv6 = 1;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&conn, &config),
          "setup-new_conn_hrr6");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(conn, connEventCB),
          "setup-set_cb_hrr6");
  CHECK_MOZQUIC_ERR(mozquic_start_server(conn), "setup-start_server_hrr6");
  server_connections.push_back(conn);
  conn = nullptr;

  cout << "server initialized" << endl;
}

int connEventCB(void *closure, uint32_t event, void *param) {
  switch (event) {
    case MOZQUIC_EVENT_NEW_STREAM_DATA: {
      char buf[1024];
      uint32_t received = 0;
      int fin = 0;
      mozquic_stream_t *stream = param;
      int id = mozquic_get_streamid(stream);
      if (id >= 128) {
        return MOZQUIC_ERR_GENERAL;
      }
      do {
        memset(buf, 0, 1024);
        int code = mozquic_recv(stream, &buf, 1023, &received, &fin);
        if (code != MOZQUIC_OK) {
          cerr << "Read stream error " << code << endl;
          return MOZQUIC_OK;
        } else if (received > 0) {
          string msg (buf);
          if (msg == "/new_connection")
            streams.push_back(stream);
          else {
            pass_to_clients(buf, stream);
          }
        }
      } while (received > 0 && !fin);
    }
      break;

    case MOZQUIC_EVENT_ACCEPT_NEW_CONNECTION:
      return accept_new_connection(param);

    case MOZQUIC_EVENT_CLOSE_CONNECTION:
    case MOZQUIC_EVENT_ERROR:
      if (event == MOZQUIC_EVENT_CLOSE_CONNECTION)
        cout << "MOZQUIC_EVENT_CLOSE_CONNECTION" << endl;
      else
        cout << "MOZQUIC_EVENT_ERROR" << endl;
      return close_connection(param);

      default:
        break;
  }
  return MOZQUIC_OK;
}

int accept_new_connection(mozquic_connection_t* new_connection) {
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(new_connection, connEventCB),
          "accept_new_connection-set_callback");
  connections.push_back(new_connection);
  std::cout << "new connections accepted. connected: "
            << connections.size() << std::endl;
  return MOZQUIC_OK;
}

int close_connection(mozquic_connection_t *c) {
  auto it = find(connections.begin(), connections.end(), c);
  if (it != connections.end())
    connections.erase(it);
  cout << "server closed connection. connected: "
       << connections.size() << endl;
  return mozquic_destroy_connection(c);
}

void pass_to_clients(char* msg, mozquic_stream_t* stream) {
  for(mozquic_stream_t* s : streams) {
    if(s != stream) {
      mozquic_send(s, msg, static_cast<uint32_t>(strlen(msg)), 0);
    }
  }
}

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    std::string arg(argv[i]);

    if (arg == "--log" || arg == "-l") {
      // log everything
      setenv("MOZQUIC_LOG", "all:9", 0);
    }
  }

  char buf[100];
  memset(buf, 0, 100);
  getcwd(buf, 100);
  string nss_config(buf);
  nss_config += "/../nss-config/";

  // check for nss_config
  if (mozquic_nss_config(const_cast<char*>(nss_config.c_str())) != MOZQUIC_OK) {
    std::cout << "MOZQUIC_NSS_CONFIG FAILURE [" << nss_config << "]"
              << std::endl;
    return -1;
  }

  Server server;
  server.run();

  return 0;
}
