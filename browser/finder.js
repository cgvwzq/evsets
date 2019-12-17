self.importScripts('/utils.js', '/evset.js');

// Send log to main thread
function log(...args) {
	self.postMessage({type: 'log', str: args});
}

// Constants
const P = 4096;
const VERBOSE = false;
const NOLOG = false;

const THRESHOLD = 60;
const RESULTS = [];

// global vars to refactor
var first, next, n;

self.onmessage = async function start(evt) {

	// Parse settings
	let {B, CONFLICT, OFFSET, ASSOC, STRIDE} = evt.data.conf;

	// Prepare wasm instance
    const {module, memory} = evt.data;
	const instance = new WebAssembly.Instance(module, {env: {mem: memory}});
	// Memory view
	const view = new DataView(memory.buffer);

	if (!NOLOG) log('Prepare new evset');
	const evset = new EvSet(view, B, P*2, P, ASSOC, STRIDE, OFFSET);
	first = true, next = CONFLICT;

	n = 0;
	const RETRY = 10;
	await new Promise(r => setTimeout(r, 10)); // timeout to allow counter
	do {
		let r = 0;
		while (!cb(instance, evset, CONFLICT) && ++r < RETRY && evset.victim) {
			if (VERBOSE) log('retry');
			first = false;
		}
		if (r < RETRY) {
			RESULTS.push(evset.refs); // save eviction set
			evset.refs = evset.del.slice();
			evset.del = [];
			evset.relink(); // from new refs
			next = CONFLICT;
			if (VERBOSE) log('Find next (', evset.refs.length, ')');
		}
		else
		{
			next = CONFLICT;
		}
	} while (CONFLICT && evset.vics.length > 0 && evset.refs.length > ASSOC);

	log('Found ' + RESULTS.length + ' different eviction sets');
	log('EOF');
	postMessage({type:'eof'});
}

function cb(instance, evset, findall) {

    let {wasm_hit, wasm_miss} = instance.exports;

    const REP = 6;
	const T = 1000;

	const CLOCK = 256; // hardcoded offset in wasm
	const VICTIM = evset.victim|0;
	const PTR = evset.ptr|0;

	function runCalibration(title, hit, miss, warm) {
		for (let i=0; i<T; i++) {
			hit(VICTIM);
			miss(VICTIM, 0);
		}
		if (!warm) {
			// real run
			let t_hit = hit(VICTIM);
			let t_miss = miss(VICTIM, PTR);
			// output
			if (VERBOSE) log ('--- ' + title + ' ---');
			if (VERBOSE) log ('Hit:\t' + (Array.isArray(t_hit) ? stats(t_hit) : t_hit));
			if (VERBOSE) log ('Miss:\t' + (Array.isArray(t_miss) ? stats(t_miss) : t_miss));
			if (VERBOSE) log ('-----------');
			// calc threshold
			if (Array.isArray(t_hit)) {
				t_hit = stats(t_hit).median;
			}
			if (Array.isArray(t_miss)) {
				t_miss = stats(t_miss).median;
			}
			if (t_hit > t_miss) {
				return 0;
			} else {
				return ((Number(t_miss) + Number(t_hit) * 2) / 3);
			}
		}
	}

	const wasmMeasureOpt = {
		hit : function hit(vic) {
			let t, total = [];
			for (let i=0; i<REP; i++) {
				t = wasm_hit(vic);
				total.push(Number(t));
			}
			return total;
		},
		miss : function miss(vic, ptr) {
			let t, total = [];
			for (let i=0; i<REP; i++) {
				t = wasm_miss(vic, ptr);
				total.push(Number(t));
			}
			return total;
		}
	}

	if (first) {
		runCalibration('Wasm measure opt', wasmMeasureOpt.hit, wasmMeasureOpt.miss, true);
		if (!THRESHOLD) {
			log('Error: calibrating');
			return false;
		}
		log('Calibrated threshold: ' + THRESHOLD);

		if (findall) {
			log('Creating conflict set...');
			evset.genConflictSet(wasmMeasureOpt.miss, THRESHOLD);
			log('Done: ' + evset.refs.length);
			first = false;
		}
	}

	if (next) {
		let t;
		do {
			evset.victim = evset.vics.pop();
			if (VERBOSE) log('\ttry victim', evset.victim);
			let e = 0;
			while (evset.victim && e < RESULTS.length) {
				if (median(wasmMeasureOpt.miss(evset.victim, RESULTS[e][0])) >= THRESHOLD) {
					if (VERBOSE) log('\tanother, this belongs to a previous eviction set');
					evset.victim = evset.vics.pop();
				}
				e += 1;
			}
			t = median(wasmMeasureOpt.miss(evset.victim, evset.ptr));
		} while (evset.victim && t < THRESHOLD);
		if (!evset.victim) {
			if (VERBOSE) log('No more victims');
			return false;
		}
		next = false;
	}

	if (VERBOSE) log ('Starting reduction...');
	evset.groupReduction(wasmMeasureOpt.miss, THRESHOLD);

	if (evset.refs.length === evset.assoc) {
		if (!NOLOG) log('Victim addr: ' + evset.victim);
		if (!NOLOG) log('Eviction set: ' + evset.refs);
		evset.del = evset.del.flat();
		return true;
	} else {
		while (evset.del.length > 0) {
			evset.relinkChunk();
		}
		if (VERBOSE) log('Failed: ' + evset.refs.length);
		return false;
	}
}
