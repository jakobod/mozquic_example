#include <string>
#include <iostream>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include "Server.h"
#include "mozquic_helper.h"

using namespace std;

static const char* NSS_CONFIG =
        "/home/jakob/Desktop/mozilla/mozquic/sample/nss-config/";
static const uint16_t SERVER_PORT = 4434;
static const char* SERVER_NAME = "foo.example.com";
static const int SEND_CLOSE_TIMEOUT_MS = 1500;
static const int TIMEOUT_CLIENT_MS = 30000;

int connEventCB(void *closure, uint32_t event, void *param);
int accept_new_connection(mozquic_connection_t *new_connection);
int close_connection(mozquic_connection_t *c, closure_t* closure);

int connected = 0;

void Server::run() {
  // set up server
  setup();

  // now trigger IO
  uint32_t delay = 1000;
  uint32_t i = 0;
  do {
    usleep (delay); // this is for handleio todo
    if (!(i++ & 0xf)) {
      assert(connected >= 0);
      if (!connected) {
        delay = 5000;
      } else {
        delay = 1000;
      }
    }
    mozquic_IO(connection);
    mozquic_IO(connection_ip6);
    mozquic_IO(hrr);
    mozquic_IO(hrr6);
  } while (true);
}


void Server::setup() {
  mozquic_config_t config;
  memset(&config, 0, sizeof(mozquic_config_t));

  config.originName = SERVER_NAME;
  config.originPort = SERVER_PORT;

  cout << "server using certificate for " << config.originName << ":" << config.originPort << endl;

  config.handleIO = 0; // todo mvp
  config.appHandlesLogging = 0;

  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateBadALPN", 1, 0), "setup-bad_ALPN");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateNoTransportParams", 1, 0), "setup-noTransport");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "sabotageVN", 0, 0), "setup-sabotage");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "forceAddressValidation", 0, 0), "setup-addrValidation->0");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "streamWindow", 4906, 0), "setup-streamWindow");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "connWindow", 8192, 0), "setup-connWindow");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "enable0RTT", 1, 0), "setup-0rtt");

  // set up connections
  config.ipv6 = 0;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&connection, &config), "setup-new_conn_ip4");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(connection, connEventCB), "setup-event_cb_ip4");
  CHECK_MOZQUIC_ERR(mozquic_start_server(connection), "setup-start_server_ip4");

  config.ipv6 = 1;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&connection_ip6, &config), "setup-new_conn_ip6");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(connection_ip6, connEventCB), "setup-event_cb_ip6");
  CHECK_MOZQUIC_ERR(mozquic_start_server(connection_ip6), "setup-start_server_ip6");

  config.originPort = SERVER_PORT + 1;
  config.ipv6 = 0;
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "forceAddressValidation", 1, 0), "setup-addr_Val_hrr");
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&hrr, &config), "setup-new_conn_hrr");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(hrr, connEventCB), "setup-event_cb_hrr");
  CHECK_MOZQUIC_ERR(mozquic_start_server(hrr), "setup-start_server_hrr");

  cout << "server using certificate (HRR) for " << config.originName << ":" << config.originPort << endl;

  config.ipv6 = 1;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&hrr6, &config), "setup-new_conn_hrr6");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(hrr6, connEventCB), "setup-set_cb_hrr6");
  CHECK_MOZQUIC_ERR(mozquic_start_server(hrr6), "setup-start_server_hrr6");

  cout << "server initialized" << endl;
}

