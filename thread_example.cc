#include <iostream>
#include <thread>
#include "hierarchical_mutex.h"

hierarchical_mutex high_m(1000);
hierarchical_mutex low_m(900);
void low_level_func();

void thread_init() {
  std::unique_lock<hierarchical_mutex> l(high_m);
  std::cout<<"This statement is executed in the new thread."<<std::endl;
  low_level_func();
  l.unlock();
}

void low_level_func() {
  std::unique_lock<hierarchical_mutex> l(low_m);
  std::cout<<"This statement is executed in the new thread."<<std::endl;
  l.unlock();
}

int main() {
  std::cout<<std::thread::hardware_concurrency()<<" cores"<<std::endl;
  std::thread t(thread_init);
  std::cout<<"This statement is executed in the main thread."<<std::endl;
  t.join();
  }
