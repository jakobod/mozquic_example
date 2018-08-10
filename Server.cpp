#include <string>
#include <iostream>
#include <cstring>
#include <cassert>
#import "Server.h"

static const char* NSS_CONFIG =
        "/home/boss/CLionProjects/mozquic/sample/nss-config/";
static const uint16_t SERVER_PORT = 4242;

static int connEventCB(void *closure, uint32_t event, void *param);

void Server::run() {
  setup();
}

void Server::setup() {
  config.ipv6 = 0;
  mozquic_new_connection(&connection, &config);
  mozquic_set_event_callback(connection, connEventCB);
  mozquic_start_server(connection);

  config.ipv6 = 1;
  mozquic_new_connection(&connection_ip6, &config);
  mozquic_set_event_callback(connection_ip6, connEventCB);
  mozquic_start_server(connection_ip6);

  config.originPort = SERVER_PORT + 1;
  config.ipv6 = 0;
  assert(mozquic_unstable_api1(&config, "forceAddressValidation", 1, nullptr) == MOZQUIC_OK);
  mozquic_new_connection(&hrr, &config);
  mozquic_set_event_callback(hrr, connEventCB);
  mozquic_start_server(hrr);
  fprintf(stderr,"server using certificate (HRR) for %s on port %d\n", config.originName, config.originPort);

  config.ipv6 = 1;
  mozquic_new_connection(&hrr6, &config);
  mozquic_set_event_callback(hrr6, connEventCB);
  mozquic_start_server(hrr6);
}

static int connEventCB(void *closure, uint32_t event, void *param) {
  switch (event) {
    case MOZQUIC_EVENT_NEW_STREAM_DATA:
    {
      mozquic_stream_t *stream = param;

      char buf;
      int streamtest1 = 0;
      uint32_t amt = 0;
      int fin = 0;
      int line = 0;
      struct closure_t *data = (struct closure_t *)closure;
      assert(closure);
      if (!closure) {
        return MOZQUIC_ERR_GENERAL;
      }
      int id = mozquic_get_streamid(stream);
      if (id >= 128) {
        return MOZQUIC_ERR_GENERAL;
      }
      do {
        uint32_t code = mozquic_recv(stream, &buf, 1, &amt, &fin);
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
              data->state[id] = (buf == 'G') ? 4 : 0;
              break;
            case 1:
              data->state[id] = (buf == 'I') ? 2 : 0;
              break;
            case 2:
              data->state[id] = (buf == 'N') ? 3 : 0;
              streamtest1 = 1;
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
              do09(data, id, stream, &buf, amt);
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

    case MOZQUIC_EVENT_RESET_STREAM:
    {
      // todo not implemented yet.
      // mozquic_stream_t *stream = param;
      fprintf(stderr,"Stream was reset\n");
      return MOZQUIC_OK;
    }

    case MOZQUIC_EVENT_ACCEPT_NEW_CONNECTION:
      return accept_new_connection(param);

    case MOZQUIC_EVENT_CLOSE_CONNECTION:
    case MOZQUIC_EVENT_ERROR:
      // todo this leaks the 64bit int allocation
      return close_connection(param);

    case MOZQUIC_EVENT_IO:
      if (!closure) {
        return MOZQUIC_OK;
      }
      {
        struct closure_t *data = (struct closure_t *)closure;
        // mozquic_connection_t *conn = param;
        data->i += 1;
        if (send_close && (data->i == SEND_CLOSE_TIMEOUT_MS)) {
          fprintf(stderr,"server terminating connection\n");
          close_connection(param);
          free(data);
          exit(0);
        } else if (data->shouldClose == 3) {
          fprintf(stderr,"server closing based on fin\n");
          close_connection(param);
          free(data);
        } else if (!(data->i % TIMEOUT_CLIENT_MS)) {
          fprintf(stderr,"server testing conn\n");
          mozquic_check_peer(param, 2000);
        }
        return MOZQUIC_OK;
      }

//  default:
//    fprintf(stderr,"unhandled event %X\n", event);
  }
  return MOZQUIC_OK;
}

