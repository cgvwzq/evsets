#ifndef public_structs_H
#define public_structs_H

#define ALGORITHM_NAIVE			0
#define ALGORITHM_GROUP			1
#define ALGORITHM_BINARY		2
#define ALGORITHM_LINEAR		3
#define ALGORITHM_NAIVE_OPTIMISTIC	4

#define STRATEGY_HASWELL		0
#define STRATEGY_SKYLAKE		1
#define STRATEGY_ASMSKY			3
#define STRATEGY_ASMHAS			4
#define STRATEGY_ASM			5
#define STRATEGY_RRIP			10
#define STRATEGY_SIMPLE			2

#define FLAG_VERBOSE			(1<<0)
#define FLAG_NOHUGEPAGES		(1<<1)
#define FLAG_CALIBRATE			(1<<2)
#define FLAG_RETRY				(1<<3)
#define FLAG_BACKTRACKING		(1<<4)
#define FLAG_IGNORESLICE		(1<<5)
#define FLAG_FINDALLCOLORS		(1<<6)
#define FLAG_FINDALLCONGRUENT	(1<<7)
#define FLAG_VERIFY				(1<<8)
#define FLAG_DEBUG				(1<<9)
#define FLAG_CONFLICTSET		(1<<10)

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
	int flags;
};

#endif /* public_structs_H */
