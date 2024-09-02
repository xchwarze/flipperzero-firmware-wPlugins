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
	rm -rf ./dist/pages ./flipperzero/__init__.py
	cat ./flipperzero/_*.py > ./flipperzero/__init__.py
	source venv/bin/activate && sphinx-build docs/pages dist/pages

.PHONY: publish
publish: pages
	./publish.sh pages
