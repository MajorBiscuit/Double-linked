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
    if (this_thread_hierarchy_value < hierarchy_value) {
      std::cout<<"this_thread_hierarchy_value "<<this_thread_hierarchy_value<<std::endl;
      std::cout<<"hierarchy_value "<<hierarchy_value<<std::endl;
      std::cout<<"previous_hierarchy_value "<<previous_hierarchy_value<<std::endl;
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
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> previous;
    Node(unsigned long m_order)
      : m(m_order), mutex_order(m_order), next(nullptr), previous(nullptr)
    {}
    Node(unsigned long m_order, std::string const& value)
      : m(m_order), mutex_order(m_order), data(value), next(nullptr), previous(nullptr)
    {}
  };
  // variable declaration in class. Valid in C++11
  std::shared_ptr<Node> head{new Node(150000)};
  std::shared_ptr<Node> tail{new Node(5000)};
  size_t size;
  std::string random_string( size_t length, std::function<char(void)> rand_char ) {
    std::string str(length, 0);
    std::generate_n(str.begin(), length, rand_char);
    return str;
  }

public:
  doubly_ll()
    : size(0) {}

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
    if(!head->next) {
      // std::unique_ptr is non-assignable and non-copyable
      std::cout<<"in !head clause\n";
      std::unique_lock<hierarchical_mutex> tail_l(tail->m);
      std::cout<<"acquired lock on tail mutex\n";
      new_node->next = tail;
      new_node->previous = head;
      tail->previous = std::move(new_node);
      head->next = tail->previous;
      std::cout<<"head points to "<<head.get()<<std::endl;
      std::cout<<"tail points to "<<tail<<std::endl;
    }
    else {
      std::cout<<"in else block\n";
      std::unique_lock<hierarchical_mutex> next_l(head->next->m);
      std::cout<<"acquired lock on next\n";
      new_node->next = head->next;
      new_node->previous = head;
      head->next = std::move(new_node);
      // std::cout<<"tail->next points to "<<tail->next.get()<<std::endl;
      // std::cout<<"head->next points to "<<head->next.get()<<std::endl;
      // std::cout<<"tail->next points to "<<tail->next.get()<<std::endl;
      // std::cout<<"head->next points to "<<head->next.get()<<std::endl;
    }
    size++;
  }

  void display() {
    Node* current = head->next.get();
    unsigned int count{0};
    while(current && (current != tail.get())) {
      std::cout<<"Node "<<count<<" is "<<current->data<<"\n";
      current = current->next.get();
      count++;
    }
    std::cout<<"count is "<<count<<std::endl;
    // std::cout<<"head is "<<head->data<<std::endl;
    // std::cout<<"next of head is "<<head->next->data<<std::endl;
    // std::cout<<"head mutex order is "<<head->mutex_order<<std::endl;
    // std::cout<<"tail is "<<tail->data<<std::endl;
    // std::cout<<"tail mutex order is "<<tail->mutex_order<<std::endl;
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
      push(r_string, (unsigned long) 10000+i);
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
      // not strictly necessary since tail has an empty string
      if (current->next.get() == tail.get()) break;
    }
    std::cout<<"Concatenated string is "<<cat<<std::endl;
  }

  size_t length() {
    return size;
  }


  // std::shared_ptr find_node(unsigned int index) {
  //   Node* current = head->next.get();
  //   unsigned int count{1};
  //   Node* result = nullptr;
  //   while(current && (current != tail.get())) {
  //     std::cout<<"Node "<<count<<" is "<<current->data<<"\n";
  //     if (count == index) {
  //       return current;
  //     }
  //     else {
  //     current = current->next.get();
  //     count++;
  //     }
  //   }
  // }
  void remove(unsigned int index) {
    std::shared_ptr<Node> current = head;
    unsigned int count{1};
    std::unique_lock<hierarchical_mutex> current_l(current->m);
    std::cout<<"acquired lock on head\n";
    while(current->next && (current->next != tail)) {
      std::unique_lock<hierarchical_mutex> next_l(current->next->m);
      if(count == index) {
        std::unique_lock<hierarchical_mutex> next_next_l(current->next->next->m);
        std::shared_ptr<Node> old = current->next;
        //old = std::move(current->next->next->previous);
        current->next->next->previous = current;
        current->next = current->next->next;
        next_next_l.unlock();
        next_l.unlock();
        size--;
        return;
      }
      current_l.unlock();
      current = current->next;
      current_l = std::move(next_l);
      count++;
    }
  }
};

int main() {
  doubly_ll list;
  //list.push("test1", 500);
  list.populate(5, 2, 8);
  //list.push("test2", 2000);
  list.display();
  list.remove(5);
  list.display();
  list.remove(3);
  list.display();
  list.remove(3);
  list.display();
  list.remove(1);
  list.remove(1);
  list.display();
  //list.remove(5);
  //list.display();
  int length = list.length();
  std::cout<<"length of list is "<<length<<std::endl;
  return 1;
}