int connEventCB(void *closure, uint32_t event, void *param) {
  switch (event) {
    case MOZQUIC_EVENT_NEW_STREAM_DATA: {
      if (!closure) {
        return MOZQUIC_ERR_GENERAL;
      }

      char buf;
      bool streamtest = false;
      uint32_t amt = 0;
      int fin = 0;
      int line = 0;
      mozquic_stream_t *stream = param;
      auto data = static_cast<closure_t*>(closure);
      int id = mozquic_get_streamid(stream);
      if (id >= 128) {
        return MOZQUIC_ERR_GENERAL;
      }
      do {
        int code = mozquic_recv(stream, &buf, 1, &amt, &fin);
        if (code != MOZQUIC_OK) {
          cerr << "Read stream error " << code << endl;
          return MOZQUIC_OK;
        } else if (amt > 0) {
          assert(amt == 1);
          if (!line)
            cout << "Data:" << endl;
          line++;

          switch (data->state[id]) {
            case 0:
              if (buf == 'F')
                data->state[id] = 1;
              else if (buf == 'G')
                data->state[id] = 4;
              break;
            case 1:
              data->state[id] = (buf == 'I') ? 2 : 0;
              break;
            case 2:
              data->state[id] = (buf == 'N') ? 3 : 0;
              streamtest = true;
              data->shouldClose = 1;
              break;
            case 4:
              data->state[id] = (buf == 'E') ? 5 : 0;
              break;
            case 5:
              data->state[id] = (buf == 'T') ? 6 : 0;
              break;
            case 6:
              data->state[id] = (buf == ' ') ? 7 : 0;
              break;
            case 7:
            default:
              // do09(data, id, stream, &buf, amt)
              data->state[id] = 0;
              break;
          }
          cout << "state " << data->state[id] << " [" << buf << "] fin=" << fin << endl;
        }
      } while (amt > 0 && !fin && !streamtest);
      if (streamtest) {
        char msg[] = "Server sending data.";
        mozquic_send(stream, msg, strlen(msg), 1);
      }
    }
      break;

    case MOZQUIC_EVENT_RESET_STREAM:
      cout << "Stream was reset" << endl;
      return MOZQUIC_OK;

    case MOZQUIC_EVENT_ACCEPT_NEW_CONNECTION:
      return accept_new_connection(param);

    case MOZQUIC_EVENT_CLOSE_CONNECTION:
    case MOZQUIC_EVENT_ERROR:
      if (event == MOZQUIC_EVENT_CLOSE_CONNECTION)
        cout << "MOZQUIC_EVENT_CLOSE_CONNECTION" << endl;
      else
        cout << "MOZQUIC_EVENT_ERROR" << endl;

      // todo this leaks the 64bit int allocation
      return close_connection(param, static_cast<closure_t*>(closure));

    case MOZQUIC_EVENT_IO:
      if (!closure) {
        return MOZQUIC_OK;
      }
      {
        auto data = static_cast<closure_t*>(closure);
        // mozquic_connection_t *conn = param;
        data->i += 1;
        if (data->i == SEND_CLOSE_TIMEOUT_MS) {
          cerr << "TIMEOUT! server terminating connection" << endl;
          close_connection(param, data);
          //exit(0);
        } else if (data->shouldClose == 3) {
          cout << "server closing based on fin" << endl;
          close_connection(param, data);
        } else if (!(data->i % TIMEOUT_CLIENT_MS)) {
          cout << "server testing conn" << endl;
          mozquic_check_peer(param, 2000);
        }
        return MOZQUIC_OK;
      }

      default:
        cerr << "unhandled event " << event << endl;
  }
  return MOZQUIC_OK;
}


int accept_new_connection(mozquic_connection_t* new_connection) {
  auto closure = new closure_t;
  memset(closure, 0, sizeof(closure_t));
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(new_connection, connEventCB), "accept_new_connection-set_callback");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback_closure(new_connection, closure), "accept_new_connection-set_closure");
  connected++;
  std::cout << "new connections accepted. connected: " << connected << std::endl;
  return MOZQUIC_OK;
}


int close_connection(mozquic_connection_t *c, closure_t* closure) {
  connected--;
  assert(connected >= 0);
  cout << "server closed connection. connected: " << connected << endl;
  delete closure;
  return mozquic_destroy_connection(c);
}


int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    std::string arg(argv[i]);

    if (arg == "--log" || arg == "-l") {
      // log everything
      setenv("MOZQUIC_LOG", "all:9", 0);
    }
  }
  setenv("MOZQUIC_LOG", "all:9", 0);

  // check for nss_config
  if (mozquic_nss_config(const_cast<char*>(NSS_CONFIG)) != MOZQUIC_OK) {
    std::cout << "MOZQUIC_NSS_CONFIG FAILURE [" << NSS_CONFIG << "]" << std::endl;
    return -1;
  }

  Server server;
  server.run();
}

