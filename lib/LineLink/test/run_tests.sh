#!/bin/sh
# Host-side LineLink unit tests: no hardware, no PlatformIO. Needs clang++ or g++.
# Usage: lib/LineLink/test/run_tests.sh   (from anywhere; paths may contain spaces)
HERE="$(cd "$(dirname "$0")" && pwd)"
CORE="$HERE/../src/LineLinkCore.cpp"
OUT="${TMPDIR:-/tmp}/linelink_tests_$$"
CXX="${CXX:-clang++}"
command -v "$CXX" >/dev/null 2>&1 || CXX=g++
SDK="$(xcrun --show-sdk-path 2>/dev/null || true)"

# build <sanitizer> <output> <test source>
# Some macOS Command Line Tools installs ship an incomplete libc++ under /usr/include;
# fall back to the copy inside the SDK.
build() {
    "$CXX" -std=c++11 -Wall -Wextra -g -O1 -fsanitize="$1" -I"$HERE/../src" "$3" "$CORE" -o "$2" 2>/dev/null && return 0
    if [ -d "$SDK/usr/include/c++/v1" ]; then
        "$CXX" -std=c++11 -Wall -Wextra -g -O1 -fsanitize="$1" -nostdinc++ -isystem "$SDK/usr/include/c++/v1" \
            -I"$HERE/../src" "$3" "$CORE" -o "$2"
    else
        "$CXX" -std=c++11 -Wall -Wextra -g -O1 -fsanitize="$1" -I"$HERE/../src" "$3" "$CORE" -o "$2"   # show the real error
    fi
}

STATUS=0

# Protocol logic under a mock radio (loss, duplication, reordering, wraparound).
if build address,undefined "$OUT" "$HERE/test_linelink.cpp"; then
    "$OUT" || STATUS=1
else
    STATUS=1
fi
rm -f "$OUT"

# Concurrency stress (ThreadSanitizer): receive callback / service() / app API on separate threads.
if build thread "$OUT.tsan" "$HERE/test_linelink_threads.cpp"; then
    "$OUT.tsan" || STATUS=1
else
    STATUS=1
fi
rm -f "$OUT.tsan"

exit $STATUS
