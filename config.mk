# Shared toolchain config. Include from per-test makefiles.

WATCOM  := $(HOME)/fun/beta_kappa/vendor/openwatcom-v2/current-build-2026-04-04
WBIN    := $(WATCOM)/macos-arm64
WCC     := $(WBIN)/wcc
WASM    := $(WBIN)/wasm
WLINK   := $(WBIN)/wlink
KVIKDOS := $(HOME)/fun/beta_kappa/kvikdos

export WATCOM
export INCLUDE := $(WATCOM)/h

# 16-bit, 8086, small model, optimize for size, no stdio dependencies
WCFLAGS := -ms -bt=dos -0 -os -zl -zq -s
WAFLAGS := -q -0 -ms

# Default link recipe for tiny .COM (no libc, no startup obj).
# Usage: $(call link_com,output.com,obj1.o obj2.o ...)
define link_com
$(WLINK) format dos com name $(1) $(addprefix file ,$(2)) option quiet
endef
