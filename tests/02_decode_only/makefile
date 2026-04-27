include ../../config.mk

NASM    := nasm
SRCDIR  := ../../src
LZSAINC := ../../vendor/lzsa/asm/8088

OBJS := start.o main.o unlzsa2.o unbcj.o
TARGET := decode_only.com

.PHONY: all run clean

all: $(TARGET)

start.o: ../00_smoke/start.asm
	$(WASM) $(WAFLAGS) -fo=$@ $<

main.o: main.c sample_packed.h $(SRCDIR)/dos.h $(SRCDIR)/unlzsa2.h $(SRCDIR)/unbcj.h
	$(WCC) $(WCFLAGS) -fo=$@ main.c

unbcj.o: $(SRCDIR)/unbcj.c $(SRCDIR)/unbcj.h
	$(WCC) $(WCFLAGS) -fo=$@ $(SRCDIR)/unbcj.c

unlzsa2.o: $(SRCDIR)/unlzsa2.S $(LZSAINC)/decompress_small_v2.S
	$(NASM) -f obj -I $(LZSAINC)/ $(SRCDIR)/unlzsa2.S -o $@

$(TARGET): $(OBJS)
	$(call link_com,$@,$(OBJS))

clean:
	rm -f *.o $(TARGET)
