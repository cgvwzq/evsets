#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#ifdef __MACH__
	#include <mach/vm_statistics.h>
#endif

#include "list_utils.h"
#include "hist_utils.h"
#include "utils.h"
#include "cache.h"
#include "micro.h"
#include "algorithms.h"

#define MAX_REPS 50

int run(void);

struct config conf = {
	.rounds = 200,
	.cal_rounds = 1000000,
	.stride = 4096,
	.cache_size = 6 << 20,
	.cache_way = 12,
	.cache_slices = 4,
	.verbose = false,
	.no_huge_pages = false,
	.calibrate = true,
	.algorithm = ALGORITHM_GROUP,
	.strategy = 2,
	.offset = 0,
	.retry = false,
	.backtracking = false,
	.verify = false,
	.ignoreslice = false,
	.debug = false,
	.con = 0,
	.noncon = 0,
	.ratio = -1.0,
	.buffer_size = 3072,
	.findallcolors = false,
	.findallcongruent = false,
	.conflictset = false,
};

void
usage(char *name)
{
	printf("[?] Usage: %s [flags] [params]\n\n"
		"\tFlags:\n"
		"\t\t--nohugepages\n"
		"\t\t--retry\t(complete repetitions)\n"
		"\t\t--backtracking\n"
		"\t\t--verbose\n"
		"\t\t--verify\t(requires root)\n"
		"\t\t--ignoreslice\t(unknown slicing function)\n"
		"\t\t--findallcolors\n"
		"\t\t--findallcongruent\n"
		"\t\t--conflictset\n"
		"\tParams:\n"
		"\t\t-b N\t\tnumber of lines in initial buffer (default: 3072)\n"
		"\t\t-t N\t\tthreshold in cycles (default: calibrates)\n"
		"\t\t-c N\t\tcache size in MB (default: 6)\n"
		"\t\t-s N\t\tnumber of cache slices (default: 4)\n"
		"\t\t-n N\t\tcache associativity (default: 12)\n"
		"\t\t-o N\t\tstride for blocks in bytes (default: 4096)\n"
		"\t\t-a n|o|g|l\tsearch algorithm (default: 'g')\n"
		"\t\t-e 0|1|2|3|4\teviction strategy: 0-haswell, 1-skylake, 2-simple (default: 2)\n"
		"\t\t-C N\t\tpage offset (default: 0)\n"
		"\t\t-r N\t\tnumer of rounds per test (default: 200)\n"
		"\t\t-q N\t\tratio of success for passing a test (default: disabled)\n"
		"\t\t-h\t\tshow this help\n\n"
		"\tExample:\n\t\t%s -b 3000 -c 6 -s 8 -a g -n 12 -o 4096 -a g -e 2 -C 0 -t 85 --verbose --retry --backtracking\n"
	"\n", name, name);
}

