#include "mempool.hpp"

#ifndef ECOMEM
  #ifdef DETAILED_LOG
    #include <atomic>
    #include "util/log.hpp"
    #include "util/str-util.hpp"

    inline std::atomic_uint mem_pool_cur_bytes_used {0};
    inline std::atomic_uint mem_pool_max_bytes_used {0};
  #endif
#endif

Mem_pool::Mem_pool(std::size_t chunk_sz)
#ifndef ECOMEM
: source(chunk_sz)
#endif
{}

void Mem_pool::release() {
#ifndef ECOMEM
  for (cnauto deleter: deleters)
    deleter();
  deleters.clear();
  source.release();
#endif

  m_allocated = 0;
}

Mem_pool::~Mem_pool() {
#ifndef ECOMEM
  release();
  print_used_bytes();
#endif
}

void Mem_pool::add_used_bytes(std::size_t sz) {
#ifndef ECOMEM
#ifdef DETAILED_LOG
  mem_pool_cur_bytes_used += sz;
  mem_pool_max_bytes_used.store( std::max(
    mem_pool_max_bytes_used,
    mem_pool_cur_bytes_used )
  );
#endif
#endif
}

void Mem_pool::sub_used_bytes(std::size_t sz) {
#ifndef ECOMEM
#ifdef DETAILED_LOG
  mem_pool_cur_bytes_used -= sz;
#endif
#endif
}

void Mem_pool::print_used_bytes() {
#ifndef ECOMEM
#ifdef DETAILED_LOG
  detailed_log("max mem usage in pool: " << mem_pool_max_bytes_used << " (" <<
    n2s(scast<double>(mem_pool_max_bytes_used) / (1024*1024), 3)
    << " mb)\n");
#endif
#endif
}

