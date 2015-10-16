#include <iostream>
#include <cassert>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <random>
#include <atomic>

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "thorin_runtime.h"

#include "runtime.h"
#include "platform.h"
#include "cpu_platform.h"
#ifdef HAS_CUDA
#include "cuda_platform.h"
#endif

static Runtime runtime;

Runtime::Runtime() {
    register_platform<CpuPlatform>();
#ifdef HAS_CUDA
    register_platform<CudaPlatform>();
#endif
}

void thorin_info(void) {
    runtime.display_info(std::cout);
}

void* thorin_alloc(uint32_t dev, int64_t size) {
    return runtime.alloc((device_id)dev, size);
}

void thorin_release(void* ptr) {
    runtime.release(ptr);
}

void* thorin_map(void* ptr, int64_t offset, int64_t size) {
    assert(0 && "Not implemented");
    return nullptr;
}

void thorin_unmap(void* view) {
    assert(0 && "Not implemented");
}

void thorin_copy(const void* src, void* dst) {
    runtime.copy(src, dst);
}

void thorin_set_block_size(uint32_t dev, uint32_t x, uint32_t y, uint32_t z) {
    runtime.set_block_size((device_id)dev, x, y, z);
}

void thorin_set_grid_size(uint32_t dev, uint32_t x, uint32_t y, uint32_t z) {
    runtime.set_grid_size((device_id)dev, x, y, z);
}

void thorin_set_arg(uint32_t dev, uint32_t arg, void* ptr) {
    runtime.set_arg((device_id)dev, arg, ptr);
}

void thorin_load_kernel(uint32_t dev, const char* file, const char* name) {
    runtime.load_kernel((device_id)dev, file, name);
}

void thorin_launch_kernel(uint32_t dev) {
    runtime.launch_kernel((device_id)dev);
}

#if _POSIX_VERSION >= 200112L || _XOPEN_SOURCE >= 600
void* thorin_aligned_malloc(size_t size, size_t alignment) {
    void* p;
    posix_memalign(&p, alignment, size);
    return p;
}
void thorin_aligned_free(void* ptr) { free(ptr); }
#elif _ISOC11_SOURCE
void* thorin_aligned_malloc(size_t size, size_t alignment) { return ::aligned_alloc(alignment, size); }
void thorin_aligned_free(void* ptr) { ::free(ptr); }
#elif defined(_WIN32) || defined(__CYGWIN__)
#include <malloc.h>

void* thorin_aligned_malloc(size_t size, size_t alignment) { return ::_aligned_malloc(size, alignment); }
void thorin_aligned_free(void* ptr) { ::_aligned_free(ptr); }
#else
#error "There is no way to allocate aligned memory on this system"
#endif

long long thorin_get_micro_time() {
#if defined(_WIN32) || defined(__CYGWIN__) // Use QueryPerformanceCounter on Windows
    LARGE_INTEGER counter, freq;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&freq);
    return counter.QuadPart * 1000000LL / freq.QuadPart;
#else
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
}

std::atomic_llong thorin_kernel_time(0);

long long thorin_get_kernel_time() {
    return thorin_kernel_time;
}

void thorin_print_char(char c)      { std::cout << c; }
void thorin_print_int(int i)        { std::cout << i; }
void thorin_print_long(long long l) { std::cout << l; }
void thorin_print_float(float f)    { std::cout << f; }
void thorin_print_double(double d)  { std::cout << d; }
void thorin_print_string(char* s)   { std::cout << s; }

#if defined(__APPLE__) && defined(__clang__)
#pragma message("Runtime random function is not thread-safe")
static std::mt19937 std_gen;
#else
static thread_local std::mt19937 std_gen;
#endif
static std::uniform_real_distribution<float> std_dist(0.0f, 1.0f);

void thorin_random_seed(unsigned seed) {
    std_gen.seed(seed);
}

float thorin_random_val() {
    return std_dist(std_gen);
}
