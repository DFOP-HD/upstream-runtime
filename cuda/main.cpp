#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "cu_runtime.h"

static int num = 1024;

int main_impala() {
    size_t dev = 0;
    int *host = (int *)array(sizeof(int), num, 1);

    for (unsigned int i=0; i<num; ++i) {
        host[i] = 0;
    }

    // CODE TO BE GENERATED: BEGIN
    mem_id mem = malloc_memory(dev, host);
    write_memory(dev, mem, host);

    load_kernel(dev, "simple-gpu64.nvvm", "simple");
    set_kernel_arg_map(dev, mem);
    set_problem_size(dev, 1024, 1, 1);
    set_config_size(dev, 128, 1, 1);
    launch_kernel(dev, "simple");
    synchronize(dev); // optional
    read_memory(dev, mem, host);
    free_memory(dev, mem);
    // CODE TO BE GENERATED: END

    // check result
    for (unsigned int i=0; i<num; ++i) {
        if (host[i] != i) {
            printf("Test failed!\n");
            return EXIT_FAILURE;
        }
    }
    printf("Test passed!\n");

    for (unsigned int i=0; i<num; ++i) {
        host[i] = i;
    }


    // CODE TO BE GENERATED: BEGIN
    CUdeviceptr tex = malloc_memory(dev, host);
    write_memory(dev, tex, host);

    int *host_out = (int *)array(sizeof(int), num, 1);
    CUdeviceptr out = malloc_memory(dev, host_out);
    for (unsigned int i=0; i<num; ++i) {
        host_out[i] = 0;
    }
    write_memory(dev, out, host_out);

    load_kernel(dev, "simple-gpu64.nvvm", "simple_tex");
    get_tex_ref(dev, "texture");
    bind_tex(dev, tex, CU_AD_FORMAT_SIGNED_INT32);
    set_kernel_arg_map(dev, out);
    set_problem_size(dev, 1024, 1, 1);
    set_config_size(dev, 128, 1, 1);
    launch_kernel(dev, "simple");
    synchronize(dev); // optional
    read_memory(dev, out, host_out);
    free_memory(dev, out);
    free_memory(dev, tex);
    // CODE TO BE GENERATED: END

    // check result
    for (unsigned int i=0; i<num; ++i) {
        if (host_out[i] != i) {
            printf("Texture test failed!\n");
            return EXIT_FAILURE;
        }
    }
    printf("Texture test passed!\n");

    free(host);
    free(host_out);

    return EXIT_SUCCESS;
}

