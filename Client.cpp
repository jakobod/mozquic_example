/*
 *  @author Jakob Otto
 */

#include "Client.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <thread>
#include <string>
#include "Trigger.h"
#include "MozQuic.h"
#include "mozquic_helper.h"

using namespace std;

// function prototypes
static int connEventCB(void *closure, uint32_t event, void *param);
static int tryCB(void *closure, uint32_t event, void *param);

// constants
static const char* help = "./client [params]\n"
                          "possible params are:\n"
                          "--help: display this text\n"
                          "-p|--port <port> set port to connect to\n"
                          "-h|--host <host> set host to connect to\n"
                          "-l|--log: enable mozquic-connection logging\n"
                          "-n|--nss-config <path-to-config> set nss-config path";

void Client::run(std::string& host, uint16_t port) {
  // set up connections to the server
  connect(host, port);

  // start thread to trigger IO automatically
  vector<mozquic_connection_t*> conn;
  conn.push_back(connection);
  Trigger trigger(conn);
  thread t_trigger(std::ref(trigger));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  chat();

  // clean up all running things
  trigger.stop();
  t_trigger.join();
  mozquic_shutdown_connection(connection);
  mozquic_destroy_connection(connection);
}


void Client::chat() {
  mozquic_stream_t* stream;
  string msg;
  char conn_msg[] = "/new_connection";

  CHECK_MOZQUIC_ERR(
          mozquic_start_new_stream(&stream, connection, 0, 0, conn_msg,
                  static_cast<uint32_t>(strlen(conn_msg)), 0),
          "chat-start_new_stream");

  // conected and stream opened
  while (getline(cin, msg)) {
    if (msg == "/quit") break;
    CHECK_MOZQUIC_ERR(mozquic_send(stream, const_cast<char *>(msg.c_str()),
                                     static_cast<uint32_t>(msg.length()), 0),
                                     "chat-send");
  }

  cout << "client closing connection" << endl;
  mozquic_end_stream(stream);
}


void Client::connect(std::string host, uint16_t port) {
  mozquic_config_t config = {};
  memset(&config, 0, sizeof(mozquic_config_t));

  // handle IO manually. automatic handling not yet implemented.
  config.handleIO = 0;

  config.originName = host.c_str();
  config.originPort = port;
  cout << "client connecting to " << config.originName << ":"
       << config.originPort << endl;

  // set quic-related things
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "greaseVersionNegotiation",
                                          0, nullptr),
                    "connect-versionNegotiation");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateBadALPN", 1,
                                          nullptr), "connect-tolerateALPN");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "tolerateNoTransportParams",
                                          1, nullptr),
                    "connect-noTransportParams");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "maxSizeAllowed", 1452,
                                          nullptr), "connect-maxSize");
  CHECK_MOZQUIC_ERR(mozquic_unstable_api1(&config, "enable0RTT", 1, nullptr),
                    "connect-0rtt");

  try_connect(config);

  // open new connections
  CHECK_MOZQUIC_ERR(mozquic_new_connection(&connection, &config),
                    "connect-new_conn");
  CHECK_MOZQUIC_ERR(mozquic_set_event_callback(connection, connEventCB),
                    "connect-callback");
  CHECK_MOZQUIC_ERR(mozquic_start_client(connection), "connect-start");
}

void Client::try_connect(mozquic_config_t& config) {
  mozquic_connection_t* connection;
  mozquic_new_connection(&connection, &config);
  mozquic_set_event_callback(connection, tryCB);
  mozquic_start_client(connection);

  uint32_t i=0;
  do {
    usleep (1000); // this is for handleio todo
    int code = mozquic_IO(connection);
    if (code != MOZQUIC_OK) {
       fprintf(stderr,"IO reported failure\n");
       break;
    }
  } while (++i < 2000);

  mozquic_destroy_connection(connection);
}


int tryCB(void*, uint32_t event, void* param){
  if (event == MOZQUIC_EVENT_CLOSE_CONNECTION ||
     event == MOZQUIC_EVENT_ERROR) {
    mozquic_destroy_connection(param);
    exit(event == MOZQUIC_EVENT_ERROR ? 2 : 0);
  }
  return MOZQUIC_OK;
}


int connEventCB(void*, uint32_t event, void* param) {
  switch (event) {
    case MOZQUIC_EVENT_0RTT_POSSIBLE:
      cout << "0RTT possible" << endl;
      break;
    case MOZQUIC_EVENT_CONNECTED:
      cout << "client connected" << endl;
      break;

    case MOZQUIC_EVENT_NEW_STREAM_DATA: {
      mozquic_stream_t *stream = param;
      if (mozquic_get_streamid(stream) & 0x3) {
        cerr << "ignore data on streams 0-3" << endl;
        break;
      }

      char buf[1024];
      uint32_t amt = 0;
      int fin = 0;
      do {
        memset(buf, 0, 1024);
        int code = mozquic_recv(stream, buf, 1023, &amt, &fin);
        if (code != MOZQUIC_OK) {
          cerr << "recv stream error " << code << endl;
          break;
        }
        cout << buf << endl; // why double newline?!
    } while(amt > 0 && !fin);
      break;
    }

    case MOZQUIC_EVENT_CLOSE_CONNECTION:
    case MOZQUIC_EVENT_ERROR:
      if (event == MOZQUIC_EVENT_CLOSE_CONNECTION)
        cout << "server closed connection." << endl;
      else
        cerr << "MOZQUIC_EVENT_ERROR" << endl;
      mozquic_destroy_connection(param);
      exit(event == MOZQUIC_EVENT_ERROR ? 2 : 0);

    default:
      break;
  }

  return MOZQUIC_OK;
}

int main(int argc, char** argv) {
  uint16_t port = 4434;
  std::string host = "localhost";
  char buf[100];
  memset(buf, 0, 100);
  getcwd(buf, 100);
  string nss_config(buf);
  nss_config += "/../nss-config/";

  for (int i = 0; i < argc; ++i) {
    std::string arg(argv[i]);

    if (arg == "--port" || arg == "-p") {
      port = static_cast<uint16_t>(atoi(argv[i+1]));
      ++i;
    }
    if (arg == "--host" || arg == "-h") {
      host = string(argv[i+1]);
      ++i;
    }
    if (arg == "--help") {
      cout << help << endl;
      exit(0);
    }
    if (arg == "--log" || arg == "-l") {
      // log everything
      setenv("MOZQUIC_LOG", "all:9", 0);
    }
    if (arg == "--nss-config" || arg == "-n") {
      nss_config = string(argv[i+1]);
      i++;
    }
  }

  // check for nss_config
  if (mozquic_nss_config(const_cast<char*>(nss_config.c_str())) != MOZQUIC_OK) {
    std::cerr << "MOZQUIC_NSS_CONFIG FAILURE [" << nss_config << "]"
              << std::endl;
    exit(-1);
  }

  Client client;
  client.run(host, port);

  exit(0);
}
