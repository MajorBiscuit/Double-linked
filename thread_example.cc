#include <iostream>
#include <thread>

void thread_init() {
  std::cout<<"This statement is executed in the new thread."<<std::endl;
}

int main() {
  std::cout<<std::thread::hardware_concurrency()<<" cores"<<std::endl;
  std::thread t(thread_init);
  std::cout<<"This statement is executed in the main thread."<<std::endl;
  t.join();
  }
