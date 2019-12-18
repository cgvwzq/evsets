# Finding eviction sets

This tool allows to find eviction sets using different algorithms and settings.

This is part of the work done for "Theory and Practice of Finding Eviction Sets" paper: https://www.computer.org/csdl/proceedings-article/sp/2019/666000a695/19skfTIwuZy (or https://vwzq.net/papers/evictionsets18.pdf)

`Warning: this is a proof-of-concept, only useful for testing your system and interacting with the cache. Use under your own risk.`

## Installation

Only a `make` is required to build the static binary.

## Example

```
$ sudo ./evsets -b 3000 -c 6 -s 8 -a g -e 2 -n 12 -o 4096 -r 10 -t 95 -C 0 --verify --retry --backtracking --nohugepages
[+] 11 MB buffer allocated at 0x7fb6dd79a000 (192000 blocks)
[+] Default Threshold = 95
[+] Initial candidate set evicted victim
[+] Created linked list structure (3000 elements)
[+] Starting group reduction...
[+] Reduction time: 0.057381 seconds
[+] Total execution time: 0.086908 seconds
[+] (ID=0) Found minimal eviction set for 0x7fb6d579a000 (length=12): 0x7fb6ddc25000 0x7fb6dfe85000 0x7fb6e08d5000 0x7fb6e4475000 0x7fb6dde75000 0x7fb6e47e5000 0x7fb6ddb69000 0x7fb6e0715000 0x7fb6e4d85000 0x7fb6dfb55000 0x7fb6de63d000 0x7fb6df995000
[+] Verify eviction set (only in Linux with root):
 - victim pfn: 0x2e1a95000, cache set: 0x140, slice: 0x2
 - element pfn: 0x448915000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e18f5000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e34f5000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e2795000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e19d5000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e3805000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e4365000, cache set: 0x140, slice: 0x2
 - element pfn: 0x310c35000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e6ba5000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2f91d5000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e6325000, cache set: 0x140, slice: 0x2
 - element pfn: 0x2e4c65000, cache set: 0x140, slice: 0x2
[+] Verified!
```

## Description parameters

Help:

```
[?] Usage: ./evsets [flags] [params]

	Flags:
		--nohugepages
		--retry	(complete repetitions)
		--backtracking
		--verbose
		--verify (requires root)
		--ignoreslice (unknown slicing function)
		--findallcolors
		--findallcongruent
		--conflictset
	Params:
		-b N		number of lines in initial buffer (default: 3072)
		-t N		threshold in cycles (default: calibrates)
		-c N		cache size in MB (default: 6)
		-s N		number of cache slices (default: 4)
		-n N		cache associativity (default: 12)
		-o N		stride for blocks in bytes (default: 4096)
		-a n|o|g|l	search algorithm (default: 'g')
		-e 0|1|2|3|4	eviction strategy: 0-haswell, 1-skylake, 2-simple (default: 2)
		-C N		page offset (default: 0)
		-r N		numer of rounds per test (default: 200)
		-q N		ratio of success for passing a test (default: disabled)
		-h		show this help

	Example:
		./evsets -b 3072 -c 6 -s 8 -a g -n 12 -o 4096 -a g -e 2 -C 0 -t 85 --verbose --retry --backtracking
```
### `--nohugepages`

By default it tries to use huge pages.

Note that to enable them it might be necessary to run `sysctl -w vm.nr_hugepages=2048`. This flag forces to use 4KB pages.

### `--retry`

Repeat the whole proces from scratch if the reduction was not successful.

### `--backtracking`

Performs backtracking during the reductions in order to recover from an error.

### `--verbose`

Print some detailed info of the reduction status.

### `--verify`

Only works on Linux and requires root privileges. Will translate the virtual addresses of the eviction sets in order to verify that set index and slice match.

### `--ignoreslice`

By default we rely on Intel reverse engineered slice functions, which have proved to work well on Haswell, Broadwell, Skylake, and Kaby Lake R systems.

If the system use a different slicing, this flag disables the check.

### `--findallcolors`

