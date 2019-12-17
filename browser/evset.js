function EvSet(view, nblocks, start=8192, victim=4096, assoc=16, stride=4096, offset=0) {

	const RAND = true;

	/* private methods */
	this.genIndices = function (view, stride) {
		let arr = [], j = 0;
		for (let i=(stride)/4; i < (view.byteLength-this.start)/4; i += stride/4) {
			arr[j++] = this.start + this.offset + i*4;
		}
		arr.unshift(this.start + this.offset);
		return arr;
	}

	this.randomize = function (arr) {
		for (let i = arr.length; i; i--) {
			var j = Math.floor(Math.random() * i | 0) | 0;
			[arr[i - 1], arr[j]] = [arr[j], arr[i - 1]];
		}
		return arr;
	}

	this.indicesToLinkedList =  function (buf, indices) {
		if (indices.length == 0) {
			this.ptr = 0;
			return;
		}
		let pre = this.ptr = indices[0];
		for (let i=1; i<indices.length; i++) {
			view.setUint32(pre, indices[i], true);
			pre = indices[i];
		}
		view.setUint32(pre, 0, true);
	}

	this.init = function() {
		let indx = this.genIndices(view, stride);
		if (RAND) indx = this.randomize(indx);
		indx.splice(nblocks, indx.length); // select nblocks elements
		this.indicesToLinkedList(view, indx);
		return indx;
	}
	/* end-of-private */

	/* properties */
	this.start = start;
	this.offset = (offset&0x3f)<<6;
	this.victim = victim+this.offset;
	view.setUint32(this.victim, 0, true); // lazy alloc
	this.assoc = assoc;
	this.ptr = 0;
	this.refs = this.init();
	this.del = [];
	this.vics = [];
	/* end-of-properties */

	/* public methods */
	this.unlinkChunk = function unlinkChunk(chunk) {
		let s = this.refs.indexOf(chunk[0]), f = this.refs.indexOf(chunk[chunk.length-1]);
		view.setUint32(this.refs[f], 0, true);
		this.refs.splice(s, chunk.length); // splice chunk indexes
		if (this.refs.length === 0) { // empty list
			this.ptr = 0;
		} else if (s === 0) { // removing first chunk
			this.ptr = this.refs[0];
		} else if (s > this.refs.length-1) { // removing last chunk
			view.setUint32(this.refs[this.refs.length-1], 0, true);
		} else { // removing middle chunk
			view.setUint32(this.refs[s-1], this.refs[s], true);
		}
		this.del.push(chunk); // right
	}

	this.relinkChunk = function relinkChunk() {
		let chunk = this.del.pop(); // right
		if (chunk === undefined) {
			return;
		}
		this.ptr = chunk[0];
		if (this.refs.length > 0) {
			view.setUint32(chunk[chunk.length-1], this.refs[0], true);
		}
		if (typeof(chunk) === 'number') {
			this.refs.unshift(chunk); // left
		} else {
			this.refs.unshift(...chunk); // left
		}
	}

	this.groupReduction = function groupReduction(miss, threshold) {
		const MAX = 20;
		let i = 0, r = 0;
		while (this.refs.length > this.assoc) {
			let m = this.refs.chunk(this.assoc+1);
			let found = false;
			for (let c in m) {
				this.unlinkChunk(m[c]);
				let t = median(miss(this.victim, this.ptr));
				if (t < threshold) {
					this.relinkChunk();
				} else {
					found = true;
					break;
				}
			}
			if (!found) {
				r += 1;
				if (r < MAX) {
					this.relinkChunk();
					if (this.del.length === 0) break;
				} else {
					while (this.del.length > 0) {
						this.relinkChunk();
					}
					break;
				}
			}
			if (VERBOSE) if (!(i++ % 100)) print('\tremaining size: ', this.refs.length);
		}
	}

	this.linkElement = function linkElement(e) {
		if (e === undefined) return;
		this.ptr = e;
		if (this.refs.length > 0) {
			view.setUint32(e, this.refs[0], true);
		} else {
			view.setUint32(e, 0, true);
		}
		this.refs.unshift(e); // left
	}

	this.relink = function () {
		this.indicesToLinkedList(this.buffer, this.refs);
	}

	this.genConflictSet = function (miss, threshold) {
		let indices = this.refs; // copy original indices
		this.refs = [];
		this.vics = [];
		let pre = this.ptr = indices[0], i = 0, e, l = indices.length;
		for (i=0; i<Math.min(l, 800); i++) {
			e =  indices.pop();
			this.linkElement(e);
		}
		while (indices.length > 0) {
			e = indices.pop();
			view.setUint32(e, 0, true); // chrome's COW
			let t = miss(e, this.ptr);
			if (Array.isArray(t)) {
				t = median(t);
			}
			if (t < threshold) {
				this.linkElement(e);
			} else {
				this.vics.push(e);
				// break;
			}
		}
		first = true;
	}
	/* end-of-public */
}
