#ifndef RUNTIME_H
#define RUNTIME_H

#include <iostream>

#include "platform.h"

class Runtime {
public:
    struct Mem {
        device_id dev;
        int64_t size;
        Mem() {}
        Mem(device_id dev, int64_t size)
            : dev(dev), size(size)
        {}
    };

    struct View {
        void* ptr;
        int64_t off;
        int64_t size;

        View() {}
        View(void* ptr, int64_t off, int64_t size)
            : ptr(ptr), off(off), size(size)
        {}
    };

    struct Device {
        int index;
        Platform* p;

        Device() {}
        Device(int index, Platform* p)
            : index(index), p(p)
        {}
    };

    Runtime();

    ~Runtime() {
        assert(mems_.size() == 0 && "Some memory blocks have not been released");
        for (auto p: platforms_) {
            delete p;
        }
    }

    /// Registers the given platform into the runtime.
    template <typename T, typename... Args>
    void register_platform(Args... args) {
        Platform* p = new T(this, args...);
        platforms_.push_back(p);

        for (int i = 0; i < p->dev_count(); i++)
            devices_.emplace_back(i, p);
    }

    /// Displays available platforms.
    void display_info(std::ostream& os) {
        os << "Available platforms:" << std::endl;
        for (auto p: platforms_) {
            os << "    * " << p->name() << ": " << p->dev_count() << " device(s)" << std::endl;
        }
    }

    /// Returns the device corresponding to the given ID.
    const Device& device(device_id dev) const {
        return devices_[dev];    
    }

    /// Allocates memory on the given device.
    void* alloc(device_id dev, int64_t size) {
        if (dev >= devices_.size())
            error("Device ", dev, "is not available (there is only ", devices_.size(), "devices)");

        void* ptr = devices_[dev].p->alloc(dev, size);
        mems_.emplace(ptr, Mem(dev, size));
        return ptr;
    }

    /// Releases memory.
    void release(void* ptr) {
        auto it = mems_.find(ptr);
        if (it == mems_.end())
            error("Memory not allocated from runtime");

        devices_[it->second.dev].p->release(ptr);
        mems_.erase(it);
    }

    void set_block_size(device_id dev, uint32_t x, uint32_t y, uint32_t z) {
        devices_[dev].p->set_block_size(dev, x, y, z);
    }

    void set_grid_size(device_id dev, uint32_t x, uint32_t y, uint32_t z) {
        devices_[dev].p->set_grid_size(dev, x, y, z); 
    }

    void set_arg(device_id dev, uint32_t arg, void* ptr) {
        devices_[dev].p->set_arg(dev, arg, ptr);
    }

    void load_kernel(device_id dev, const char* file, const char* name) {
        devices_[dev].p->load_kernel(dev, file, name);
    }

    void launch_kernel(device_id dev) {
        devices_[dev].p->launch_kernel(dev);
    }

    /// Copies memory.
    void copy(const void* src, void* dst) {
        auto src_mem = mems_.find((void*)src);
        auto dst_mem = mems_.find(dst);
        if (src_mem == mems_.end() || dst_mem == mems_.end())
            error("Memory not allocated from runtime");

        auto& src_dev = devices_[src_mem->second.dev];
        auto& dst_dev = devices_[dst_mem->second.dev];
        if (src_dev.p == dst_dev.p) {
            // Copy from same platform
            src_dev.p->copy(src, dst);
        } else {
            // Copy from another platform
            if (src_mem->second.dev == 0) {
                // Source is the CPU platform
                dst_dev.p->copy_from_host(src, dst);
            } else if (dst_mem->second.dev == 0) {
                // Destination is the CPU platform
                src_dev.p->copy_to_host(src, dst);
            } else {
                error("Cannot copy memory between different platforms");
            }
        }
    }

    Mem memory_info(const void* ptr) {
        auto it = mems_.find((void*)ptr);
        assert(it != mems_.end());
        return it->second;
    }

    template <typename... Args>
    void error(Args... args) {
        std::cerr << "Runtime error: ";
        print(args...);
        std::abort();
    }

    template <typename... Args>
    void log(Args... args) {
#ifndef NDEBUG
        std::cerr << "Runtime message: ";
        print(args...);
#endif
    }

private:
    template <typename T, typename... Args>
    static void print(T t, Args... args) {
        std::cerr << t;
        print(args...);
    }

    template <typename T>
    static void print(T t) {
        std::cerr << t << std::endl;
    }

    std::vector<Platform*> platforms_;
    std::vector<Device> devices_;
    std::unordered_map<void*, Mem> mems_;
};

#endif
