#include <mutex>

class hierarchical_mutex {
  std::mutex internal_mutex;
  unsigned long const hierarchy_value;
  unsigned long previous_hierarchy_value;
  static thread_local unsigned long this_thread_hierarchy_value;
  void check_for_hierarchy_violation();
  void update_hierarchy_value();
public:
  explicit hierarchical_mutex(unsigned long value);
  void lock();
  void unlock();
  bool try_lock();
};
