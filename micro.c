#include "micro.h"
#include "cache.h"

#include <fcntl.h>
#include <unistd.h>

ul
vtop(ul vaddr)
{
	int fd = open ("/proc/self/pagemap", O_RDONLY);
	if (fd < 0)
	{
		return -1;
	}

	unsigned long paddr = -1;
	unsigned long index = (vaddr / PAGE_SIZE2) * sizeof(paddr);
	if (pread (fd, &paddr, sizeof(paddr), index) != sizeof(paddr))
	{
		return -1;
	}
	close (fd);
	paddr &= 0x7fffffffffffff;
	return (paddr << PAGE_BITS) | (vaddr & (PAGE_SIZE2-1));
}

unsigned int
count_bits(ul n)
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
nbits(ul n)
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

ul
ptos(ul paddr, ul slices)
{
	unsigned long long ret = 0;
	unsigned long long mask[3] = {0x1b5f575440ULL, 0x2eb5faa880ULL, 0x3cccc93100ULL}; // according to Maurice et al.
	int bits = nbits(slices) - 1;
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
recheck(Elem *ptr, char *victim, bool err, struct config *conf)
{
	unsigned int cache_sets = conf->cache_size / LINE_SIZE / conf->cache_way / conf->cache_slices;
	ul vpaddr = 0, paddr = 0, vcacheset = 0, cacheset = 0, vslice = 0, slice = 0;
	ul num = 0;
	bool verified = true;
	if (victim)
	{
		vpaddr = vtop ((ul)victim);
		vcacheset = (vpaddr >> LINE_BITS) & (cache_sets - 1);
		vslice = ptos (vpaddr, conf->cache_slices);
	}
	else
	{
		vpaddr = vtop ((ul)ptr);
		vcacheset = (vpaddr >> LINE_BITS) & (cache_sets - 1);
		vslice = ptos (vpaddr, conf->cache_slices);
	}
	if (vpaddr == 0xffffffffffffffff)
	{
		printf("[!] Page map not supported. Can't verify set.\n");
		return;
	}
	else
	{
		if (!err)
		{
			printf("[+] Verify eviction set (only in Linux with root):\n");
			if (victim)
			{
				printf(" - victim pfn: 0x%llx, cache set: 0x%llx, slice: ",
						vpaddr, vcacheset);
                if (!(conf->flags & FLAG_IGNORESLICE))
                {
                    printf("0x%llx\n", vslice);
                }
                else
                {
                    printf("???\n");
                }
			}
		}
	}
	while (ptr)
	{
		paddr = vtop ((ul)ptr);
		cacheset = (paddr >> LINE_BITS) & (cache_sets - 1);
		slice = ptos (paddr, conf->cache_slices);
		if (!err)
		{
			printf(" - element pfn: 0x%llx, cache set: 0x%llx, slice: ",
					paddr, cacheset);
			if (!(conf->flags & FLAG_IGNORESLICE))
			{
				printf("0x%llx\n", slice);
			}
			else
			{
				printf("???\n");
			}
		}
		ptr = ptr->next;
		if (vcacheset == cacheset && ((vslice == slice) || (conf->flags & FLAG_IGNORESLICE)))
		{
			num++;
		}
		else
		{
			verified = false;
		}
	}
	if (verified && !err)
	{
		printf("[+] Verified!\n");
	}
	else
	{
		printf("[-] Num. congruent addresses: %llu\n", num);
	}
}

int
filter(Elem **ptr, char *victim, int n, int m, struct config *conf)
{
	unsigned int cache_sets = conf->cache_size / LINE_SIZE / conf->cache_way / conf->cache_slices;
	Elem *tmp = *ptr, *prev = NULL;
	ul vpaddr = vtop ((ul)victim), vslice = ptos (vpaddr, conf->cache_slices);
	ul vcacheset = (vpaddr >> LINE_BITS) & (cache_sets - 1);
	ul cacheset = 0, paddr = 0, slice = 0;
	if (vpaddr == 0xffffffffffffffff)
	{
		printf("[!] Page map not supported. Can't verify set.\n");
		return 1;
	}
	while (tmp)
	{
		paddr = vtop ((ul)tmp);
		slice = ptos (paddr, conf->cache_slices);
		cacheset = (paddr >> LINE_BITS) & (cache_sets - 1);
		if (vcacheset == cacheset && ((vslice == slice) || (conf->flags & FLAG_IGNORESLICE)))
		{
			if (n > 0)
			{
				if (prev == NULL)
				{
					*ptr = prev = tmp;
					tmp->prev = NULL;
				}
				else
				{
					prev->next = tmp;
					tmp->prev = prev;
					prev = tmp;
				}
				n--;
			}
		}
		else if (m > 0)
		{
			if (prev == NULL)
			{
				*ptr = prev = tmp;
				tmp->prev = NULL;
			}
			else
			{
				prev->next = tmp;
				tmp->prev = prev;
				prev = tmp;
			}
			m--;
		}
		tmp = tmp->next;
	}
	if (prev)
	{
		prev->next = NULL;
	}
	else
	{
		*ptr = NULL;
		return 1;
	}
	return 0;
}
