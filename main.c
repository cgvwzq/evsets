#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#ifdef __MACH__
	#include <mach/vm_statistics.h>
#endif

#include "cache.h"
#include "evsets_api.h"

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

	return find_evsets (&conf);
}
