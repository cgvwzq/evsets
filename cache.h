#ifndef cache_H
#define cache_H

#include <stdlib.h>
#include <stdint.h>

#ifdef THREAD_COUNTER
    #include <pthread.h>
#endif

#define ALGORITHM_NAIVE 0
#define ALGORITHM_GROUP 1
#define ALGORITHM_BINARY 2
#define ALGORITHM_LINEAR 3
#define ALGORITHM_NAIVE_OPTIMISTIC 4

typedef struct elem
{
	struct elem *next;
	struct elem *prev;
	int set;
	size_t delta;
	char pad[32]; // up to 64B
} Elem;

struct
config
{
	int rounds, cal_rounds;
	int stride;
	int cache_size;
	int buffer_size;
	int cache_way;
	int cache_slices;
	int threshold;
	int algorithm;
	int strategy;
	int offset;
	int con, noncon; // only for debug
    void (*traverse)(Elem*);
	double ratio;
	int verbose, no_huge_pages, calibrate, retry, backtracking, ignoreslice, findallcolors, findallcongruent, verify, debug, conflictset;
};

#ifdef THREAD_COUNTER
struct params_t {
	pthread_mutex_t lock;
	uint64_t counter;
};

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
