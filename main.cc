
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

#include "unix/Thread.h"
using namespace std;
void process(std::string msg) {
  std::cout << msg << "\n";
  std::cout << "thread id" << this_thread::get_id() << "\n";
}

int main() {
  auto message = string{"faew"};
  auto func = std::bind(process, std::move(message));
  rnet::Thread::Thread thread(func, "thread1");
  thread.start();
  thread.join();
  func();
  return 0;
}