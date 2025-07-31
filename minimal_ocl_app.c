#define CL_TARGET_OPENCL_VERSION 120 // For OpenCL 1.2
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <stdio.h>

int main() {
    cl_platform_id platform_id = NULL;
    cl_uint num_platforms;
    cl_int ret;

    printf("Attempting clGetPlatformIDs...\n");
    ret = clGetPlatformIDs(1, &platform_id, &num_platforms);
    if (ret == CL_SUCCESS) {
        printf("clGetPlatformIDs successful. Number of platforms: %u\n", num_platforms);
        if (num_platforms > 0) {
            char platform_name[128];
            ret = clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
            if (ret == CL_SUCCESS) {
                printf("Platform name: %s\n", platform_name);
            } else {
                printf("clGetPlatformInfo failed with %d\n", ret);
            }

            cl_device_id device_id = NULL;
            cl_uint num_devices;
            printf("Attempting clGetDeviceIDs...\n");
            ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &num_devices);
            if (ret == CL_SUCCESS) {
                printf("clGetDeviceIDs successful for GPU. Number of devices: %u\n", num_devices);
            } else if (ret == CL_DEVICE_NOT_FOUND) {
                 printf("clGetDeviceIDs: No GPU found. Trying CL_DEVICE_TYPE_DEFAULT.\n");
                 ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &num_devices);
                 if (ret == CL_SUCCESS) {
                    printf("clGetDeviceIDs successful for DEFAULT. Number of devices: %u\n", num_devices);
                 } else {
                    printf("clGetDeviceIDs for DEFAULT failed with %d\n", ret);
                 }
            } else {
                printf("clGetDeviceIDs for GPU failed with %d\n", ret);
            }
        }
    } else {
        printf("clGetPlatformIDs failed with %d\n", ret);
    }
    printf("Minimal OpenCL app finished.\n");
    return 0;
}
