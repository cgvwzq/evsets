#!/bin/env /bin/bash

google-chrome-beta --user-data-dir=/tmp/tmp.u9lo18kaTh --js-flags='--allow-natives-syntax --experimental-wasm-bigint' http://localhost:8000/ | ./verify_addr.sh
