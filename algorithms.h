#ifndef algorithms_H
#define algorithms_H

#include "cache.h"

int naive_eviction(Elem **ptr, Elem **can, char *victim);
int naive_eviction_optimistic(Elem **ptr, Elem **can, char *victim);
int gt_eviction(Elem **ptr, Elem **can, char *victim);
int gt_eviction_any(Elem **ptr, Elem **can);
int binary_eviction(Elem **ptr, Elem **can, char *victim);

#endif /* algorithms_H */
