include ../../config.mk

SRCDIR  := ../../src
LZSADEC := ../../vendor/lzsa/asm/8088/decompress_small_v2.S

OBJS := start.o main.o ovly.o unbcj.o unlzsa2.o api.o
TARGET := apicall.com

.PHONY: all run clean test

all: $(TARGET)

start.o: ../00_smoke/start.asm
	$(WASM) $(WAFLAGS) -fo=$@ $<

api.o: $(SRCDIR)/api.asm
	$(WASM) $(WAFLAGS) -fo=$@ $<

cold.bin: cold.nasm
	nasm -f bin $< -o $@

cold_packed.h: cold.bin ../../tools/pack.sh
	../../tools/pack.sh cold.bin cold $@

main.o: main.c cold_packed.h $(SRCDIR)/dos.h $(SRCDIR)/api.h $(SRCDIR)/ovly.h
	$(WCC) $(WCFLAGS) -fo=$@ main.c

ovly.o: $(SRCDIR)/ovly.c $(SRCDIR)/ovly.h $(SRCDIR)/unlzsa2.h $(SRCDIR)/unbcj.h $(SRCDIR)/dos.h
	$(WCC) $(WCFLAGS) -fo=$@ $(SRCDIR)/ovly.c

unbcj.o: $(SRCDIR)/unbcj.c $(SRCDIR)/unbcj.h
	$(WCC) $(WCFLAGS) -fo=$@ $(SRCDIR)/unbcj.c

unlzsa2.inc: $(LZSADEC) ../../tools/gen_decoder_inc.sh
	../../tools/gen_decoder_inc.sh $(LZSADEC) $@

unlzsa2.o: $(SRCDIR)/unlzsa2.asm unlzsa2.inc
	$(WASM) $(WAFLAGS) -i=. -fo=$@ $(SRCDIR)/unlzsa2.asm

$(TARGET): $(OBJS)
	$(call link_com,$@,$(OBJS))

run: $(TARGET)
	$(KVIKDOS) $(TARGET)

test: $(TARGET) expected.txt
	$(call run_test,$(TARGET))

clean:
	rm -f *.o *.inc *.com *.bin cold_packed.h out.tmp
