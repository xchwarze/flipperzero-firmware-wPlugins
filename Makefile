.PHONY: update
update:
	git submodule update --remote lib/micropython && git add lib/micropython

.PHONY: build
build: update
	ufbt build

.PHONY: launch
launch: build
	ufbt launch

.PHONY: clean
clean:
	ufbt -c

.PHONY: pages
pages:
	source venv/bin/activate && sphinx-build docs/pages dist/pages
