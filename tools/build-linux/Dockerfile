# Container image for the Linux build of the wiz3D DX9 path (build-linux.sh).
# debian:testing + clang-cl/lld + xwin (Windows SDK, CRT and ATL for x86).
FROM debian:testing
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    clang lld llvm llvm-tools ninja-build python3 ca-certificates curl xz-utils \
 && rm -rf /var/lib/apt/lists/*
# xwin: static musl binary from GitHub releases
ARG XWIN_VERSION=0.6.7
RUN curl -fsSL https://github.com/Jake-Shadle/xwin/releases/download/${XWIN_VERSION}/xwin-${XWIN_VERSION}-x86_64-unknown-linux-musl.tar.gz \
    | tar -xz --strip-components=1 -C /usr/local/bin \
 && xwin --version
# Windows SDK + MSVC CRT + ATL, x86 and x86_64, splatted into the image.
RUN xwin --accept-license --arch x86,x86_64 --include-atl splat --output /opt/xwin \
 && rm -rf /.xwin-cache \
 && ls /opt/xwin/crt/lib/x86/atls.lib /opt/xwin/crt/lib/x86_64/atls.lib
