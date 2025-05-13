#define CL_TARGET_OPENCL_VERSION 120

#include <CL/cl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BLOCK_SIZE (100 * 1024 * 1024)  // 100 MB
#define disk1 "/dev/nvme0n1p12"
#define disk2 "/dev/nvme0n1p13"
#define disk3 "/dev/nvme0n1p14"

const char *kernelSource =
"__kernel void xor_buffers(__global const uchar *a, __global const uchar *b, __global uchar *c) {\n"
"    int id = get_global_id(0);\n"
"    c[id] = a[id] ^ b[id];\n"
"}\n";

void checkErr(cl_int err, const char* msg) {
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s failed with error %d\n", msg, err);
        exit(1);
    }
}

double get_duration_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) +
           (end.tv_nsec - start.tv_nsec) / 1e9;
}


int main() {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("Starting XOR operation with OpenCL and block size: %d bytes\n", BLOCK_SIZE);

    int fd1 = open(disk1, O_RDONLY);
    int fd2 = open(disk2, O_RDONLY);
    int fd3 = open(disk3, O_WRONLY);

    if (fd1 < 0 || fd2 < 0 || fd3 < 0) {
        perror("Failed to open disk partitions");
        return 1;
    }

    unsigned char *buf1 = (unsigned char *)malloc(BLOCK_SIZE);
    unsigned char *buf2 = (unsigned char *)malloc(BLOCK_SIZE);
    unsigned char *buf3 = (unsigned char *)malloc(BLOCK_SIZE);

    if (!buf1 || !buf2 || !buf3) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    err = clGetPlatformIDs(1, &platform, NULL);
    checkErr(err, "clGetPlatformIDs");

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    checkErr(err, "clGetDeviceIDs");

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    checkErr(err, "clCreateContext");

    queue = clCreateCommandQueue(context, device, 0, &err);
    checkErr(err, "clCreateCommandQueue");

    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    checkErr(err, "clCreateProgramWithSource");

    //printf("Was here\n");

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        char buildLog[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
        fprintf(stderr, "Build log:\n%s\n", buildLog);
        checkErr(err, "clBuildProgram");
    }

    kernel = clCreateKernel(program, "xor_buffers", &err);
    checkErr(err, "clCreateKernel");

    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, BLOCK_SIZE, NULL, &err);
    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, BLOCK_SIZE, NULL, &err);
    cl_mem bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, BLOCK_SIZE, NULL, &err);

    size_t total_xored = 0;
    ssize_t bytes1, bytes2;

    while (1) {
        bytes1 = read(fd1, buf1, BLOCK_SIZE);
        bytes2 = read(fd2, buf2, BLOCK_SIZE);

        if (bytes1 < 0 || bytes2 < 0) {
            perror("Read error");
            break;
        }

        if (bytes1 == 0 || bytes2 == 0)
            break;

        if (bytes1 != bytes2) {
            fprintf(stderr, "Mismatched read sizes: %zd vs %zd\n", bytes1, bytes2);
            break;
        }

        err = clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, bytes1, buf1, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, bytes1, buf2, 0, NULL, NULL);
        checkErr(err, "clEnqueueWriteBuffer");

        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
        err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
        err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);
        checkErr(err, "clSetKernelArg");

        size_t global_size = bytes1;
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
        checkErr(err, "clEnqueueNDRangeKernel");

        err = clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, bytes1, buf3, 0, NULL, NULL);
        checkErr(err, "clEnqueueReadBuffer");

        ssize_t written = write(fd3, buf3, bytes1);
        if (written != bytes1) {
            perror("Write error");
            break;
        }

        total_xored += bytes1;
    }

    close(fd1); close(fd2); close(fd3);
    free(buf1); free(buf2); free(buf3);

    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    clock_gettime(CLOCK_MONOTONIC, &end_time);  // << replace clock()
	double duration = (end_time.tv_sec - start_time.tv_sec) +
                 (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    printf("XOR completed using OpenCL. Total bytes processed: %zu\n", total_xored);
    printf("Time taken: %.2f seconds\n", duration);

    return 0;
}
