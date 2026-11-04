# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

# Install build dependencies for Unikraft core and cross-compilation
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        bison \
        build-essential \
        ca-certificates \
        curl \
        flex \
        gawk \
        gcc \
        gcc-aarch64-linux-gnu \
        gdb \
        gdb-multiarch \
        git \
        libfdt-dev \
        libncurses-dev \
        libssl-dev \
        make \
        ninja-build \
        patch \
        perl \
        pkg-config \
        python3 \
        python3-pip \
        qemu-system-arm \
        qemu-system-x86 \
        socat \
        unzip \
        wget \
        xz-utils \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install KraftKit CLI for building and running unikernels
RUN curl -fsSL https://get.kraftkit.sh | sh
