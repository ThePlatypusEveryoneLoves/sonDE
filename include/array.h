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

#define ARRAY_FOREACH_REVERSE(ARRAY, ITEM)                              \
  for (int should_continue = 1, i = (ARRAY)->length - 1;                \
       should_continue && i >= 0;                                       \
       should_continue = !should_continue, i--)                         \
    for (ITEM = &(ARRAY)->items[i];                                     \
         should_continue;                                               \
         should_continue = !should_continue)

#define ARRAY_REALLOC(ARRAY, NEW_CAPACITY, ITEM_SIZE)                   \
  do {                                                                  \
    (ARRAY)->items = realloc((ARRAY)->items, (NEW_CAPACITY) * (ITEM_SIZE)); \
    (ARRAY)->capacity = (NEW_CAPACITY);                                 \
    /* update length if it needs to decrease */                         \
    if ((ARRAY)->length > (ARRAY)->capacity) (ARRAY)->length = (ARRAY)->capacity; \
  } while(false)

#define ARRAY_APPEND(ARRAY, ITEM)                                       \
  do {                                                                  \
    if ((ARRAY)->length == (ARRAY)->capacity) {                         \
      /* double the capacity */                                         \
      ARRAY_REALLOC(ARRAY, (ARRAY)->capacity == 0 ? 8 : (ARRAY)->capacity * 2, sizeof(ITEM)); \
    }                                                                   \
    /* append an item */                                                \
    (ARRAY)->items[(ARRAY)->length++] = (ITEM);                         \
  } while(false)
