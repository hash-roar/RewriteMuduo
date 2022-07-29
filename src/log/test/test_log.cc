#include "log/Logger.h"
#include "unix/Thread.h"

using namespace rnet;

int main() {
  LOG_INFO << "error";
  LOG_INFO << "error";
  LOG_INFO << "error";

  return 0;
}