all: bin/main bin/checker

bin/main : src/main.cpp
	g++ -g -Wall -Wextra $< -o $@ -lOpenCL

bin/checker : src/checker.cpp
	g++ -g -Wall -Wextra $< -o $@

data_gen : src/gen.py
	rm -rf data/*
	python3 $<

clean:
	rm -rf bin/*

.PHONY: clean data_gen all