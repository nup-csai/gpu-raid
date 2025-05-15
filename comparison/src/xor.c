#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define BLOCK_SIZE    (4 * 1024 * 1024)
#define ALIGNMENT     4096

static inline void xor_buffers(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] ^= src[i];
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input1> <input2> <output>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *in1_path = argv[1];
    const char *in2_path = argv[2];
    const char *out_path = argv[3];

    int fd1 = open(in1_path, O_RDONLY | O_DIRECT);
    if (fd1 < 0) {
        perror("Opening first input device");
        return EXIT_FAILURE;
    }
    int fd2 = open(in2_path, O_RDONLY | O_DIRECT);
    if (fd2 < 0) {
        perror("Opening second input device");
        close(fd1);
        return EXIT_FAILURE;
    }
    int fd3 = open(out_path, O_RDWR | O_DIRECT);
    if (fd3 < 0) {
        perror("Opening output device");
        close(fd1);
        close(fd2);
        return EXIT_FAILURE;
    }

    uint8_t *buf1, *buf2;
    if (posix_memalign((void **)&buf1, ALIGNMENT, BLOCK_SIZE) != 0 ||
        posix_memalign((void **)&buf2, ALIGNMENT, BLOCK_SIZE) != 0) {
        fprintf(stderr, "posix_memalign failed\n");
        close(fd1); close(fd2); close(fd3);
        return EXIT_FAILURE;
    }

    struct timespec t_start, t_end;
    off_t total_bytes = 0;

    if (clock_gettime(CLOCK_MONOTONIC_RAW, &t_start) < 0) {
        perror("clock_gettime");
        return EXIT_FAILURE;
    }

    while (1) {
        ssize_t r1 = read(fd1, buf1, BLOCK_SIZE);
        if (r1 < 0) {
            perror("read input1");
            break;
        }
        if (r1 == 0) {
            break;
        }

        ssize_t r2 = read(fd2, buf2, r1);
        if (r2 < 0) {
            perror("read input2");
            break;
        }
        if (r2 != r1) {
            fprintf(stderr, "Warning: mismatched block sizes (%zd vs %zd)\n", r1, r2);
        }

        xor_buffers(buf1, buf2, r1);

        ssize_t w = write(fd3, buf1, r1);
        if (w < 0) {
            perror("write output");
            break;
        }
        if (w != r1) {
            fprintf(stderr, "Short write: %zd of %zd bytes\n", w, r1);
            break;
        }

        total_bytes += w;
    }

    if (clock_gettime(CLOCK_MONOTONIC_RAW, &t_end) < 0) {
        perror("clock_gettime");
    } else {
        double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
        double gib = (double)total_bytes / (1024.0 * 1024.0 * 1024.0);
        printf("Processed %.2f GiB in %.3f s => %.2f GiB/s\n", gib, elapsed, gib / elapsed);
    }

    free(buf1);
    free(buf2);
    close(fd1);
    close(fd2);
    close(fd3);

    return EXIT_SUCCESS;
}
