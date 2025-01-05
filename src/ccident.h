#if defined(__GNUC__)
  #if defined(__clang__)
    #define CCIDENT __VERSION__
  #else
    #define CCIDENT "GCC " __VERSION__
  #endif
#elif defined(__TINYC__)
  #define CCIDENT "TCC"
#else
  #define CCIDENT "unknown compiler"
#endif
