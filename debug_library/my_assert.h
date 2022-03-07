#ifndef _MY_ASSERT_H
#define _MY_ASSERT_H

// #define _USE_ASSERT    1

/* Exported macro -------------------------------*/
#ifdef  _USE_ASSERT
/**
  * @brief  The __assert macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function which reports 
  *         the name of the source file and the source line number of the call 
  *         that failed. If expr is true, it returns no value.
  * @retval None
  */
  #define __assert(expr) ((expr) ? (void)0 : _assert_failed((char *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
  void _assert_failed(char* file, unsigned int line);
#else
  #define __assert(expr) ((void)0)
#endif /* _USE_ASSERT */

#ifdef _USER_DEBUG
  #include "stdio.h"
  #define my_debug(...) printf(__VA_ARGS__)
#else
  #define my_debug(...)
#endif

#endif
