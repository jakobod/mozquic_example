/*
 *  @author Jakob Otto
 */

#include "Client.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include "MozQuic.h"
#include "mozquic_helper.h"

using namespace std;

// function prototypes
static int connEventCB(void *closure, uint32_t event, void *param);

// constants
static const char* SERVER_NAME = "localhost";
static const uint16_t SERVER_PORT = 44444;

void Client::run() {
  Closure closure;
  // set up connection to the server
  connect(closure);
  // should be connected now
}

int Client::connect(Closure& closure) {
  config.originName = SERVER_NAME;
  config.originPort = SERVER_PORT;
  cout << "client connecting to " << config.originName << ":" << config.originPort << endl;

  // ?!?! TODO:
  config.handleIO = 0; // todo mvp

  // set things for the protocol
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "greaseVersionNegotiation", 0, 0), "connect-versionNegotiation");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateBadALPN", 1, 0), "connect-tolerateALPN");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateNoTransportParams", 1, 0), "connect-noTransportParams");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "maxSizeAllowed", 1452, 0), "connect-maxSize");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "enable0RTT", 1, 0), "connect-0rtt");

  // open new connection
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&connection, &config), "connect-new_conn");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(connection, connEventCB), "connect-callback");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback_closure(connection, &closure), "connect-closure");
  CHECK_MOZQUIC_ERR(mozquic_start_client(connection), "connect-start");

  uint32_t i=0;
  do {
    usleep (1000); // this is for handleio todo
    int code = mozquic_IO(connection);
    if (code != MOZQUIC_OK) {
      cerr << "IO reported failure" << endl;
      break;
    }
    if (closure.getCount == -1) {
      break;
    }
  } while (true);
  return 0;
}

static int connEventCB(void* closure, uint32_t event, void* param) {
  auto clo = static_cast<Closure*>(closure);
  if (event == MOZQUIC_EVENT_CONNECTED) {
    cout << "client is connected" << endl;
  }

  if (event == MOZQUIC_EVENT_0RTT_POSSIBLE) {
    cout << "We will send data during 0RTT.\n" << endl;
  }

  if (event == MOZQUIC_EVENT_NEW_STREAM_DATA) {
    mozquic_stream_t *stream = param;
    // data came from stream 0-3 ignore these
    if (mozquic_get_streamid(stream) & 0x3) {
      cout << "ignore non client bidi streams\n" << endl;
      return MOZQUIC_OK;
    }

    char buf[1000] = {0};
    uint32_t amt = 0;
    int fin = false;

    int code = mozquic_recv(stream, buf, sizeof(buf), &amt, &fin);
    if (code != MOZQUIC_OK) {
      cout << "recv stream error " << code << endl;
      return MOZQUIC_OK;
    }
    // print received data
    cout << buf << endl;
    cout << "Data: stream " << mozquic_get_streamid(stream)
              << amt << " fin=" << fin << endl;

    for (size_t j=0; j < amt; ) {
      // check that file is opened!
      FILE* file = clo->fd[mozquic_get_streamid(stream)];
      if (file) {
        size_t rv = fwrite(buf + j, 1, amt - j, file);
        assert(rv > 0);
        j += rv;
      }
    }
    if (fin) {
      if (clo->fd[mozquic_get_streamid(stream)]) {
        cout << "closing stream now" << endl;
        fclose (clo->fd[mozquic_get_streamid(stream)]);
        clo->fd[mozquic_get_streamid(stream)] = nullptr;
      }
      clo->recvFin = true;
      mozquic_end_stream(stream);
      if (clo->getCount) {
        if (!--clo->getCount) {
          clo->getCount = -1;
        }
      }
    }
  } else if (event == MOZQUIC_EVENT_IO) {
  } else if (event == MOZQUIC_EVENT_CLOSE_CONNECTION ||
             event == MOZQUIC_EVENT_ERROR) {
    mozquic_destroy_connection(param);
    exit(event == MOZQUIC_EVENT_ERROR ? 2 : 0);
  }
  return MOZQUIC_OK;
}