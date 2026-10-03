#!/bin/sh
# Linux build of the wiz3D Direct3D 9 path (Release): the d3d9.dll proxy
# loader (wiz3D-proxy/d3d9), the S3DWrapperD3D9.dll wrapper, S3DAPI.dll,
# ZLOg.dll and the SideBySideOutput plug-in. Runs in a debian:testing
# container with clang-cl + lld-link and an xwin-splatted Windows SDK, MSVC
# CRT and ATL. Requires only Docker. Sources are mounted read-only.
#
# Usage: ./build-linux.sh [x64|x86|all]      (default: x64)
#   OUT=dir          where out/<arch>/ and the manifest go (default: ./out)
#   BUILD=dir        build tree (default: ./build-linux-obj)
#   WIZ3D_WITH_SR=0  Leia SR output stays out (the DX9 chain never uses it;
#                    SimulatedRealityWeaveOutput is not part of this build)
#   JOBS=4           parallel compile jobs (maximum 4)
set -e
SOL=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ARCHS=${1:-x64}
[ "$ARCHS" = all ] && ARCHS="x64 x86"
OUT=${OUT:-$SOL/out}
BUILD=${BUILD:-$SOL/build-linux-obj}
JOBS=${JOBS:-4}
case "$JOBS" in 1|2|3|4) ;; *) echo "JOBS must be between 1 and 4" >&2; exit 2 ;; esac
IMAGE=wiz3d-linux-build:latest
[ "${WIZ3D_WITH_SR:-0}" = 0 ] || { echo "WIZ3D_WITH_SR=1 is not supported by the DX9 chain build" >&2; exit 2; }

mkdir -p "$OUT" "$BUILD"
SHA=$(git -C "$SOL" rev-parse --short HEAD 2>/dev/null || echo unknown)
git -C "$SOL" diff --quiet 2>/dev/null || SHA="$SHA-dirty"
DATE=$(git -C "$SOL" log -1 --format=%cs 2>/dev/null || echo 1970-01-01)
OUT=$(CDPATH= cd -- "$OUT" && pwd)
BUILD=$(CDPATH= cd -- "$BUILD" && pwd)
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "building image $IMAGE ..."
    DOCKER_BUILDKIT=0 docker build --cpu-quota=400000 -t "$IMAGE" "$SOL/tools/build-linux"
fi
for ARCH in $ARCHS; do
CONTAINER=$(docker run -d --cpus 4 -u "$(id -u):$(id -g)" -e WIZ3D_ARCH=$ARCH -e ARCH=$ARCH -e JOBS=$JOBS -e SHA=$SHA -e DATE=$DATE \
    -v "$SOL:$SOL:ro" -v "$OUT:/out" -v "$BUILD:/build" "$IMAGE" sh -c '
set -e
SOL="'"$SOL"'"
XA=x86_64; [ "$ARCH" = x86 ] && XA=x86
B=/build/$ARCH
mkdir -p $B
python3 "$SOL/tools/build-linux/make_vfs.py" $B/vfs.json "$SOL" /opt/xwin/crt/include /opt/xwin/sdk/include
python3 "$SOL/tools/build-linux/gen_version.py" "$SOL" $B/ver "$SHA" "$DATE"
XWIN_DIR=/opt/xwin python3 "$SOL/tools/build-linux/gen_build.py" "$SOL" $B
ninja -C $B -j$JOBS -k 0 all
python3 "$SOL/tools/build-linux/stage.py" $B /out/$ARCH "$SOL" $ARCH
')
trap 'docker rm -f "$CONTAINER" >/dev/null' EXIT HUP INT TERM
RESULT=$(docker wait "$CONTAINER")
docker logs "$CONTAINER"
docker rm "$CONTAINER" >/dev/null
trap - EXIT HUP INT TERM
[ "$RESULT" = 0 ] || exit "$RESULT"
done
( cd "$OUT" && find . -type f ! -name SHA256SUMS -print | sort | xargs sha256sum > SHA256SUMS )
echo "=== $OUT ==="; cat "$OUT/SHA256SUMS"
