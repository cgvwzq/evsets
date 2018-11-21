#include "hist_utils.h"
#include <stdio.h>
#include <math.h>

void
hist_add(struct histogram *hist, int len, size_t val)
{
	// remove outliers
	int j = val;
	if (j < 800) // remove outliers
	{
		while (hist[j % len].val > 0 && hist[j % len].val != (int)val) {
			j++;
		}
		hist[j % len].val = val;
		hist[j % len].count++;
	}
}

float
hist_avg(struct histogram *hist, int len) {
	float total = 0;
	int i = 0, n = 0;
	for (i=0; i < len; i++) {
		if (hist[i].val > 0) {
			total += hist[i].val * hist[i].count;
			n += hist[i].count;
		}
	}
	return (float)(total / n);
}

int
hist_mode(struct histogram *hist, int len)
{
	int i, max = 0, mode = 0;
	for (i=0; i < len; i++)
	{
		if (hist[i].count > max)
		{
			max = hist[i].count;
			mode = hist[i].val;
		}
	}
	return mode;
}

int
hist_min(struct histogram *hist, int len)
{
	int i, min = 99999;
	for (i=0; i < len; i++)
	{
		if (hist[i].count > 0 && hist[i].val < min)
		{
				min = hist[i].val;
		}
	}
	return min;
}

int
hist_max(struct histogram *hist, int len)
{
	int i, max= 0;
	for (i=0; i < len; i++)
	{
		if (hist[i].count > 0 && hist[i].val > max)
		{
				max = hist[i].val;
		}
	}
	return max;
}

double
hist_variance(struct histogram *hist, int len, int mean)
{
	int i, count = 0;
	double sum = 0;
	for (i=0; i < len; i++)
	{
		if (hist[i].count > 0) {
			sum += pow((double)(hist[i].val - mean), 2.0) * hist[i].count;
			count += hist[i].count;
		}
	}
	return sum / count;
}

double
hist_std(struct histogram *hist, int len, int mean)
{
	return sqrt(hist_variance (hist, len, mean));
}

// count number of misses
int
hist_q(struct histogram *hist, int len, int threshold)
{
	int i = 0, count = 0;
	for (i=0; i < len; i++)
	{
		if (hist[i].count > 0 && hist[i].val > threshold)
		{
			count += hist[i].count;
		}
	}
	return count;
}

void
hist_print(struct histogram *hist, int len)
{
	int i = 0;
	for (i=0; i < len; i++)
	{
		if (hist[i].count > 0)
		{
			printf("%d(%d) ", hist[i].val, hist[i].count);
		}
	}
	printf("\n");
}
