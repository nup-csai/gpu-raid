#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
#include <fstream>
#include <iostream>

using namespace std;

const char *kernel_source = R"(
__kernel void sum(__global const float *A, __global const float *B, __global float *C) {
    int i = get_global_id(0);
    C[i] = A[i] + B[i];
}
)";

vector<float> read_vector(const string &filename, const int size = 26000000) {
    ifstream file(filename);
    vector<float> data(size);
    for (int i = 0; i < size; ++i) {
        file >> data[i];
    }
    file.close();
    return data;
}

int main() {
    try {
        vector<float> A = read_vector("data/vector1.dat");
        vector<float> B = read_vector("data/vector2.dat");
        int size = (int) A.size();
        vector<float> C(size);

        vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        cl::Platform platform = platforms[0];

        cerr << platform.getInfo<CL_PLATFORM_NAME>() << endl;

        vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        cl::Device device = devices[0];

        cerr << device.getInfo<CL_DEVICE_NAME>() << endl;

        cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platform()) , 0 };
        cl::Context context(device, properties);
        cl::CommandQueue queue(context, device);

        cl::Buffer bufferA(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * size, A.data());
        cl::Buffer bufferB(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * size, B.data());
        cl::Buffer bufferC(context, CL_MEM_WRITE_ONLY, sizeof(float) * size);

        cl::Program program(context, kernel_source);
        program.build({device});

        cl::Kernel kernel(program, "sum");
        kernel.setArg(0, bufferA);
        kernel.setArg(1, bufferB);
        kernel.setArg(2, bufferC);

        auto start = chrono::high_resolution_clock::now();

        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(size));
        queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, sizeof(float) * size, C.data());

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> duration = end - start;
        cerr << "EXECUTED IN " << duration.count() << " SECONDS" << endl;
        
		ofstream file("data/results.dat");
        for (int i = 0; i < size; ++i) {
            file << C[i] << '\n';
        }
        file.close();

    } catch (const cl::Error &err) {
        cerr << "OpenCL error: " << err.what() << " (" << err.err() << ")" << endl;
        return 1;
    } catch (const exception &ex) {
        cerr << "Error: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
