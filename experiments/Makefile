all: bin experiments/bin

bin/main : src
	g++ -g -Wall -Wextra $< -o $@ -lOpenCL

bin/checker : src
	g++ -g -Wall -Wextra $< -o $@

data_gen : src
	rm -rf data/*
	python3 $<

clean:
	rm -rf bin/*

.PHONY: clean data_gen all