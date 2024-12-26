#pragma once

#include "common.h"


#define ARRAY(TYPE) \
  struct {                                      \
    TYPE* items;                                \
    uint32_t length;                            \
    uint32_t capacity;                          \
  }

#define ARRAY_CLEAR(ARRAY)                                              \
  do {                                                                  \
    if ((ARRAY)->items != NULL)                                         \
      free((ARRAY)->items);                                             \
    (ARRAY)->items = NULL;                                              \
    (ARRAY)->length = 0;                                                \
    (ARRAY)->capacity = 0;                                              \
  } while (false)

// should_continue variable needed for using break inside the loop
// https://stackoverflow.com/a/400970
#define ARRAY_FOREACH(ARRAY, ITEM)                                      \
  for (int should_continue = 1, i = 0;                                  \
       should_continue && i < (ARRAY)->length;                          \
       should_continue = !should_continue, i++)                         \
    for (ITEM = &(ARRAY)->items[i];                                     \
         should_continue;                                               \
         should_continue = !should_continue)
