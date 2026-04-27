include config.mk

.PHONY: test clean kvikdos

test: kvikdos
	@./tests/runner.sh

kvikdos: $(KVIKDOS)

$(KVIKDOS):
	@if [ ! -f vendor/kvikdos/Makefile ]; then \
	    echo "vendor/kvikdos is empty — did you forget 'git submodule update --init --recursive'?"; \
	    exit 1; \
	fi
	$(MAKE) -C vendor/kvikdos $(KVIKDOS_TARGET)

clean:
	@for d in tests/*/; do \
	    [ -f "$$d/makefile" ] && (cd "$$d" && $(MAKE) -s clean) || true; \
	done
	rm -rf build
