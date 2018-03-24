#include <iostream>
#include <thread>
// Declare a function object

struct func {
  int& i;
  func(int& i_):i(i_) {}
  void operator()() {
    for(unsigned j=0; j<10; j++) {
      i++;
      std::cout<<"We are in a thread"<<std::endl;
      std::cout<<"i is "<<i<<std::endl;
    }
  }
};

class thread_guard {
  std::thread& t;
public:
  explicit thread_guard(std::thread& t_):t(t_) {}
  ~thread_guard() {
    if (t.joinable()) t.join();
  }
  thread_guard(thread_guard const&) = delete;
  thread_guard& operator = (thread_guard const&) = delete;
};

int main() {
  int some_local_state = 0;
  func my_func(some_local_state);
  std::cout<<"We are now in the main thread."<<std::endl;
  // Initialise thread t with my_func. func()() operator will be called
  std::thread t(my_func);
  thread_guard g(t);
}
