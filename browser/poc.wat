(module
    (import "env" "mem" (memory 2048 2048 shared))
    (export "wasm_hit" (func $hit))
    (export "wasm_miss" (func $miss))

    (func $hit (param $victim i32) (result i64)
        (local $t0 i64)
        (local $t1 i64)
        (local $td i64)
        ;; acces victim
        (set_local $td (i64.load (i32.and (i32.const 0xffffffff) (get_local $victim))))
        ;; t0 (mem[0])
        (set_local $t0 (i64.load (i32.and (i32.const 0xffffffff) (i32.or (i32.const 256) (i32.eqz (i64.eqz (get_local $td)))))))
        ;; re-access
        (set_local $td (i64.load (i32.and (i32.const 0xffffffff) (i32.or (get_local $victim) (i64.eqz (get_local $t0))))))
        ;; t1 (mem[0])
        (set_local $t1 (i64.load (i32.and (i32.const 0xffffffff) (i32.or (i32.const 256) (i32.eqz (i64.eqz (get_local $td)))))))
        (i64.sub (get_local $t1) (get_local $t0))
        return)

    (func $miss (param $victim i32) (param $ptr i32) (result i64)
        (local $t0 i64)
        (local $t1 i64)
        (local $td i64)
        ;; acces victim
        (set_local $td (i64.load (i32.and (i32.const 0xffffffff) (get_local $victim))))
		;; traverse
        (set_local $td (i64.extend_u/i32 (i32.or (i32.eqz (i64.eqz (get_local $td))) (get_local $ptr))))
        (loop $iter
            (set_local $td (i64.load (i32.wrap/i64 (get_local $td))))
            (br_if $iter (i32.eqz (i64.eqz (get_local $td)))))
        ;; t0 (mem[0])
        (set_local $t0 (i64.load (i32.and (i32.const 0xffffffff) (i32.or (i32.const 256) (i32.eqz (i64.eqz (get_local $td)))))))
        ;; re-access
        (set_local $td (i64.load (i32.and (i32.const 0xffffffff) (i32.or (get_local $victim) (i64.eqz (get_local $t0))))))
        ;; t1 (mem[0])
        (set_local $t1 (i64.load (i32.and (i32.const 0xffffffff) (i32.or (i32.const 256) (i32.eqz (i64.eqz (get_local $td)))))))
        (i64.sub (get_local $t1) (get_local $t0))
        return)
)
