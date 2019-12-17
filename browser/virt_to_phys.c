/* from https://github.com/cgvwzq/evsets/blob/master/micro.c */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_BITS 12
#define LINE_BITS 6
#define SLICE_BITS 3
#define SET_BITS 10

#define PAGE_SIZE2 (1 << PAGE_BITS)
#define LINE_SIZE (1 << LINE_BITS)
#define CACHE_SLICES (1 << SLICE_BITS)
#define CACHE_SETS (1 << SET_BITS)

unsigned long long
vtop(unsigned pid, unsigned long long vaddr)
{
	char path[1024];
	sprintf (path, "/proc/%u/pagemap", pid);
	int fd = open (path, O_RDONLY);
	if (fd < 0)
	{
		return -1;
	}

	unsigned long long paddr = -1;
	unsigned long long index = (vaddr / PAGE_SIZE2) * sizeof(paddr);
	if (pread (fd, &paddr, sizeof(paddr), index) != sizeof(paddr))
	{
		return -1;
	}
	close (fd);
	paddr &= 0x7fffffffffffff;
	return (paddr << PAGE_BITS) | (vaddr & (PAGE_SIZE2-1));
}

unsigned int
count_bits(unsigned long long n)
{
	unsigned int count = 0;
	while (n)
	{

		n &= (n-1) ;
		count++;
	}
	return count;
}

unsigned int
nbits(unsigned long long n)
{
	unsigned int ret = 0;
	n = n >> 1;
	while (n > 0)
	{
		n >>= 1;
		ret++;
	}
	return ret;
}

unsigned long  long
ptos(unsigned long long paddr, unsigned long long bits)
{
	unsigned long long ret = 0;
	unsigned long long mask[3] = {0x1b5f575440ULL, 0x2eb5faa880ULL, 0x3cccc93100ULL}; // according to Maurice et al.
	switch (bits)
	{
		case 3:
			ret = (ret << 1) | (unsigned long long)(count_bits(mask[2] & paddr) % 2);
		case 2:
			ret = (ret << 1) | (unsigned long long)(count_bits(mask[1] & paddr) % 2);
		case 1:
			ret = (ret << 1) | (unsigned long long)(count_bits(mask[0] & paddr) % 2);
		default:
		break;
	}
	return ret;
}

void
check(unsigned int pid, unsigned long long *virtual_addresses, unsigned int length)
{
	unsigned int cache_sets = 1024;

	unsigned long long paddr = 0, cacheset = 0, slice = 0;

	for (unsigned int i = 0; i < length; i++)
	{
		paddr = vtop (pid, virtual_addresses[i]);
		cacheset = (paddr >> LINE_BITS) & (CACHE_SETS - 1);
		slice = ptos (paddr, SLICE_BITS);
		printf(" - element pfn: 0x%llx, cache set: 0x%llx, slice: 0x%llx\n", paddr, cacheset, slice);
	}
}

int
main(int argc, char **argv)
{
	unsigned int i = 0;
	if (argc < 3)
	{
		printf ("[!] Use: %s pid 0x1 0x2 0x3 ...\n", argv[0]);
		return 1;
	}
	unsigned int pid = atoi (argv[1]);
	unsigned int len = argc - 2;
	unsigned long long *addrs = malloc (sizeof(unsigned long long)*len);
	char *eos = argv[argc-1] + strlen(argv[argc-1]);
	if (!addrs)
	{
		printf ("[!] Err: allocate\n");
		return 1;
	}
	for (i = 2; i < argc; i++)
	{
		addrs[i-2] = strtoull (argv[i], &eos, 16);
	}

	check (pid, addrs, len);


}
