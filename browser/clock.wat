(module
    (import "env" "mem" (memory 2048 2048 shared))
    (func $main
        (loop $ccc
              ;;(i64.atomic.rmw.add (i32.const 256) (i64.const 1))
              (i64.store (i32.const 256) (i64.add (i64.load (i32.const 256)) (i64.const 1)))
              (br $ccc))
    )
    (start $main)
)
