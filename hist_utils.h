#ifndef hist_utils_H
#define hist_utils_H

#include <stdlib.h>

struct histogram
{
	int val;
	int count;
};

float hist_avg(struct histogram *hist, int len);
int hist_mode(struct histogram *hist, int len);
int hist_min(struct histogram *hist, int len);
int hist_max(struct histogram *hist, int len);
int hist_q(struct histogram *hist, int len, int threshold);
double hist_variance(struct histogram *hist, int len, int mean);
double hist_std(struct histogram *hist, int len, int mean);
void hist_print(struct histogram *hist, int len);
void hist_add(struct histogram *hist, int len, size_t val);

#endif /* hist_utils_H */
