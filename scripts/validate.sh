#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${ITRS_BUILD_DIR:-$root/build}"
cmake -S "$root" -B "$build" -DITRS_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$build" --parallel
ctest --test-dir "$build" --output-on-failure

baseline="$($build/itrs-number-demo)"
local_down="$($build/itrs-number-demo --local-down)"
both_down="$($build/itrs-number-demo --local-down --regional-down)"
grep -q 'selected=asl-local ' <<<"$baseline"
grep -q 'selected=asl-regional ' <<<"$local_down"
grep -q 'selected=vrs-bridge ' <<<"$both_down"
psap='authoritative_psap=psap-frederick <sip:psap-frederick@example.invalid>'
[[ "$(grep -F "$psap" <<<"$baseline")" == "$psap" ]]
[[ "$(grep -F "$psap" <<<"$local_down")" == "$psap" ]]
[[ "$(grep -F "$psap" <<<"$both_down")" == "$psap" ]]
echo "PASS: deterministic resolver, failover chain, and PSAP invariant"
enum_one="$($build/itrs-enum-demo)"
enum_two="$($build/itrs-enum-demo)"
grep -q '^query=2\.1\.2\.1\.5\.5\.5\.1\.0\.8\.1\.itrs\.us\.$' <<<"$enum_one"
grep -q '^uri=sip:+18015551212@providerGW\.example\.com$' <<<"$enum_one"
[[ "$enum_one" == "$enum_two" ]]
echo "PASS: deterministic iTRS ENUM/NAPTR rewrite and replay identity"

if [[ -n "${ITRS_CELIX_SOURCE_DIR:-}" ]]; then
    celix_build="${ITRS_CELIX_BUILD_DIR:-${build}-celix}"
    cmake -S "$root" -B "$celix_build" \
        -DITRS_BUILD_TESTS=ON \
        -DITRS_ENABLE_CELIX=ON \
        -DITRS_CELIX_SOURCE_DIR="$ITRS_CELIX_SOURCE_DIR" \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build "$celix_build" --parallel
    ctest --test-dir "$celix_build" --output-on-failure

    deploy="$celix_build/deploy/itrs-ng-celix-demo"
    set +e
    celix_output="$(cd "$deploy" && timeout 4 ./itrs-ng-celix-demo 2>&1)"
    celix_rc=$?
    set -e
    [[ $celix_rc -eq 0 || $celix_rc -eq 124 ]]
    grep -q 'ITRS_NG_ENUM_EVENT_LOOP_GUARD .*pass=true' <<<"$celix_output"
    grep -q 'ITRS_NG_CELIX_SMOKE rc=0 .*selected=asl-local' <<<"$celix_output"
    grep -q 'ITRS_NG_ENUM_SMOKE rc=0 .*provider=celix-enum-fixture .*ttl=60' <<<"$celix_output"
    echo "PASS: Celix Number, ASL resource plane, and ranked ENUM provider smoke"
fi
