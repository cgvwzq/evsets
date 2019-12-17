self.onmessage = function(evt) {
    const {module, memory, cb} = evt.data;
    const instance = new WebAssembly.Instance(module, {env: {mem: memory}});
    if (cb) {
        let fn = new Function('instance', 'mem', cb);
        fn(instance, memory);
    }
}
