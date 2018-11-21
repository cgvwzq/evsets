#include "utils.h"

void
shuffle(int *array, size_t n)
{
	size_t i;
	if (n > 1)
	{
		for (i = 0; i < n - 1; i++)
		{
			size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}
