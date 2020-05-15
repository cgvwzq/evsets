# JS/Wasm implementation of group-testing reduction for finding minimal eviction ets

We implement the threshold group testing based reduction in JS and Wasm for efficiently finding eviction sets of minimal size. For more details check our paper.

Tested on *Chrome 74.0.3729.75 with V8 7.4* with `--allow-natives-syntax --experimental-wasm-bigint` flags. Natives syntax is only required for validating of resulting JS offsets. Wasm BigInt should be supported will be default soon.

Check my slides for more details about the Wasm implementation: https://vwzq.net/slides/2019-rootedcon_extended.pdf

## Build

For building Wasm binary files I used `wat2wasm` (v1.0.12) from the wabt toolkit:

```
$ wat2wasm --enable-threads *.wat
```

For the C verifier any compiler should be fine:

```
$ clang virt_to_phys.c -o virt_to_phys
```

## Setup

Launch web server in current directory:

```
$ python3 -m http.server --bind localhost
$ ./run.sh
```

You might need to modify `run.sh` with the right path to chrome.

## Run

The HTML page has a simple form with different parameters. Once the eviction set is found, `verify_addr.sh` will use `pmap` to identify the right PID for the chrome renderer process based on the size of the allocated eviction buffer, as well as the base virtual address of this buffer. Then it will parse the found JS offsets, add them to the index, and pass them to `virt_to_phys`, which requires sudo privileges to compute the physical address. From this, the program simply calculates the corresponding cache slice set-index.

## Demo video

Video from my talk at 2019 IEEE Symposium on Security & Privacy, skips to t=14m for live demo.

[![video](http://img.youtube.com/vi/6kYylH6fTHc/3.jpg)](https://www.youtube.com/watch?v=6kYylH6fTHc#t=14m)
