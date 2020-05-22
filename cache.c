#include "cache.h"
#include "micro.h"
#include "hist_utils.h"
#include "public_structs.h"

#ifdef THREAD_COUNTER
	#include <pthread.h>
	#include <signal.h>
#endif

#ifdef THREAD_COUNTER
void* counter_thread()
{
	while (1)
	{
		__asm__ volatile ("mfence");
        params.counter++;
		__asm__ volatile ("mfence");
	}
	pthread_exit(NULL);
}

static inline uint64_t
clock_thread()
{
	uint64_t ret;
	__asm__ volatile ("mfence");
	ret = params.counter;
	__asm__ volatile ("mfence");
	return ret;
}

int
create_counter()
{
    if (pthread_create (&thread, NULL, counter_thread, &params))
    {
        printf("[!] Error: thread counter\n");
        return 1;
    }
    return 0;
}

void destroy_counter()
{
	pthread_kill(thread, 0);
}
#endif

inline
void
traverse_list_skylake(Elem *ptr)
{
	while (ptr && ptr->next && ptr->next->next)
	{
		maccess (ptr);
		maccess (ptr->next);
		maccess (ptr->next->next);
		maccess (ptr);
		maccess (ptr->next);
		maccess (ptr->next->next);
		ptr = ptr->next;
	}
}

inline
void
traverse_list_asm_skylake(Elem *ptr)
{
	__asm__ volatile
	(
		"test %%rcx, %%rcx;"
		"jz out;"
		"loop:"
		"movq (%%rcx), %%rax;"
		"test %%rax, %%rax;"
		"jz out;"
		"movq (%%rax), %%rax;"
		"test %%rax, %%rax;"
		"jz out;"
		"movq (%%rax), %%rax;"
		"movq (%%rcx), %%rcx;"
		"movq (%%rcx), %%rax;"
		"movq (%%rax), %%rax;"
		"test %%rcx, %%rcx;"
		"jnz loop;"
		"out:"
		: // no output
		: "c" (ptr)
		: "cc", "memory"
	);
}

inline
void
traverse_list_asm_haswell(Elem *ptr)
{
	__asm__ volatile
	(
		"test %%rcx, %%rcx;"
		"jz out2;"
		"loop2:"
		"movq (%%rcx), %%rax;"
		"test %%rax, %%rax;"
		"jz out2;"
		"movq (%%rax), %%rax;"
		"movq (%%rcx), %%rcx;"
		"movq (%%rcx), %%rax;"
		"test %%rcx, %%rcx;"
		"jnz loop2;"
		"out2:"
		: // no output
		: "c" (ptr)
		: "cc", "memory"
	);
}

inline
void
traverse_list_asm_simple(Elem *ptr)
{
	__asm__ volatile
	(
		"loop3:"
		"test %%rcx, %%rcx;"
		"jz out3;"
		"movq (%%rcx), %%rcx;"
		"jmp loop3;"
		"out3:"
		: // no output
		: "c" (ptr)
		: "cc", "memory"
	);
}

inline
void
traverse_list_haswell(Elem *ptr)
{
	while (ptr && ptr->next)
	{
		maccess (ptr);
		maccess (ptr->next);
		maccess (ptr);
		maccess (ptr->next);
		ptr = ptr->next;
	}
}

inline
void
traverse_list_simple(Elem *ptr)
{
	while (ptr)
	{
		maccess (ptr);
		ptr = ptr->next;
	}
}

inline
void
traverse_list_rrip(Elem *ptr)
{
	Elem *p, *s = ptr;
	while (ptr)
	{
		p = ptr;
		maccess (ptr);
		maccess (ptr);
		maccess (ptr);
		maccess (ptr);
		ptr = ptr->next;
	}
	while (p != s)
	{
		maccess (p);
		maccess (p);
		p = p->prev;
	}
	maccess (p);
	maccess (p);
}

inline
void
traverse_list_to_n(Elem *ptr, int n)
{
	while (ptr && n-- > 0)
	{
		maccess (ptr);
		ptr = ptr->next;
	}
}

inline
void
traverse_list_time (Elem *ptr, void (*trav)(Elem*))
{
	size_t time;
	trav (ptr);
	while (ptr)
	{
//		time = rdtsc();
		time = rdtscfence();
		maccess (ptr);
		ptr->delta += rdtscfence() - time;
//		ptr->delta += rdtscp() - time;
		ptr = ptr->next;
	}
}


int
test_set(Elem *ptr, char *victim, void (*trav)(Elem*))
{
	maccess (victim);
	maccess (victim);
	maccess (victim);
	maccess (victim);

	trav (ptr);

	maccess (victim + 222); // page walk

	size_t delta, time;
#ifndef THREAD_COUNTER
//	time = rdtsc();
	time = rdtscfence();
	maccess (victim);
//	delta = rdtscp() - time;
	delta = rdtscfence() - time;
#else
	time = clock_thread();
	maccess (victim);
	delta = clock_thread() - time;
#endif
	return delta;
}