After finding a minimal eviction set, will remove it from the initial buffer and iterate to find other eviction sets.

### `--findallcongruent`

After finding a minimal eviction set, will remove all other congruent addresses from the initial buffer.

### `--conflictset`

Before starting the reduction will compute a conflict set (i.e. the union of all minimal eviction sets in the set). This acelerates reduction when finding many eviction sets (or any).

### `-b`: size initial buffer

This parameter defines the number of randomly selected lines (from a 128MB buffer pool) that will form the initial eviction set. The choice of this parameter should be done based on the probability models for finding an eviction set for a given address or for finding any eviction set. Both depend on the associativity and probability of collision `P(C)`. The probability of collision is calculated based on the number of cache sets, slices, and information about the physical address (usually the page size).

Some common values are `-b 800` for finding any eviction set (i.e., `-a l`), and `-b 3000` for finding an eviction set for a given address (i.e., `-a g|b|n|o`).

### `-t`: threshold

This parameter is optional. If not provided a calibration phase will define it. This value is system dependant, but after one calibration it can be fixed to safe time in future executions.

### `-c`: cache size

Total size of cache in MB.

### `-s`: cache slices

Number of cache slices (usually depends on the number of cores).

### `-n`: cache associativity

Cache associativity. This parameter depends on the hardware (e.g. 8, 12, 16).

### `-o`: stride

This parameter servers for simulating the knowlegde of the physical address that the attacker has. By default it only knows the page offset, hence, the only addresses that need to be accessed for finding an eviction set are 4096 bytes apart. Shorter distances would ensure that the 6 less significant bits of the index set are different.

### `-a`: search algorithm

* `n` naive algorithm: O(n^2). SPECIFIC
* `o` naive algorithm optimistic: O(n^2). SPECIFIC
* `g` threshold group testing: O(u^2*n). SPECIFIC
* `b` threshold binary group testing: ?. SPECIFIC (inefficient implementation)
* `l` linear: ?. ANY (inneficient implementation due to larger number of time measurements)

### `-e`: eviction strategy

* `0` optimal for Haswell
* `1` optimal for Skylake
* `2` simple traversing element by element (default)
* `3` optimal for Skylake in x86
* `4` optimal for Haswell in x86

(according to rowhammer.js paper)

### `-C`: page offset

Selects the offset used for the victim and all addresses in the buffer. Only useful for SPECIFIC algorithm.

### `-r`: repetitions per test

This parameter defines the number of time measurements used for averaging before comparing with the threshold. It should help to remove noise due to several aspects: imperfect eviction strategy, false positives, multi-process systems, etc.

Depending on the overhead of the system, `20` should be enough.

### `-q`: ratio of success per test

If defined, a test is positive only if there are at least `repetition`*`ratio` misses. Instead of using the average of `repetition` tests.

## Debug

A hidden `--debug` flag has been added to allow quick tests with fixed number of congruent (`-x N`) and non-congruent (`-y M`) addresses.

For instance: `sudo ./evsets -b 3000 -c 6 -s 8 -a g -e 1 -n 12 -o 4096 -r 20 -C 1 -t 100 --verify --debug -x 12 -y 0`, performs tests with only 12 congruent addresses.

Requires root.

## Tested systems

This tools has been successfully tested on:

* Intel(R) Core(TM) i5-6500 CPU (2 x 2) @ 3.20GHz (Skylake)
	* RAM: 16GB
	* L3: 6MB
	* Associativity: 12
	* Cache sets: 8192
	* Slices: 8
* Intel(R) Core(TM) i7-4790 CPU (4 x 2) @ 3.60GHz (Haswell)
	* RAM: 8GB
	* L3: 8MB
	* Associativity: 16
	* Cache sets: 8192
	* Slices: 4
* Intel(R) Core(TM) i7-8550U CPU (4 x 2) @ 1.80GHz (Kaby Lake R)
	* RAM: 16GB
	* L3: 8MB
	* Associativity: 16
	* Cache sets: 8192
	* Slices: 8
