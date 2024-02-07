#pragma once
#include <omp.h>
#include <mutex>

namespace omp {
  //! мютекс из OMP
  class mutex final {
  private:
    omp_lock_t writelock = {};
  public:
    mutex() { omp_init_lock(&writelock); }
    ~mutex() { omp_destroy_lock(&writelock); }
    void lock() { omp_set_lock(&writelock); }
    void unlock() { omp_unset_lock(&writelock); }
  }; // mutex
  
  using lock_guard = std::lock_guard<omp::mutex>;
} // omp ns