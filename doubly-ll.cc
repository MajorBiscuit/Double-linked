#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <functional> // for std::function
#include <algorithm>  // for std::generate_n
#include <limits>     // for numeric_limits

class hierarchical_mutex {
  std::mutex internal_mutex;
  unsigned long const hierarchy_value;
  unsigned long previous_hierarchy_value;
  static thread_local unsigned long this_thread_hierarchy_value;
  void check_for_hierarchy_violation() {
    if (this_thread_hierarchy_value <= hierarchy_value) {
      std::cout<<"this_thread_hierarchy_value "<<this_thread_hierarchy_value<<std::endl;
      std::cout<<"hierarchy_value "<<hierarchy_value<<std::endl;
      throw std::logic_error("Mutex hierarchy violated");
    }
  }
  void update_hierarchy_value() {
    previous_hierarchy_value = this_thread_hierarchy_value;
    this_thread_hierarchy_value = hierarchy_value;
  }
public:
  explicit hierarchical_mutex(unsigned long value):
    hierarchy_value(value), previous_hierarchy_value(0) {}
  void lock() {
    check_for_hierarchy_violation();
    internal_mutex.lock();
    update_hierarchy_value();
  }
  void unlock() {
    this_thread_hierarchy_value=previous_hierarchy_value;
    internal_mutex.unlock();
  }
  bool try_lock() {
    check_for_hierarchy_violation();
    if (!internal_mutex.try_lock())
      return false;
    update_hierarchy_value();
    return true;
  }
};
// static data member 
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(std::numeric_limits<unsigned long>::max());

class doubly_ll {
  struct Node {
    hierarchical_mutex m;
    unsigned long mutex_order;
    std::string data;
    std::unique_ptr<Node> next;
    Node* previous;
    Node(unsigned long m_order)
      : m(m_order), mutex_order(m_order), next(nullptr), previous(nullptr)
    {}
    Node(unsigned long m_order, std::string const& value)
      : m(m_order), mutex_order(m_order), data(value), next(nullptr), previous(nullptr)
    {}
  };
  // variable declaration in class. Valid in C++11
  std::unique_ptr<Node> head{new Node(10000)};
  Node* tail{new Node(5000)};

  std::string random_string( size_t length, std::function<char(void)> rand_char ) {
    std::string str(length, 0);
    std::generate_n(str.begin(), length, rand_char);
    return str;
  }

public:
  doubly_ll() {}

  ~doubly_ll() {
    // remove_if([](node const&){return true;});
  }

  doubly_ll(doubly_ll const&) = delete;
  doubly_ll& operator = (doubly_ll const&) = delete;

  void push (std::string const value, unsigned long m_order) {
    // std::cout<<"in push function\n";
    std::unique_ptr<Node> new_node(new Node(m_order, value));
    std::cout<<"initiated new_node\n";
    std::cout<<"attempting to lock head\n";
    std::unique_lock<hierarchical_mutex> head_l(head->m);
    std::cout<<"acquired lock on head mutex\n";
    if(head->data.empty()) {
      // std::unique_ptr is non-assignable and non-copyable
      // std::cout<<"in !head conditional\n";
      head = std::move(new_node);
      std::unique_lock<hierarchical_mutex> tail_l(tail->m);
      std::cout<<"acquired lock on tail mutex\n";
      tail = head.get();
      // std::cout<<"head points to "<<head.get()<<std::endl;
      // std::cout<<"tail points to "<<tail<<std::endl;
    }
    else if(head.get() == tail) {
      // having a lock on head is the same as having a lock on tail. No need to lock tail
      std::cout<<"in else if clause"<<std::endl;
      tail->next = std::move(new_node);
      tail->next->previous = tail;
      tail = tail->next.get();
    }
    else {
      std::cout<<"in else block\n";
      std::unique_lock<hierarchical_mutex> tail_l(tail->m);
      std::cout<<"acquired lock on tail\n";
      tail->next = std::move(new_node);
      std::unique_lock<hierarchical_mutex> next_l(tail->next->m);
      // std::cout<<"tail->next points to "<<tail->next.get()<<std::endl;
      // std::cout<<"head->next points to "<<head->next.get()<<std::endl;
      tail->next->previous = tail;
      tail = tail->next.get();
      // std::cout<<"tail->next points to "<<tail->next.get()<<std::endl;
      // std::cout<<"head->next points to "<<head->next.get()<<std::endl;
    }
  }

  void display() {
    Node* current = head.get();
    while(current) {
      std::cout<<"Node is "<<current->data<<"\n";
      current = current->next.get();
    }
    std::cout<<"head is "<<head->data<<std::endl;
    std::cout<<"head mutex order is "<<head->mutex_order<<std::endl;
    std::cout<<"tail is "<<tail->data<<std::endl;
    std::cout<<"tail mutex order is "<<tail->mutex_order<<std::endl;
  }

  void populate(size_t size, size_t a, size_t b) {
    // list of possible characters
    const std::vector<char> charset {
      'a','b','c','d','e','f',
        'g','h','i','j','k',
        'l','m','n','o','p',
        'q','r','s','t','u',
        'v','w','x','y','z'
        };
    // non-deterministic random number generator
    std::default_random_engine rng_c(std::random_device{}());
    std::uniform_int_distribution<> dist_c(0, charset.size()-1);
    // returns a value from the uniform distribution
    auto randchar = [ charset, &dist_c, &rng_c ](){return charset[ dist_c(rng_c) ];};
    std::default_random_engine rng_s_length(std::random_device{}());
    std::uniform_int_distribution<> dist_s_length(a, b);
    for(unsigned int i=0; i < size; i++) {
      auto length = dist_s_length(rng_s_length);
      const std::string r_string = random_string(length, randchar);
      push(r_string, (unsigned long) 10000-i);
    }
  }

  void cat() {
    Node* current = head.get();
    std::unique_lock<hierarchical_mutex> current_l(head->m);
    std::string cat = head->data;
    while(Node* const next = current->next.get()) {
      std::unique_lock<hierarchical_mutex> next_l(next->m);
      current_l.unlock();
      cat += next->data;
      current = next;
      current_l = std::move(next_l);
    }
    std::cout<<"Concatenated string is "<<cat<<std::endl;
  }
};

int main() {
  doubly_ll list;
  //list.push("test1", 500);
  list.populate(100, 2, 8);
  //list.push("test2", 2000);
  list.display();
  list.cat();
  return 1;
}