int
test_and_time(Elem *ptr, int rep, int threshold, int ways, void (*trav)(Elem*))
{
	int i = 0, count = 0;
	Elem *tmp = ptr;
	while (tmp)
	{
		tmp->delta = 0;
		tmp = tmp->next;
	}
	for (i = 0; i < rep; i++)
	{
		tmp = ptr;
		traverse_list_time (tmp, trav);
	}
	while (ptr)
	{
		ptr->delta = (float)ptr->delta / rep;
		if (ptr->delta > (unsigned)threshold)
		{
			count++;
		}
		ptr = ptr->next;
	}
	return count > ways;
}

int
tests_avg(Elem *ptr, char *victim, int rep, int threshold, void (*trav)(Elem*))
{
	int i = 0, ret =0, delta = 0;
	Elem *vic = (Elem*)victim;
	vic->delta = 0;
	for (i=0; i < rep; i++)
	{
		delta = test_set (ptr, victim, trav);
		if (delta < 800) vic->delta += delta;
	}
	ret	= (float)vic->delta / rep;
	return ret > threshold;
}

int
tests(Elem *ptr, char *victim, int rep, int threshold, float ratio, void (*trav)(Elem*))
{
	int i = 0, ret = 0, delta, hsz = rep * 100;
	struct histogram *hist;
	if ((hist = (struct histogram*) calloc (hsz, sizeof(struct histogram)))
		== NULL)
	{
		return 0;
	}
	for (i=0; i < rep; i++)
	{
        delta = test_set (ptr, victim, trav);
        hist_add (hist, hsz, delta);
	}
	ret = hist_q (hist, hsz, threshold);
	free (hist);
    return ret > (int)(rep * ratio);
}

int
calibrate(char *victim, struct config *conf)
{
	size_t delta, time, t_flushed, t_unflushed;
	struct histogram *flushed, *unflushed;
	int i, ret, hsz = conf->cal_rounds * 100;

	flushed = (struct histogram*) calloc (hsz, sizeof(struct histogram));
	unflushed = (struct histogram*) calloc (hsz, sizeof(struct histogram));

	if (flushed == NULL || unflushed == NULL)
	{
		return -1;
	}

	for (i=0; i < conf->cal_rounds; i++)
	{
		maccess (victim);
		maccess (victim);
		maccess (victim);
		maccess (victim);

		maccess (victim + 222); // page walk

#ifndef THREAD_COUNTER
//		time = rdtsc();
		time = rdtscfence();
		maccess (victim);
//		delta = rdtscp() - time;
		delta = rdtscfence() - time;
#else
		time = clock_thread();
		maccess (victim);
		delta = clock_thread() - time;
#endif
		hist_add (unflushed, hsz, delta);
	}
	t_unflushed = hist_avg (unflushed, hsz);

	for (i=0; i < conf->cal_rounds; i++)
	{
		maccess (victim); // page walk
		flush (victim);

#ifndef THREAD_COUNTER
//		time = rdtsc();
		time = rdtscfence();
		maccess (victim);
//		delta = rdtscp() - time;
		delta = rdtscfence() - time;
#else
		time = clock_thread();
		maccess (victim);
		delta = clock_thread() - time;
#endif
		hist_add (flushed, hsz, delta);
	}
	t_flushed = hist_avg (flushed, hsz);

	ret = hist_min (flushed, hsz);

	if (conf->flags & FLAG_VERBOSE)
	{
		printf("\tflushed: min %d, mode %d, avg %f, max %d, std %.02f, q %d (%.02f)\n",
			hist_min (flushed, hsz), hist_mode(flushed, hsz),
			hist_avg (flushed, hsz), hist_max (flushed, hsz),
			hist_std (flushed, hsz, hist_avg (flushed, hsz)),
			hist_q (flushed, hsz, ret),
			(double) hist_q (flushed, hsz, ret) / conf->cal_rounds);
		printf("\tunflushed: min %d, mode %d, avg %f, max %d, std %.02f, q %d (%.02f)\n",
			hist_min (unflushed, hsz), hist_mode(unflushed, hsz),
			hist_avg (unflushed, hsz), hist_max (unflushed, hsz),
			hist_std (unflushed, hsz, hist_avg (unflushed, hsz)),
			hist_q (unflushed, hsz, ret),
			(double) hist_q (unflushed, hsz, ret) / conf->cal_rounds);
	}

	free (unflushed);
	free (flushed);

	if (t_flushed < t_unflushed)
	{
		return -1;
	} else {
		return (t_flushed + t_unflushed * 2) / 3;
	}
}
