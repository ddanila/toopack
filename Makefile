.PHONY: test clean

test:
	@./tests/runner.sh

clean:
	@for d in tests/*/; do \
	    [ -f "$$d/makefile" ] && (cd "$$d" && make -s clean) || true; \
	done
	rm -rf build
