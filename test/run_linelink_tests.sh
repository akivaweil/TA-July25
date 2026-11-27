#!/bin/sh
# Runs the LineLink host-side unit tests (no hardware). The tests live with the library.
exec "$(cd "$(dirname "$0")" && pwd)/../lib/LineLink/test/run_tests.sh" "$@"
