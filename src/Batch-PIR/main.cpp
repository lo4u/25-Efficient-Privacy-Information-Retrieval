#include "okvs.h"
#include "pir_io.h"
#include "PIR_server.h"
#include "net_server.h"

int main() {
  NetServer server;
  server.start();
  return 0;
}