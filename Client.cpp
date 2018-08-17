/*
 *  @author Jakob Otto
 */

#include "Client.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <vector>
#include "MozQuic.h"
#include "mozquic_helper.h"

using namespace std;

// function prototypes
static int connEventCB(void *closure, uint32_t event, void *param);

// constants
static const char* SERVER_NAME = "localhost";
static const uint16_t SERVER_PORT = 4434;

void Client::run() {
  Closure closure;
  // set up connections to the server
  connect(closure);
  // should be connected now

  //  streamtest(closure);

  mozquic_destroy_connection(connection);
}


void Client::streamtest(Closure& closure) {
  cout << "streamtest is starting now." << endl;

  char pre[] = "PREAMBLE";
  char msg[8000];
  memset(msg, 'f', 7999);
  msg[7999] = 0;

  CHECK_MOZQUIC_ERR(
          mozquic_start_new_stream(&stream, connection, 0, 0, pre, (uint32_t)strlen(pre), 0),
          "streamtest-start_new_stream");
  CHECK_MOZQUIC_ERR(mozquic_send(stream, msg, (uint32_t)strlen(msg), 0), "streamtest-send");

  do {
    usleep (1000); // this is for handleio todo
    int code = mozquic_IO(connection);
    if (code != MOZQUIC_OK) {
      cerr << "IO reported failure" << endl;
      break;
    }
  } while (!closure.recvFin);
  closure.recvFin = false;
  int i = 0;
  do {
    usleep (1000); // this is for handleio todo
    int code = mozquic_IO(connection);
    if (code != MOZQUIC_OK) {
      cerr << "IO reported failure" << endl;
      break;
    }
  } while (++i < 2000);

  cout << "streamtest1 complete" << endl;
}


int Client::connect(Closure& closure) {
  config.originName = SERVER_NAME;
  config.originPort = SERVER_PORT;
  cout << "client connecting to " << config.originName << ":" << config.originPort << endl;

  config.handleIO = 0; // todo mvp

  // set things for the protocol
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "greaseVersionNegotiation", 0, 0), "connect-versionNegotiation");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateBadALPN", 1, 0), "connect-tolerateALPN");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateNoTransportParams", 1, 0), "connect-noTransportParams");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "maxSizeAllowed", 1452, 0), "connect-maxSize");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "enable0RTT", 1, 0), "connect-0rtt");

  // open new connections
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&connection, &config), "connect-new_conn");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(connection, connEventCB), "connect-callback");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback_closure(connection, &closure), "connect-closure");
  CHECK_MOZQUIC_ERR(mozquic_start_client(connection), "connect-start");

  uint32_t i=0;
  do {
    usleep (1000); // this is for handleio todo
    int code = mozquic_IO(connection);
    if (code != MOZQUIC_OK) {
      fprintf(stderr,"IO reported failure\n");
      break;
    }
    if (closure.getCount == -1) {
      break;
    }
  } while (++i < 2000 || closure.getCount);

  return 0;
}


int connEventCB(void *closure, uint32_t event, void *param) {
  auto clo = static_cast<Closure*>(closure);
  if (event == MOZQUIC_EVENT_0RTT_POSSIBLE) {
    cout << "We will send data during 0RTT." << endl;
  }
  if (event == MOZQUIC_EVENT_CONNECTED)
    cout << "client connected" << endl;

  if (event == MOZQUIC_EVENT_NEW_STREAM_DATA) {
    mozquic_stream_t *stream = param;
    if (mozquic_get_streamid(stream) & 0x3) {
      fprintf(stderr,"ignore non client bidi streams\n");
      return MOZQUIC_OK;
    }

    char buf[1000];
    uint32_t amt = 0;
    int fin = 0;

    int code = mozquic_recv(stream, buf, 1000, &amt, &fin);
    if (code != MOZQUIC_OK) {
      fprintf(stderr,"recv stream error %d\n", code);
      return MOZQUIC_OK;
    }
    fprintf(stderr,"Data: stream %d %d fin=%d\n",
            mozquic_get_streamid(stream), amt, fin);
    for (size_t j=0; j < amt; ) {
      size_t rv = fwrite(buf + j, 1, amt - j, clo->fd[mozquic_get_streamid(stream)]);
      assert(rv > 0);
      j += rv;
    }
    if (fin) {
      if (clo->fd[mozquic_get_streamid(stream)]) {
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
    return MOZQUIC_OK;
  } else if (event == MOZQUIC_EVENT_IO) {
  } else if (event == MOZQUIC_EVENT_CLOSE_CONNECTION ||
             event == MOZQUIC_EVENT_ERROR) {
    if (event == MOZQUIC_EVENT_CLOSE_CONNECTION)
      cout << "MOZQUIC_EVENT_CLOSE_CONNECTION" << endl;
    else
      cerr << "MOZQUIC_EVENT_ERROR" << endl;

    mozquic_destroy_connection(param);
    exit(event == MOZQUIC_EVENT_ERROR ? 2 : 0);
  } else {
//    fprintf(stderr,"unhandled event %X\n", event);
  }
  return MOZQUIC_OK;
}
