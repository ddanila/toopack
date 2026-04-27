include ../../config.mk

OBJS := start.o main.o
TARGET := alloc.com

.PHONY: all run clean test

all: $(TARGET)

start.o: ../00_smoke/start.asm
	$(WASM) $(WAFLAGS) -fo=$@ $<

main.o: main.c ../../src/dos.h
	$(WCC) $(WCFLAGS) -fo=$@ main.c

$(TARGET): $(OBJS)
	$(call link_com,$@,$(OBJS))

run: $(TARGET)
	$(KVIKDOS) $(TARGET)

test: $(TARGET) expected.txt
	$(call run_test,$(TARGET))

clean:
	rm -f *.o $(TARGET) out.tmp
