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
    std::string data;
    std::unique_ptr<Node> next;
    Node* previous;
    Node()
      : m(10), next(nullptr), previous(nullptr)
    {}
    Node(unsigned long m_order, std::string const& value)
      : m(m_order), data(value), next(nullptr), previous(nullptr)
    {}
  };
  // variable declaration in class. Valid in C++11
  std::unique_ptr<Node> head{new Node};
  Node* tail{new Node};

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
    // std::cout<<"initiated new_node\n";
    std::lock_guard<hierarchical_mutex> l(head->m);
    std::cout<<"acquired lock on mutex\n";
    if(head->data.empty()) {
      // std::unique_ptr is non-assignable and non-copyable
      // std::cout<<"in !head conditional\n";
      head = std::move(new_node);
      tail = head.get();
      // std::cout<<"head points to "<<head.get()<<std::endl;
      // std::cout<<"tail points to "<<tail<<std::endl;
    }
    else {
      std::cout<<"in else block\n";
      tail->next = std::move(new_node);
      // std::cout<<"tail->next points to "<<tail->next.get()<<std::endl;
      // std::cout<<"head->next points to "<<head->next.get()<<std::endl;
      tail->next->previous = tail;
      tail = tail->next.get();
      // std::cout<<"tail->next points to "<<tail->next.get()<<std::endl;
      // std::cout<<"head->next points to "<<head->next.get()<<std::endl;
    }

    // new_node->previous = &head;
    // if(head.next==nullptr) {
    //   new_node->next = nullptr;
    // }
    // else {
    //   new_node->next = std::move(head.next);
    //   head.next->previous = new_node.get();
    // }
    // head.next = std::move(new_node);
  }

  void display() {
    Node* current = head.get();
    while(current) {
      std::cout<<"Node is "<<current->data<<"\n";
      current = current->next.get();
    }
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
      push(r_string, (unsigned long) 500+i);
    }
  }
};

int main() {
  doubly_ll list;
  list.populate(100, 2, 8);
  list.display();
  return 1;
}
