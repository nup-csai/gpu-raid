#define _GNU_SOURCE
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define BLOCK_SIZE   (4 * 1024 * 1024)  // 4 MiB
#define ALIGNMENT    4096               // ≥ page size
#define VECTOR_WIDTH 16                 // uchar16 in kernel
#define LOCAL_WS     256

#define CHECK_CL_ERR(err, msg) \
    if ((err) != CL_SUCCESS) { \
        fprintf(stderr, "%s:%d: %s failed (%d)\n", __FILE__, __LINE__, msg, (err)); \
        exit(EXIT_FAILURE); \
    }

static inline void perror_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

const char *kernel_src =
        "__kernel void xor_kernel(__global const uchar16 *a,\n"
        "                         __global const uchar16 *b,\n"
        "                         __global       uchar16 *c) {\n"
        "    size_t gid = get_global_id(0);\n"
        "    c[gid] = a[gid] ^ b[gid];\n"
        "}\n";

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <in1> <in2> <out>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *in1_path = argv[1];
    const char *in2_path = argv[2];
    const char *out_path = argv[3];

    // --- open devices with O_DIRECT ---
    int fd1 = open(in1_path, O_RDONLY | O_DIRECT);
    if (fd1 < 0) perror_exit("open in1");
    int fd2 = open(in2_path, O_RDONLY | O_DIRECT);
    if (fd2 < 0) perror_exit("open in2");
    int fd3 = open(out_path, O_RDWR   | O_DIRECT | O_CREAT, 0644);
    if (fd3 < 0) perror_exit("open out");

    // --- allocate aligned host buffers ---
    uint8_t *h_buf1, *h_buf2, *h_out;
    if (posix_memalign((void**)&h_buf1, ALIGNMENT, BLOCK_SIZE) ||
        posix_memalign((void**)&h_buf2, ALIGNMENT, BLOCK_SIZE) ||
        posix_memalign((void**)&h_out,  ALIGNMENT, BLOCK_SIZE)) {
        fprintf(stderr, "posix_memalign failed\n");
        return EXIT_FAILURE;
    }

    // --- set up OpenCL ---
    cl_int err;
    cl_uint num_platforms = 0;
    CHECK_CL_ERR(clGetPlatformIDs(0, NULL, &num_platforms), "clGetPlatformIDs");
    if (num_platforms == 0) {
        fprintf(stderr, "No OpenCL platforms found\n");
        return EXIT_FAILURE;
    }
    cl_platform_id *platforms = malloc(sizeof(*platforms) * num_platforms);
    CHECK_CL_ERR(clGetPlatformIDs(num_platforms, platforms, NULL), "clGetPlatformIDs");

    // pick first platform
    cl_platform_id platform = platforms[0];
    free(platforms);

    // pick GPU device
    cl_device_id device;
    CHECK_CL_ERR(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL),
                 "clGetDeviceIDs");

    // context & command queue
    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL_ERR(err, "clCreateContext");
    cl_command_queue queue = clCreateCommandQueue(ctx, device,
                                                  CL_QUEUE_PROFILING_ENABLE, &err);
    CHECK_CL_ERR(err, "clCreateCommandQueue");

    // build program
    cl_program prog = clCreateProgramWithSource(ctx, 1, &kernel_src, NULL, &err);
    CHECK_CL_ERR(err, "clCreateProgramWithSource");
    err = clBuildProgram(prog, 1, &device, "-cl-fast-relaxed-math", NULL, NULL);
    if (err != CL_SUCCESS) {
        // print build log
        size_t log_size;
        clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size + 1);
        clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        log[log_size] = '\0';
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        CHECK_CL_ERR(err, "clBuildProgram");
    }

    // kernel
    cl_kernel kernel = clCreateKernel(prog, "xor_kernel", &err);
    CHECK_CL_ERR(err, "clCreateKernel");

    // device buffers
    size_t vector_count = BLOCK_SIZE / VECTOR_WIDTH;
    cl_mem bufA = clCreateBuffer(ctx, CL_MEM_READ_ONLY,
                                 BLOCK_SIZE, NULL, &err);
    CHECK_CL_ERR(err, "clCreateBuffer A");
    cl_mem bufB = clCreateBuffer(ctx, CL_MEM_READ_ONLY,
                                 BLOCK_SIZE, NULL, &err);
    CHECK_CL_ERR(err, "clCreateBuffer B");
    cl_mem bufC = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY,
                                 BLOCK_SIZE, NULL, &err);
    CHECK_CL_ERR(err, "clCreateBuffer C");

    // set kernel args
    CHECK_CL_ERR(clSetKernelArg(kernel, 0, sizeof(bufA), &bufA), "clSetKernelArg 0");
    CHECK_CL_ERR(clSetKernelArg(kernel, 1, sizeof(bufB), &bufB), "clSetKernelArg 1");
    CHECK_CL_ERR(clSetKernelArg(kernel, 2, sizeof(bufC), &bufC), "clSetKernelArg 2");

    // timing
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

    off_t total = 0;
    while (1) {
        // read from inputs
        ssize_t r1 = read(fd1, h_buf1, BLOCK_SIZE);
        if (r1 <= 0) break;
        ssize_t r2 = read(fd2, h_buf2, r1);
        if (r2 < 0) { perror_exit("read in2"); }
        if (r2 != r1) {
            fprintf(stderr, "Warning: read sizes differ (%zd vs %zd)\n", r1, r2);
        }

        size_t bytes = (size_t)r1;
        // must be multiple of VECTOR_WIDTH
        size_t vecs = bytes / VECTOR_WIDTH;

        // enqueue host→device copies
        cl_event evtA, evtB;
        CHECK_CL_ERR(clEnqueueWriteBuffer(queue, bufA, CL_FALSE, 0,
                                          vecs * VECTOR_WIDTH, h_buf1,
                                          0, NULL, &evtA),
                     "clEnqueueWriteBuffer A");
        CHECK_CL_ERR(clEnqueueWriteBuffer(queue, bufB, CL_FALSE, 0,
                                          vecs * VECTOR_WIDTH, h_buf2,
                                          0, NULL, &evtB),
                     "clEnqueueWriteBuffer B");

        // launch kernel
        size_t global_ws = ((vecs + LOCAL_WS - 1) / LOCAL_WS) * LOCAL_WS;
        cl_event evtK;
        CHECK_CL_ERR(clEnqueueNDRangeKernel(queue, kernel, 1, NULL,
                                            &global_ws, (size_t[]){LOCAL_WS},
                                            2, (cl_event[]){evtA, evtB},
                                            &evtK),
                     "clEnqueueNDRangeKernel");

        // read back result
        CHECK_CL_ERR(clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0,
                                         vecs * VECTOR_WIDTH, h_out,
                                         1, &evtK, NULL),
                     "clEnqueueReadBuffer C");

        // write to output
        ssize_t w = write(fd3, h_out, bytes);
        if (w < 0) perror_exit("write out");
        total += w;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec)
                     + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    double gib = (double)total / (1024.0*1024.0*1024.0);
    printf("Processed %.2f GiB in %.3f s → %.2f GiB/s\n",
           gib, elapsed, gib/elapsed);

    // cleanup
    close(fd1);
    close(fd2);
    close(fd3);
    free(h_buf1);
    free(h_buf2);
    free(h_out);
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);

    return EXIT_SUCCESS;
}
