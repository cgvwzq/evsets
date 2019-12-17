#!/usr/bin/env bash

# Run:
# $ google-chrome-beta --user-data-dir=/tmp/tmp.u9lo18kaTh --js-flags='--allow-natives-syntax --experimental-wasm-bigint' http://localhost:8000/ | ./verify_addr.sh
#
# Dependencies:
# --allow-natives-stynax only used to verify offsets via command line
# --experimental-wasm-bigint will be soon by default
# pmap used to find offset of shared buffer (128*1024 KB)
# gcc virt_to_phys.c -o virt_to_phys (used to translate virtual to physical, get slice and index set)

declare -A allocated

while true; do

	while read line; do
		if [[ $line =~ "Prepare new evset" ]]; then
			break;
		fi
	done

	# find right pid checking large allocated buffer
	base=""
	pids=$(ps aux | grep 'chrome-beta/chrome --type=renderer' | awk '{print $2}')
	for p in $pids; do
		bases=$(pmap $p | grep '131072K' | awk '{print $1}')
		if [ ! -z "$bases" ]; then
			pid=$p
			break;
		fi
	done
	# find right allocated buffer if no gc yet
	for c in $bases; do
		if [ -z "${allocated[c]}" ]; then
			allocated+=(["$c"]=1) # add element to avoid repeat
			base="0x$c"
			break;
		fi
	done
	if [ -z "$base" ]; then
		echo "[!] Error"
		exit 1
	fi
	echo "PID: $pid"
	echo "Virtual base: $base"

	conflict=0

	while read line; do
		if [[ $line =~ "Creating conflict set..." ]]; then
			conflict=1
		elif [[ $line =~ "Victim addr:" ]]; then
			vic="$(echo $line | cut -d: -f 3 | cut -d\> -f 1)"
			vic="$(printf '%x ' $(($base+$vic)))"
		elif [[ $line =~ "Eviction set:" ]]; then
			addresses="$(echo $line | cut -d: -f 3 | cut -d\> -f 1 | tr ',' ' ')"
			vaddrs=$(for o in $addresses; do printf '%x ' $(($base+$o)); done)
			echo "Physical addresses:"
			# needs sudo to work
			sudo ./virt_to_phys $pid $vic $vaddrs
			echo "============================================"
		elif [[ $line =~ "EOF" ]]; then
			break;
		fi
	done

done
