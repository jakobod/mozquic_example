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

// function prototypes
static int connEventCB(void *closure, uint32_t event, void *param);

static FILE* fd[256];
static int _getCount = 0;
static uint8_t recvFin = 0; // could be a bool variable

int Client::run(std::string& adress, uint16_t port){
  // set up connection to the server
  connect(adress, port);

  uint32_t i=0;
  do {
    usleep (1000); // this is for handleio todo
    int code = mozquic_IO(connection);
    if (code != MOZQUIC_OK) {
      fprintf(stderr,"IO reported failure\n");
      break;
    }
    if (_getCount == -1) {
      break;
    }
  } while (++i < 2000 || _getCount);
}


int Client::connect(std::string& adress, uint16_t port) {
  char *cdir = getenv ("MOZQUIC_NSS_CONFIG");
  if (mozquic_nss_config(cdir) != MOZQUIC_OK) {
    fprintf(stderr, "MOZQUIC_NSS_CONFIG FAILURE [%s]\n", cdir ? cdir : "");
    exit(-1);
  }

  fprintf(stderr,"client connecting to %s port %d\n", config.originName, config.originPort);

  mozquic_new_connection(&connection, &config);
  mozquic_set_event_callback(connection, connEventCB);
  mozquic_start_client(connection);
}


static int connEventCB(void *closure, uint32_t event, void *param)
{
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
    int fin = 0;

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
      FILE* file = fd[mozquic_get_streamid(stream)];
      if (file) {
        size_t rv = fwrite(buf + j, 1, amt - j, file);
        assert(rv > 0);
        j += rv;
      }
    }
    if (fin) {
      if (fd[mozquic_get_streamid(stream)]) {
        std::cout << "closing stream now" << std::endl;
        fclose (fd[mozquic_get_streamid(stream)]);
        fd[mozquic_get_streamid(stream)] = nullptr;
      }
      recvFin = 1;
      mozquic_end_stream(stream);
      if (_getCount) {
        if (!--_getCount) {
          _getCount = -1;
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
