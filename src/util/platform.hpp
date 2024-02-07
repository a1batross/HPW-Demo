#pragma once

// разделители файловых систем:
#ifdef WINDOWS
  #define SEPARATOR '\\'
  #define NSEPARATOR '/'
// LINUX
#else
  #define SEPARATOR '/'
  #define NSEPARATOR '\\'
#endif

// узнать какая разрядность:
#define is_x64 __x86_64__
#define is_x32 __i386__

#if defined(__clang__)
  #define COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
  #define COMPILER_GCC
#elif defined(_MSC_VER)
  #define COMPILER_MSC
  #pragma GCC diagnostic warning "not tested on MSC!"
#endif
