.PHONY: test clean kvikdos

KVIKDOS_BIN := vendor/kvikdos/kvikdos

test: kvikdos
	@./tests/runner.sh

kvikdos: $(KVIKDOS_BIN)

$(KVIKDOS_BIN):
	@if [ ! -f vendor/kvikdos/Makefile ]; then \
	    echo "vendor/kvikdos is empty — did you forget 'git submodule update --init --recursive'?"; \
	    exit 1; \
	fi
	$(MAKE) -C vendor/kvikdos kvikdos

clean:
	@for d in tests/*/; do \
	    [ -f "$$d/makefile" ] && (cd "$$d" && $(MAKE) -s clean) || true; \
	done
	rm -rf build
