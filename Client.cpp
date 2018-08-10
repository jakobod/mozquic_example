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

static const char* NSS_CONFIG =
                    "/home/boss/CLionProjects/mozquic/sample/nss-config/";

// function prototypes
static int connEventCB(void *closure, uint32_t event, void *param);

void Client::run() {
  Closure closure;
  // set up connection to the server
  connect(closure);
  // should be connected now

}

int Client::connect(Closure& closure) {
  std::string nss_dir(NSS_CONFIG);
  // need to cast away const for c-interface..
  if (mozquic_nss_config(const_cast<char*>(nss_dir.c_str())) != MOZQUIC_OK) {
    std::cout << "MOZQUIC_NSS_CONFIG FAILURE [" << nss_dir << "]" << std::endl;
    exit(-1);
  }

  fprintf(stderr,"client connecting to %s port %d\n", config.originName, config.originPort);

  mozquic_new_connection(&connection, &config);
  mozquic_set_event_callback(connection, connEventCB);
  mozquic_set_event_callback_closure(connection, &closure);
  mozquic_start_client(connection);

  exit(0);
}

static int connEventCB(void* closure, uint32_t event, void* param) {
  auto clo = static_cast<Closure*>(closure);
  if (event == MOZQUIC_EVENT_CONNECTED) {
    std::cout << "client is connected" << std::endl;
  }

  if (event == MOZQUIC_EVENT_0RTT_POSSIBLE) {
    std::cout << "We will send data during 0RTT.\n" << std::endl;
  }

  if (event == MOZQUIC_EVENT_NEW_STREAM_DATA) {
    mozquic_stream_t *stream = param;
    // data came from stream 0-3 ignore these
    if (mozquic_get_streamid(stream) & 0x3) {
      std::cout << "ignore non client bidi streams\n" << std::endl;
      return MOZQUIC_OK;
    }

    char buf[1000] = {0};
    uint32_t amt = 0;
    int fin = false;

    int code = mozquic_recv(stream, buf, sizeof(buf), &amt, &fin);
    if (code != MOZQUIC_OK) {
      std::cout << "recv stream error " << code << std::endl;
      return MOZQUIC_OK;
    }
    // print received data
    std::cout << buf << std::endl;
    std::cout << "Data: stream " << mozquic_get_streamid(stream)
              << amt << " fin=" << fin << std::endl;

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
        std::cout << "closing stream now" << std::endl;
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