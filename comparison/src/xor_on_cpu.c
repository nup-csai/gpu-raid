#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BLOCK_SIZE (100 * 1024 * 1024)  // 100 MB
#define disk1 "/dev/nvme0n1p12"
#define disk2 "/dev/nvme0n1p13"
#define disk3 "/dev/nvme0n1p14"

double get_duration_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) +
           (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(void) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("Starting XOR operation with block size: %d bytes\n", BLOCK_SIZE);

    int fd1 = open(disk1, O_RDONLY);
    int fd2 = open(disk2, O_RDONLY);
    int fd3 = open(disk3, O_WRONLY);

    if (fd1 < 0 || fd2 < 0 || fd3 < 0) {
        fprintf(stderr, "Could not open the partitions. Did you execute with sudo?\n");
        if (fd1 >= 0) close(fd1);
        if (fd2 >= 0) close(fd2);
        if (fd3 >= 0) close(fd3);
        return 1;
    }

    unsigned char *buf1 = (unsigned char *) malloc(BLOCK_SIZE);
    unsigned char *buf2 = (unsigned char *) malloc(BLOCK_SIZE);
    unsigned char *buf3 = (unsigned char *) malloc(BLOCK_SIZE);

    if (!buf1 || !buf2 || !buf3) {
        fprintf(stderr, "Memory allocation failed\n");
        close(fd1); close(fd2); close(fd3);
        free(buf1); free(buf2); free(buf3);
        return 1;
    }

    ssize_t bytes1, bytes2;
    size_t total_xored = 0;

    while (1) {
        bytes1 = read(fd1, buf1, BLOCK_SIZE);
        bytes2 = read(fd2, buf2, BLOCK_SIZE);

        if (bytes1 < 0 || bytes2 < 0) {
            perror("Read error");
            break;
        }

        if (bytes1 == 0 || bytes2 == 0) {
            break;  // EOF
        }

        if (bytes1 != bytes2) {
            fprintf(stderr, "Mismatched read sizes: %zd vs %zd\n", bytes1, bytes2);
            break;
        }

        for (ssize_t i = 0; i < bytes1; i++) {
            buf3[i] = buf1[i] ^ buf2[i];
        }

        ssize_t written = write(fd3, buf3, bytes1);
        if (written != bytes1) {
            perror("Write error");
            break;
        }

        total_xored += bytes1;
    }

    close(fd1); close(fd2); close(fd3);
    free(buf1); free(buf2); free(buf3);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double duration = get_duration_sec(start_time, end_time);

    printf("XOR completed. Total bytes processed: %zu\n", total_xored);
    printf("Time taken: %.2f seconds\n", duration);

    return 0;
}
