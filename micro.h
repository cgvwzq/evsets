#ifndef micro_H
#define micro_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "cache.h"

#define LINE_BITS 6
#define PAGE_BITS 12
#define LINE_SIZE (1 << LINE_BITS)
#define PAGE_SIZE2 (1 << PAGE_BITS)

typedef unsigned long long int ul;

inline
void
flush(void *p)
{
	__asm__ volatile ("clflush 0(%0)" : : "c" (p) : "rax");
}

inline
uint64_t
rdtsc()
{
	unsigned a, d;
	__asm__ volatile ("cpuid\n"
	"rdtsc\n"
	"mov %%edx, %0\n"
	"mov %%eax, %1\n"
	: "=r" (a), "=r" (d)
	:: "%rax", "%rbx", "%rcx", "%rdx");
	return ((uint64_t)a << 32) | d;
}

inline
uint64_t
rdtscp()
{
	unsigned a, d;
	__asm__ volatile("rdtscp\n"
	"mov %%edx, %0\n"
	"mov %%eax, %1\n"
	"cpuid\n"
	: "=r" (a), "=r" (d)
	:: "%rax", "%rbx", "%rcx", "%rdx");
	return ((uint64_t)a << 32) | d;
}

inline
uint64_t
rdtscfence()
{
	uint64_t a, d;
	__asm__ volatile ("lfence");
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d) : :);
	__asm__ volatile ("lfence");
	return ((d<<32) | a);
}

inline
void
maccess(void* p)
{
	__asm__ volatile ("movq (%0), %%rax\n" : : "c" (p) : "rax");
}

ul vtop(ul vaddr);
ul ptos(ul paddr, ul slicebits);
void recheck(Elem *ptr, char *victim, bool err, struct config *conf);
int filter(Elem **ptr, char *vicitm, int n, int m, struct config *conf);

#endif /* micro_H */
