all: bin/xor_on_cpu bin/xor_on_gpu

bin/xor_on_cpu : src/xor_on_cpu.c
	mkdir -p bin
	gcc -g -Wall -Wextra $< -o $@ -lOpenCL

bin/xor_on_gpu : src/xor_on_gpu.c
	mkdir -p bin
	gcc -g -Wall -Wextra $< -o $@ -lOpenCL

clean:
	rm -rf bin/*

.PHONY: clean all