#ifndef private_structs_H
#define private_structs_H

#include "public_structs.h"

#ifdef THREAD_COUNTER
struct params_t {
	pthread_mutex_t lock;
	uint64_t counter;
};
#endif

#endif /* private_structs_H */