int
main(int argc, char **argv)
{
	int option = 0, option_index = 0;

#ifdef THREAD_COUNTER
    if (create_counter ())
    {
        return 1;
    }
#endif /* THREAD_COUNTER */

	static struct option long_options[] =
	{
		{"nohugepages",		no_argument, &conf.no_huge_pages, 1},
		{"retry", 			no_argument, &conf.retry, 1},
		{"backtracking",	no_argument, &conf.backtracking, 1},
		{"verbose",			no_argument, &conf.verbose, 1},
		{"verify",			no_argument, &conf.verify, 1},
		{"debug",			no_argument, &conf.debug, 1},
		{"ignoreslice",		no_argument, &conf.ignoreslice, 1},
		{"findallcolors",	no_argument, &conf.findallcolors, 1},
		{"findallcongruent",no_argument, &conf.findallcongruent, 1},
		{"conflictset",		no_argument, &conf.conflictset, 1},
		{"buffer-size",		no_argument, 0, 'b'},
		{"threshold",		no_argument, 0, 't'},
		{"ratio",			no_argument, 0, 'q'},
		{"cache-size",		no_argument, 0, 'c'},
		{"cache-slices",	no_argument, 0, 's'},
		{"associativity",	no_argument, 0, 'n'},
		{"stride",			no_argument, 0, 'o'},
		{"algorithm",		no_argument, 0, 'a'},
		{"strategy",		no_argument, 0, 'e'},
		{"offset",			no_argument, 0, 'C'},
		{"rounds",			no_argument, 0, 'r'},
		{"help",			no_argument, 0, 'h'},
		{"con",				no_argument, 0, 'x'},
		{"noncon",			no_argument, 0, 'y'},
		{0, 0, 0, 0}
	};

	while ((option = getopt_long (argc, argv, "hb:t:q:c:s:n:o:a:e:r:C:x:y:",
			long_options, &option_index)) != -1)
	{
		switch (option)
		{
			case 0:
				break;
			case 'b':
				conf.buffer_size = atoi(optarg);
				break;
			case 't' :
				conf.calibrate = false;
				conf.threshold = atoi(optarg);
				break;
			case 'q' :
				conf.ratio = atof(optarg);
				break;
			case 'c' :
				conf.cache_size = atoi(optarg) << 20;
				break;
			case 's' :
				conf.cache_slices = atoi(optarg);
				if (conf.cache_slices < 1 || conf.cache_slices % 2)
				{
					printf ("[-] Invalid number of slices\n");
					conf.cache_slices = 1;
				}
				else if (conf.cache_slices > 8)
				{
					printf ("[-] No support for more than 8 slices\n");
					conf.cache_slices = 8;
				}
				break;
			case 'n' :
				conf.cache_way = atoi(optarg);
				if (conf.cache_way < 1)
				{
					conf.cache_way  = 1;
				}
				break;
			case 'o' :
				conf.stride = atoi(optarg);
				break;
			case 'a' :
				if (strncmp (optarg, "g", strlen (optarg)) == 0) {
					conf.algorithm = ALGORITHM_GROUP;
				} else if (strncmp (optarg, "b", strlen (optarg)) == 0) {
					conf.algorithm = ALGORITHM_BINARY;
				} else if (strncmp (optarg, "l", strlen (optarg)) == 0) {
					conf.algorithm = ALGORITHM_LINEAR;
				} else if (strncmp (optarg, "n", strlen (optarg)) == 0) {
					conf.algorithm = ALGORITHM_NAIVE;
				} else if (strncmp (optarg, "o", strlen (optarg)) == 0) {
					conf.algorithm = ALGORITHM_NAIVE_OPTIMISTIC;
				}
				break;
			case 'e' :
				conf.strategy = atoi(optarg);
				break;
			case 'C' :
				conf.offset = atoi(optarg);
				break;
			case 'r' :
				conf.rounds = atoi(optarg);
				break;
			case 'x' :
				conf.con = atoi(optarg);
				if (conf.con < 0)
				{
					conf.con = 0;
				}
				break;
			case 'y' :
				conf.noncon = atoi(optarg);
				if (conf.noncon < 0)
				{
					conf.noncon = 0;
				}
				break;
			case 'h':
				usage(argv[0]);
				return 0;
			default:
				return 1;
		}
	}

	run ();

#ifdef THREAD_COUNTER
    destroy_counter ();
#endif /* THREAD_COUNTER */

	return 0;
}

