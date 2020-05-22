#include "algorithms.h"
#include "list_utils.h"
#include "utils.h"
#include "public_structs.h"
#include <math.h>
#include <stdio.h>

#define MAX_REPS_BACK 100

extern struct config conf;

int
naive_eviction(Elem **ptr, Elem **can, char *victim)
{
	Elem *candidate = NULL;
	int len = 0, cans = 0, i = 0, fail = 0, ret = 0, repeat = 0;

	len = list_length (*ptr);
	cans = list_length (*can);

	while (len > conf.cache_way)
	{
        if (conf.ratio > 0.0)
        {
            ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
        }
        else
        {
            ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
        }
        if (ret)
		{
			candidate = list_pop (ptr);
			list_push (can, candidate);
			fail = 0;
		}
		else if (!cans)
		{
			break;
		}
		else
		{
			// candidate is part of the eviction set, put it back at the end
			candidate = list_pop (can);
			list_append (ptr, candidate);
			if (fail)
			{
				// step back in decision binary tree by readding previous candidate
				if (!(conf.flags & FLAG_BACKTRACKING) || repeat > MAX_REPS_BACK)
				{
					break;
				}
				repeat++;
				if (conf.flags & FLAG_VERBOSE)
				{
					printf("\tbacktrack one step\n");
				}
			}
			fail = 1;
		}

		len = list_length (*ptr);
		cans = list_length (*can);

		if ((conf.flags & FLAG_VERBOSE) && !(i++ % 300))
		{
			printf("\teset=%d, removed=%d (%d)\n", len, cans, len+cans);
		}
	}

	if (conf.flags & FLAG_VERBOSE)
	{
		printf("\teset=%d, removed=%d (%d)\n", len, cans, len+cans);
	}

    if (conf.ratio > 0.0)
    {
        ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
    }
    else
    {
        ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
    }
	if (ret)
	{
		// Not fully reduced (exceed backtrack steps)
		if (len > conf.cache_way)
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}
	return 0;
}

int
naive_eviction_optimistic(Elem **ptr, Elem **can, char *victim)
{
	Elem *candidate = NULL, *es = NULL;
	int len = 0, cans = 0, elen = 0, i = 0, ret = 0;

	len = list_length (*ptr);

	while (elen < conf.cache_way && len > conf.cache_way)
	{
		candidate = list_pop (ptr);

        if (conf.ratio > 0.0)
        {
            ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
        }
        else
        {
            ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
        }
        if (ret)
		{
			// list still is an eviction set of victim
			// discard candidate
			list_push (can, candidate);
		}
		else
		{
			// candidate is congruent, keep it
			elen++;
			if (!es)
			{
				// pointer to eviction set sublist
				es = candidate;
			}
			list_append (ptr, candidate);
		}

		len = list_length (*ptr);
		cans = list_length (*can);

		if ((conf.flags & FLAG_VERBOSE) && !(i++ % 300))
		{
			printf("\teset=%d, removed=%d (%d)\n", len, cans, len+cans);
		}
	}

	if (conf.flags & FLAG_VERBOSE)
	{
		printf("\teset=%d, removed=%d (%d)\n", len, cans, len+cans);
	}

	list_concat (can, *ptr);

    if (elen < conf.cache_way)
	{
		*ptr = NULL;
		return 1;
	}
	else
	{
		es->prev->next = NULL;
		es->prev = NULL;
		*ptr = es;
	}

    if (conf.ratio > 0.0)
    {
        ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
    }
    else
    {
        ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
    }
	return !ret;
}

