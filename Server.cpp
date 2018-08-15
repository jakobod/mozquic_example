#include <string>
#include <iostream>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include "Server.h"
#include "mozquic_helper.h"

using namespace std;

static const uint16_t SERVER_PORT = 44444;
static const char* SERVER_NAME = "localhost";

static int connEventCB(void *closure, uint32_t event, void *param);
static int accept_new_connection(mozquic_connection_t *new_connection);
int close_connection(mozquic_connection_t *c);

static int connected = 0;
static bool send_close = false;

void Server::run() {
  setup();
  std::string dummy;

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
  memset(&config, 0, sizeof(mozquic_config_t));
  config.originName = SERVER_NAME;
  config.originPort = SERVER_PORT;
  cout << "server using certificate for " << config.originName << ":" << config.originPort << endl;

  config.handleIO = 0; // todo mvp
  config.appHandlesLogging = 0;

  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateBadALPN", 1, nullptr), "setup-bad_ALPN");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateNoTransportParams", 1, nullptr), "setup-noTransport");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "sabotageVN", 0, nullptr), "setup-sabotage");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "forceAddressValidation", 0, nullptr), "setup-addrValidation->0");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "streamWindow", 4906, nullptr), "setup-streamWindow");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "connWindow", 8192, nullptr), "setup-connWindow");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "enable0RTT", 1, nullptr), "setup-0rtt");

  config.ipv6 = 0;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&connection, &config), "setup-new_conn1");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(connection, connEventCB), "setup-event_cb1");
  CHECK_MOZQUIC_ERR(mozquic_start_server(connection), "setup-start_server1");

  config.ipv6 = 1;
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&connection_ip6, &config), "setup-new_conn2");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(connection_ip6, connEventCB), "setup-event_cb2");
  CHECK_MOZQUIC_ERR(mozquic_start_server(connection_ip6), "setup-start_server2");

  config.originPort = SERVER_PORT + 1;
  config.ipv6 = 0;
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "forceAddressValidation", 1, nullptr), "setup-fore_addValidation->1");
  mozquic_new_connection(&hrr, &config);
  mozquic_set_event_callback(hrr, connEventCB);
  mozquic_start_server(hrr);
  cout << "server using certificate (HRR) for " << config.originName << ":" << config.originPort << endl;

  config.ipv6 = 1;
  mozquic_new_connection(&hrr6, &config);
  mozquic_set_event_callback(hrr6, connEventCB);
  mozquic_start_server(hrr6);
}

static int connEventCB(void *closure, uint32_t event, void *param) {
  switch (event) {
    case MOZQUIC_EVENT_NEW_STREAM_DATA: {
      mozquic_stream_t *stream = param;

      char buf;
      int streamtest1 = 0;
      uint32_t amt = 0;
      int fin = 0;
      int line = 0;
      auto data = (struct closure_t *)closure;
      if (!closure) {
        return MOZQUIC_ERR_GENERAL;
      }
      int id = mozquic_get_streamid(stream);
      if (id >= 128) {
        return MOZQUIC_ERR_GENERAL;
      }
      do {
        int code = mozquic_recv(stream, &buf, 1, &amt, &fin);
        if (code != MOZQUIC_OK) {
          fprintf(stderr,"Read stream error %d\n", code);
          return MOZQUIC_OK;
        } else if (amt > 0) {
          assert(amt == 1);
          if (!line) {
            fprintf(stderr,"Data:\n");
          }
          line++;
          switch (data->state[id]) {
            case 0:
              data->state[id] = (buf == 'F') ? 1 : 0;
              break;
            case 1:
              data->state[id] = (buf == 'I') ? 2 : 0;
              break;
            case 2:
              data->state[id] = (buf == 'N') ? 3 : 0;
              streamtest1 = 1;
              data->shouldClose = 1;
              break;
            default:
              data->state[id] = 0;
              break;
          }
          fprintf(stderr,"state %d [%c] fin=%d\n", data->state[id], buf, fin);
        }
      } while (amt > 0 && !fin && !streamtest1);
      if (streamtest1) {
        char msg[] = "Server sending data.";
        mozquic_send(stream, msg, strlen(msg), 1);
      }
    }
      break;

    case MOZQUIC_EVENT_ACCEPT_NEW_CONNECTION:
      return accept_new_connection(param);

    case MOZQUIC_EVENT_CLOSE_CONNECTION:
    case MOZQUIC_EVENT_ERROR:
      if (event == MOZQUIC_EVENT_ERROR) cout << "MOZQUIC_EVENT_ERROR" << endl;
      else cout << "MOZQUIC_EVENT_CLOSE_CONNECTION" << endl;
      // todo this leaks the 64bit int allocation
      return close_connection(param);

      default:
        break;
  }
  return MOZQUIC_OK;
}

static int accept_new_connection(mozquic_connection_t *new_connection) {
  auto closure = new closure_t;
  memset(closure, 0, sizeof (*closure));
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(new_connection, connEventCB), "accept_new_connection-set_callback");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback_closure(new_connection, closure), "accept_new_connection-set_closure");
  connected++;
  std::cout << "new connections accepted. connected: " << connected << std::endl;
  return MOZQUIC_OK;
}

int close_connection(mozquic_connection_t *c) {
  connected--;
  assert(connected >= 0);
  cout << "server closed connection. connected: " << connected << endl;
  return mozquic_destroy_connection(c);
}