int
run(void)
{
    char *victim = NULL;
	char *addr = NULL;
	char *probe = NULL;
	char *pool = NULL;
	ul sz = conf.buffer_size * conf.stride;
	ul pool_sz = 128 << 20;
	if (sz > pool_sz)
	{
		printf("[!] Error: not enough space\n");
		return 1;
	}
	if (conf.no_huge_pages)
	{
		pool = (char*) mmap (NULL, pool_sz, PROT_READ|PROT_WRITE,
						MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
		probe = (char*) mmap (NULL, pool_sz, PROT_READ|PROT_WRITE,
						MAP_PRIVATE|MAP_ANONYMOUS, 0, 0); // add MAP_HUGETLB for testing effect of cache sets
	}
	else
	{
#ifdef MAP_HUGETLB
		pool = (char*) mmap (NULL, pool_sz, PROT_READ|PROT_WRITE,
						MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, 0, 0);
		probe = (char*) mmap (NULL, pool_sz, PROT_READ|PROT_WRITE,
						MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, 0, 0);
#elif defined VM_FLAGS_SUPERPAGE_SIZE_2MB
		/* Mac OS X specific: the file descriptor used for creating MAP_ANON
		regions can be used to pass some Mach VM flags */
		pool = (char*) mmap (NULL, pool_sz, PROT_READ|PROT_WRITE,
						MAP_PRIVATE|MAP_ANON, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
		probe = (char*) mmap (NULL, pool_sz, PROT_READ|PROT_WRITE,
						MAP_PRIVATE|MAP_ANON, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
#endif
	}

	if (pool == MAP_FAILED || addr == MAP_FAILED || probe == MAP_FAILED)
	{
		printf("[!] Error: allocation\n");
		return 1;
	}

	Elem *ptr = (Elem*)&pool[conf.offset << 6];
	Elem *can = NULL;
	victim = &probe[conf.offset << 6];
	*victim = 0; // touch line

	printf ("[+] %llu MB buffer allocated at %p (%llu blocks)\n",
			sz >> 20, (void*)ptr, sz/sizeof(Elem));

	if (conf.stride < 64 || conf.stride % 64 != 0)
	{
		printf("[!] Error: invalid stride\n");
		goto err;
	}

	int seed = time (NULL);
	srand (seed);

	// Set eviction strategy
	switch (conf.strategy)
	{
		case 0:
			conf.traverse = &traverse_list_haswell;
			break;
		case 1:
			conf.traverse = &traverse_list_skylake;
			break;
		case 3:
			conf.traverse = &traverse_list_asm_skylake;
			break;
		case 4:
			conf.traverse = &traverse_list_asm_haswell;
			break;
		case 5:
			conf.traverse = &traverse_list_asm_simple;
			break;
		case 10:
			conf.traverse = &traverse_list_rrip;
			break;
		case 2:
		default:
			conf.traverse = &traverse_list_simple;
			break;
	}

	if (conf.calibrate)
	{
		conf.threshold = calibrate (victim, &conf);
		printf("[+] Calibrated Threshold = %d\n", conf.threshold);
	}
	else
	{
		printf("[+] Default Threshold = %d\n", conf.threshold);
	}

	if (conf.threshold < 0)
	{
		printf("[!] Error: calibration\n");
		return 1;
	}

	if (conf.algorithm == ALGORITHM_LINEAR)
	{
		victim = NULL;
	}

	clock_t tts, tte;
	int rep = 0;
	tts = clock();
	pick:

	ptr = (Elem*)&pool[conf.offset << 6];
	initialize_list (ptr, pool_sz, conf.offset);

	// Conflict set incompatible with ANY case (don't needed)
	if (conf.conflictset && (conf.algorithm != ALGORITHM_LINEAR))
	{
		pick_n_random_from_list (ptr, conf.stride, pool_sz, conf.offset, conf.buffer_size);
		generate_conflict_set (&ptr, &can);
		printf ("[+] Compute conflict set: %d\n", list_length (can));
		victim = (char*)ptr;
		ptr = can; // new conflict set
		while (victim && !tests (ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse))
		{
			victim = (char*)(((Elem*)victim)->next);
		}
		can = NULL;
	}
	else
	{
		pick_n_random_from_list (ptr, conf.stride, pool_sz, conf.offset, conf.buffer_size);
		if (list_length (ptr) != conf.buffer_size)
		{
			printf("[!] Error: broken list\n");
			goto err;
		}
	}

	int ret = 0;
	if (conf.debug)
	{
		conf.verify = true;
		conf.findallcolors = false;
		conf.findallcongruent = false;
		printf ("[+] Filter: %d congruent, %d non-congruent addresses\n", conf.con, conf.noncon);
		ret = filter (&ptr, victim, conf.con, conf.noncon, &conf);
		if (ret && !conf.retry)
		{
			goto err;
		}
	}

	if (conf.algorithm == ALGORITHM_LINEAR)
	{
		ret = test_and_time (ptr, conf.rounds, conf.threshold, conf.cache_way, conf.traverse);
	}
	else if (victim)
	{
        if (conf.ratio > 0.0)
        {
            ret = tests (ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
        }
        else
        {
            ret = tests_avg (ptr, victim, conf.rounds, conf.threshold, conf.traverse);
        }
	}
	if ((victim || conf.algorithm == ALGORITHM_LINEAR) && ret)
	{
		printf("[+] Initial candidate set evicted victim\n");
		// rep = 0;
	}
	else
	{
		printf ("[!] Error: invalid candidate set\n");
		if (conf.retry && rep < MAX_REPS)
		{
			rep++;
			goto pick;
		}
		else if (rep >= MAX_REPS)
		{
			printf ("[!] Error: exceeded max repetitions\n");
		}
		if (conf.verify)
		{
			recheck (ptr, victim, true, &conf);
		}
		goto err;
	}

	clock_t ts, te;

	int len = 0, colors = conf.cache_size / conf.cache_way / conf.stride;
	Elem **result = calloc (colors, sizeof(Elem*));
	if (!result)
	{
		printf("[!] Error: allocate\n");
		goto err;
	}

	int id = 0;
	// Iterate over all colors of conf.offset
	do
	{

	printf ("[+] Created linked list structure (%d elements)\n",
			list_length (ptr));

	// Search
	switch (conf.algorithm)
	{
		case ALGORITHM_NAIVE:
			printf("[+] Starting naive reduction...\n");
			ts = clock();
			ret = naive_eviction (&ptr, &can, victim);
			te = clock();
			break;
		case ALGORITHM_NAIVE_OPTIMISTIC:
			printf("[+] Starting optimistic naive reduction...\n");
			ts = clock();
			ret = naive_eviction_optimistic (&ptr, &can, victim);
			te = clock();
			break;
		case ALGORITHM_GROUP:
			printf("[+] Starting group reduction...\n");
			ts = clock();
			ret = gt_eviction (&ptr, &can, victim);
			te = clock();
			break;
		case ALGORITHM_BINARY:
			printf("[+] Starting binary group reduction...\n");
			ts = clock();
			ret = binary_eviction (&ptr, &can, victim);
			te = clock();
			break;
		case ALGORITHM_LINEAR:
			printf("[+] Starting linear reduction...\n");
			ts = clock();
			ret = gt_eviction_any (&ptr, &can);
			te = clock();
			break;
	}

	tte = clock();

	len = list_length (ptr);
	if (ret)
	{
		printf("[!] Error: optimal eviction set not found (length=%d)\n", len);
	}
	else
	{
		printf("[+] Reduction time: %f seconds\n", ((double)(te-ts))/CLOCKS_PER_SEC);
		printf("[+] Total execution time: %f seconds\n", ((double)(tte-tts))/CLOCKS_PER_SEC);

		// Re-Check that it's an optimal eviction set
		if (conf.algorithm != ALGORITHM_LINEAR)
		{
			printf("[+] (ID=%d) Found minimal eviction set for %p (length=%d): ",
				id, (void*)victim, len);
			print_list (ptr);
		}
		else
		{
			printf("[+] (ID=%d) Found a minimal eviction set (length=%d): ", id, len);
			print_list (ptr);
		}
		result[id] = ptr;
	}

	if (conf.verify)
	{
		recheck(ptr, victim, ret, &conf);
	}

	if (ret && conf.retry)
	{
		if (rep < MAX_REPS)
		{
			list_concat (&ptr, can);
			can = NULL;
			rep++;
			if (!conf.conflictset && !conf.findallcolors)
			{
				// select a new initial set
				printf ("[!] Error: repeat, pick a new set\n");
				goto pick;
			}
			else
			{
				// reshuffle list or change victim?
				printf ("[!] Error: try new victim\n");
				goto next;
				// continue;
			}
		}
		else
		{
			printf ("[!] Error: exceeded max repetitions\n");
		}
	}
	else if (!ret)
	{
		rep = 0;
	}
	else
	{
		list_concat (&ptr, can);
		can = NULL;
	}

	// Remove rest of congruent elements
	list_set_id (result[id], id);
	ptr = can;
	if (conf.findallcongruent)
	{
		Elem *e = NULL, *head = NULL, *done = NULL, *tmp = NULL;
		int count = 0, t = 0;
		while (ptr)
		{
			e = list_pop(&ptr);
			if (conf.ratio > 0.0)
			{
				t = tests (result[id], (char*)e, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
			}
			else
			{
				t = tests_avg (result[id], (char*)e, conf.rounds, conf.threshold, conf.traverse);
			}
			if (t)
			{
				// create list of congruents
				e->set = id;
				count++;
				list_push (&head, e);
			}
			else
			{
				list_push (&done, e);
			}
		}
		if (tmp)
		{
			tmp->next = NULL;
		}
		printf ("[+] Found %d more congruent elements from set id=%d\n", count, id);
		list_concat (&result[id], head);
		ptr = done;
	}

	if (!conf.findallcolors)
	{
		break;
	}
	printf ("----------------------\n");
	id = id + 1;
	if (id == colors || !ptr ||
			(conf.conflictset && !victim) ||
			(!conf.conflictset && victim >= probe + pool_sz - conf.stride))
	{
		printf ("[+] Found all eviction sets in buffer\n");
		break;
	}

	next:
	// Find victim for different color. Only for specific algorithms.
	if (conf.algorithm != ALGORITHM_LINEAR)
	{
		int s = 0, ret = 0, ret2 = 0;
		do
		{
			if (!conf.conflictset)
			{
				victim += conf.stride;
				*victim = 0;
			}
			else
			{
				victim = (char*)((Elem*)victim)->next;
			}

			// Check again. Better reorganize this mess.
			if ((conf.conflictset && !victim) ||
				(!conf.conflictset && victim >= probe + pool_sz - conf.stride))
			{
				break;
			}

			// New victim is not evicted by previous eviction sets
			for (ret = 0, s = 0; s < id && !ret; s++)
			{
				if (conf.ratio > 0.0)
				{
					ret = tests (result[s], victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
				}
				else
				{
					ret = tests_avg (result[s], victim, conf.rounds, conf.threshold, conf.traverse);
				}
			}
			if (!ret)
			{
				// Rest of initial eviction set can evict victim
				if (conf.ratio > 0.0)
				{
					ret2 = tests (ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
				}
				else
				{
					ret2 = tests_avg (ptr, victim, conf.rounds, conf.threshold, conf.traverse);
				}
			}
		}
		while ((list_length (ptr) > conf.cache_way) && !ret2 && ((conf.conflictset && victim) ||
				(!conf.conflictset && (victim < (probe + pool_sz - conf.stride)))));

		if (ret2)
		{
			printf ("[+] Found new victim %p\n", (void*)victim);
		}
		else
		{
			printf ("[!] Error: couldn't find more victims\n");
			goto beach;
		}
	}

	can = NULL;

	}
	while ((conf.findallcolors && id < colors) || (conf.retry && rep < MAX_REPS));

	// Free
	free (result);
	munmap (probe, pool_sz);
	munmap (pool, pool_sz);

	return ret;

	beach:
	free (result);

	err:
	munmap (probe, pool_sz);
	munmap (pool, pool_sz);
	return 1;
}
