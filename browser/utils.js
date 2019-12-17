// Statistics
function stats(data) {
    return {
        'min' : Math.min.apply(0, data),
        'max' : Math.max.apply(0, data),
        'mean' : mean(data),
        'median' : median(data),
        'std': std(data),
        'mode' : mode(data),
        'toString' : function() {
            return `{min: ${this.min.toFixed(2)},\tmax: ${this.max.toFixed(2)},\tmean: ${this.mean.toFixed(2)},\tmedian: ${this.median.toFixed(2)},\tstd: ${this.std.toFixed(2)},\tmode: ${this.mode.map(e => e.toFixed(2))}}`;
        }
    };
}

function min(arr) {
	return Math.min.apply(0, arr);
}

function mean(arr) {
        return arr.reduce((a,b) => a+b) / arr.length;
}

function median(arr) {
        arr.sort((a,b) => a-b);
        return (arr.length % 2) ? arr[(arr.length / 2) | 0] : mean([arr[arr.length/2 - 1], arr[arr.length / 2]]);
}

function mode(arr) {
        var counter = {};
        var mode = [];
        var max = 0;
        for (var i in arr) {
                if (!(arr[i] in counter)) {
                        counter[arr[i]] = 0;
                }
                counter[arr[i]]++;
                if (counter[arr[i]] == max) {
                        mode.push(arr[i]);
                } else if (counter[arr[i]] > max) {
                        max = counter[arr[i]];
                        mode = [arr[i]];
                }
        }
        return mode;
}

function variance(arr) {
    var x = mean(arr);
    return arr.reduce((pre, cur) => pre + ((cur - x)**2)) / (arr.length - 1);
}

function std(arr) {
    return Math.sqrt(variance(arr));
}

// Overload
Function.prototype.toSource = function() {
    return this.toString().slice(this.toString().indexOf('{')+1,-1);
}

Object.defineProperty(Array.prototype, 'chunk', {
    value: function(n){
		let results = [];
		let ceiled = this.length%n;
		let k = Math.ceil(this.length/n);
		let q = Math.floor(this.length/n);
		let c = 0;
		for (i=0; i<ceiled; i++) {
			results[i] = this.slice(c, c+k);
			c += k;
		}
		for (i; i<n; i++) {
			results[i] = this.slice(c, c+q);
			c += q;
		}
		return results;
    }
});

// OptimizationStatus
function optimizationStatusToString(status) {
/* from https://github.com/v8/v8/blob/master/src/runtime/runtime.h */
	let o = [];
	if (status & (1<<0)) o.push('kIsFunction');
	if (status & (1<<1)) o.push('kNeverOptimize');
	if (status & (1<<2)) o.push('kAlwaysOptimize');
	if (status & (1<<3)) o.push('kMaybeDeopted');
	if (status & (1<<4)) o.push('kOptimized');
	if (status & (1<<5)) o.push('kTurboFanned');
	if (status & (1<<6)) o.push('kInterpreted');
	if (status & (1<<7)) o.push('kMarkedForOptimization');
	if (status & (1<<8)) o.push('kMarkedForConcurrentOptimization');
	if (status & (1<<9)) o.push('kOptimizingConccurently');
	if (status & (1<<10)) o.push('kIsExecuting');
	if (status & (1<<11)) o.push('kTopmostFrameIsTurboFanned');
	if (status & (1<<12)) o.push('kLiteMode');
	return o.join("|");
}

// Lists
