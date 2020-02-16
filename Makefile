release: FORCE
	python3 build.py -r

debug: FORCE
	python3 build.py -d

clean: FORCE
	python3 build.py -c

run: FORCE
	python3 build.py -r -e

FORCE: