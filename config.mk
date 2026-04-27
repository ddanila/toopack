# Shared toolchain config. Include from per-test makefiles.
#
# OpenWatcom binaries are vendored under vendor/openwatcom-v2 (mirrors the
# layout used by beta_kappa). kvikdos is a submodule under vendor/kvikdos
# and gets built on demand by the top-level Makefile.
#
# All knobs use `?=` so CI / alternative layouts can override via env.

REPO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

WATCOM  ?= $(REPO_ROOT)/vendor/openwatcom-v2/current-build-2026-04-04

# Pick host platform binaries automatically.
HOST_OS := $(shell uname -s)
ifeq ($(HOST_OS),Darwin)
  WBIN ?= $(WATCOM)/macos-arm64
else
  WBIN ?= $(WATCOM)/linux-amd64
endif

WCC     ?= $(WBIN)/wcc
WASM    ?= $(WBIN)/wasm
WLINK   ?= $(WBIN)/wlink
KVIKDOS ?= $(REPO_ROOT)/vendor/kvikdos/kvikdos

export WATCOM
export INCLUDE ?= $(WATCOM)/h

# 16-bit, 8086, small model, optimize for size, no stdio dependencies
WCFLAGS := -ms -bt=dos -0 -os -zl -zq -s
WAFLAGS := -q -0 -ms

# Default link recipe for tiny .COM (no libc, no startup obj).
# Usage: $(call link_com,output.com,obj1.o obj2.o ...)
define link_com
$(WLINK) format dos com name $(1) $(addprefix file ,$(2)) option quiet
endef

# Run a built .COM under kvikdos and diff stdout against expected.txt.
# Strips CR before diff so expected.txt can stay LF-only.
# Usage: $(call run_test,target.com)             — no stdin
#        $(call run_test,target.com,input.txt)   — with stdin
define run_test
@name=$$(basename $$(pwd)); \
$(KVIKDOS) $(if $(2),--tty-in=-2 $(1) < $(2),$(1)) 2>/dev/null | tr -d '\r' > out.tmp; \
size=$$(wc -c < $(1) | tr -d ' '); \
if diff -q out.tmp expected.txt >/dev/null 2>&1; then \
    printf "%-20s PASS  (%4d B)\n" "$$name" "$$size"; \
    rm -f out.tmp; \
else \
    printf "%-20s FAIL  (%4d B)\n" "$$name" "$$size"; \
    echo "  expected:"; sed 's/^/    /' expected.txt; \
    echo "  got:";      sed 's/^/    /' out.tmp; \
    rm -f out.tmp; \
    exit 1; \
fi
endef
