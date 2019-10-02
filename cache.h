#ifndef cache_H
#define cache_H

#include <stdlib.h>
#include <stdint.h>

#ifdef THREAD_COUNTER
    #include <pthread.h>
#endif

#include "private_structs.h"

#ifdef THREAD_COUNTER
static pthread_t thread;
static struct params_t params;

void* counter_thread();
static inline uint64_t clock_thread();
int create_counter();
void destroy_counter();
#endif /* THREAD_COUNTER */

void traverse_list_skylake(Elem *ptr);
void traverse_list_haswell(Elem *ptr);
void traverse_list_simple(Elem *ptr);
void traverse_list_asm_skylake(Elem *ptr);
void traverse_list_asm_haswell(Elem *ptr);
void traverse_list_asm_simple(Elem *ptr);
void traverse_list_rrip(Elem *ptr);
void traverse_list_to_n(Elem *ptr, int n);
void traverse_list_time(Elem *ptr, void (*trav)(Elem*));

int test_set(Elem *ptr, char *victim, void (*trav)(Elem*));
int tests(Elem *ptr, char *victim, int rep, int threshold, float ratio, void (*trav)(Elem*));
int tests_avg(Elem *ptr, char *victim, int rep, int threshold, void (*trav)(Elem*));
int test_and_time(Elem *ptr, int rep, int threshold, int ways, void (*trav)(Elem*));

int calibrate(char *victim, struct config *conf);

#endif /* cache_H */
