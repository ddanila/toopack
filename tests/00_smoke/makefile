include ../../config.mk

OBJS := start.o hello.o
TARGET := hello.com

.PHONY: all run clean test

all: $(TARGET)

start.o: start.asm
	$(WASM) $(WAFLAGS) -fo=$@ $<

hello.o: hello.c ../../src/dos.h
	$(WCC) $(WCFLAGS) -fo=$@ $<

$(TARGET): $(OBJS)
	$(call link_com,$@,$(OBJS))

run: $(TARGET)
	$(KVIKDOS) $(TARGET)

test: $(TARGET) expected.txt
	$(call run_test,$(TARGET))

clean:
	rm -f *.o $(TARGET) out.tmp