int
gt_eviction(Elem **ptr, Elem **can, char *victim)
{
	// Random chunk selection
	Elem **chunks = (Elem**) calloc (conf.cache_way + 1, sizeof (Elem*));
	if (!chunks)
	{
		return 1;
	}
	int *ichunks = (int*) calloc (conf.cache_way + 1, sizeof (int)), i;
	if (!ichunks)
	{
		free (chunks);
		return 1;
	}

	int len = list_length (*ptr), cans = 0;

	// Calculate length: h = log(a/(a+1), a/n)
	double sz = (double)conf.cache_way / len;
	double rate = (double)conf.cache_way / (conf.cache_way + 1);
	int h = ceil(log(sz) / log(rate)), l = 0;

	// Backtrack record
	Elem **back = (Elem**) calloc (h * 2, sizeof (Elem*)); // TODO: check height bound
	if (!back)
	{
		free (chunks);
		free (ichunks);
		return 1;
	}

	int repeat = 0;
	do {

		for (i=0; i < conf.cache_way + 1; i++)
		{
			ichunks[i] = i;
		}
		shuffle (ichunks, conf.cache_way + 1);

		// Reduce
		while (len > conf.cache_way)
		{

			list_split (*ptr, chunks, conf.cache_way + 1);
			int n = 0, ret = 0;

			// Try paths
			do
			{
				list_from_chunks (ptr, chunks, ichunks[n], conf.cache_way + 1);
				n = n + 1;
				if (conf.ratio > 0.0)
				{
					ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
				}
				else
				{
					ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
				}
			}
			while (!ret && (n < conf.cache_way + 1));

			// If find smaller eviction set remove chunk
			if (ret && n <= conf.cache_way)
			{
				back[l] = chunks[ichunks[n-1]]; // store ptr to discarded chunk
				cans += list_length (back[l]); // add length of removed chunk
				len = list_length (*ptr);

				if (conf.flags & FLAG_VERBOSE)
				{
					printf("\tlvl=%d: eset=%d, removed=%d (%d)\n", l, len, cans, len+cans);
				}

				l = l + 1; // go to next lvl
			}
			// Else, re-add last removed chunk and try again
			else if (l > 0)
			{
				list_concat (ptr, chunks[ichunks[n-1]]); // recover last case
				l = l - 1;
				cans -= list_length (back[l]);
				list_concat (ptr, back[l]);
				back[l] = NULL;
				len = list_length (*ptr);
				goto mycont;
			}
			else
			{
				list_concat (ptr, chunks[ichunks[n-1]]); // recover last case
				break;
			}
		}

		break;
		mycont:
			if (conf.flags & FLAG_VERBOSE)
			{
				printf("\tbacktracking step\n");
			}

	} while (l > 0 && repeat++ < MAX_REPS_BACK && (conf.flags & FLAG_BACKTRACKING));

	// recover discarded elements
	for (i = 0; i < h * 2; i++)
	{
		list_concat (can, back[i]);
	}

	free (chunks);
	free (ichunks);
	free (back);

    int ret = 0;
    if (conf.ratio > 0.0)
    {
        ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
    }
    else
    {
        ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
    }
    if (ret)
	{
		if (len > conf.cache_way)
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

int
gt_eviction_any(Elem **ptr, Elem **can)
{
	Elem **chunks = (Elem**) calloc (conf.cache_way + 2, sizeof (Elem*));
	if (!chunks)
	{
		return 1;
	}
	// Random chunk selection
	int *ichunks = (int*) calloc (conf.cache_way + 2, sizeof (int)), i;
	if (!ichunks)
	{
		free (chunks);
		return 1;
	}

	int len = list_length (*ptr), cans = 0;

	// Calculate length: h = log(a/(a+1), a/n)
	double sz = (double)(conf.cache_way + 1) / len;
	double rate = (double)(conf.cache_way + 1) / (conf.cache_way + 2);
	int h = ceil(log(sz) / log(rate)), l = 0;

	// Backtrack record
	Elem **back = calloc (h * 2, sizeof (Elem*)); // TODO: check height bound
	if (!back)
	{
		free (chunks);
		free (ichunks);
		return 1;
	}

	int repeat = 0;
	do {
		for (i=0; i < conf.cache_way + 2; i++)
		{
			ichunks[i] = i;
		}
		shuffle (ichunks, conf.cache_way + 2);

		while (len > conf.cache_way + 1)
		{

			list_split (*ptr, chunks, conf.cache_way + 2);
			int n = 0, ret = 0;

			do
			{
				list_from_chunks (ptr, chunks, ichunks[n], conf.cache_way + 2);
				n = n + 1;
			}
			while (!(ret = (test_and_time (*ptr, conf.rounds, conf.threshold, conf.cache_way, conf.traverse))) && (n < conf.cache_way + 2));

			// If find smaller eviction set remove chunk
			if (ret && n <= conf.cache_way + 1)
			{
				back[l] = chunks[ichunks[n-1]]; // store ptr to discarded chunk
				cans += list_length (back[l]); // add length of removed chunk
				len = list_length (*ptr);

				if (conf.flags & FLAG_VERBOSE)
				{
					printf("\tlvl=%d: eset=%d, removed=%d (%d)\n", l, len, cans, len+cans);
				}

				l = l + 1; // go to next lvl
			}
			// Else, re-add last removed chunk and try again
			else if (l > 0)
			{
				list_concat (ptr, chunks[ichunks[n-1]]); // recover last case
				l = l - 1;
				cans -= list_length (back[l]);
				list_concat (ptr, back[l]);
				back[l] = NULL;
				len = list_length (*ptr);
				goto mycont;
			}
			else
			{
				break;
			}
		}

		break;
		mycont:
			if (conf.flags & FLAG_VERBOSE)
			{
				printf("\tbacktracking step\n");
			}

	}
	while (l > 0 && repeat++ < MAX_REPS_BACK && (conf.flags & FLAG_BACKTRACKING));

	// recover discarded elements
	for (i = 0; i < h * 2; i++)
	{
		list_concat (can, back[i]);
	}

	free (chunks);
	free (ichunks);
	free (back);

	if (test_and_time (*ptr, conf.rounds, conf.threshold, conf.cache_way, conf.traverse))
	{
		if (len > conf.cache_way + 1)
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

int
binary_eviction(Elem **ptr, Elem **can, char *victim)
{
	// shameful inneficient implementation with lists...
	// any good way to add backtracking?
	int olen = list_length (*ptr), len, cans, count = 0, i = 0, ret = 0;
	double x = 0, pivot = 0, laste = 0, lastn = 0;
	Elem *positive = NULL;
	while (count < conf.cache_way)
	{
		x = 1;
		laste = (double)olen;
		lastn = 0;
		pivot = 0;
		i = 1;
		while (fabs(lastn - laste) > 1 && x < olen)
		{
			i = i << 1;
			pivot = ceil (x * (olen - conf.cache_way + 1) / i);
			*can = list_slice (ptr, conf.cache_way - 2 + (unsigned int)pivot + 1, olen - 1);
			len = list_length (*ptr);
			cans = list_length (*can);
            if (conf.ratio > 0.0)
            {
			    ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
            }
            else
            {
			    ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
            }
            if (ret)
			{
				laste = pivot;
				x = 2 * x - 1;
			}
			else
			{
				lastn = pivot;
				x = 2 * x + 1;
			}
			if (conf.flags & FLAG_VERBOSE)
			{
				printf("\telem==%d eset=%d res=%d (%d)\n", count, len, cans, len+cans);
			}
			list_concat (ptr, *can);
			*can = NULL;
		}
		if (pivot + conf.cache_way > olen)
		{
			printf("[-] Something wrong, quitting\n");
			return 1;
		}
		positive = list_get (ptr, conf.cache_way - 2 + (unsigned int)laste);
		list_push (ptr, positive); // re-arrange list for next round (element to head)
		count = count + 1;
	}
	*can = list_slice (ptr, conf.cache_way, len+cans-1);
    if (conf.ratio > 0.0)
    {
        ret = tests (*ptr, victim, conf.rounds, conf.threshold, conf.ratio, conf.traverse);
    }
    else
    {
        ret = tests_avg (*ptr, victim, conf.rounds, conf.threshold, conf.traverse);
    }
	return !ret;
}
