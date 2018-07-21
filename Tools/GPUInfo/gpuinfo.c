//
// GPU Info
// Based on freevram and MetalInfo tools.
// See: https://stackoverflow.com/questions/3783030/free-vram-on-os-x
// 

#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>

#include <objc/runtime.h>
#include <objc/message.h>

#include <IOKit/graphics/IOGraphicsLib.h>
#include <CoreFoundation/CoreFoundation.h>

uint64_t currentFreeVRAM(uint64_t *total) {
    kern_return_t krc;  
    mach_port_t masterPort;
    krc = IOMasterPort(bootstrap_port, &masterPort);

    if (total)
        *total = 0;

    if (krc == KERN_SUCCESS) {
        CFMutableDictionaryRef pattern = IOServiceMatching(kIOAcceleratorClassName);

        io_iterator_t deviceIterator;
        krc = IOServiceGetMatchingServices(masterPort, pattern, &deviceIterator);
        if (krc == KERN_SUCCESS) {
            io_object_t object;
            while ((object = IOIteratorNext(deviceIterator))) {
                CFMutableDictionaryRef properties = NULL;
                krc = IORegistryEntryCreateCFProperties(object, &properties, kCFAllocatorDefault, (IOOptionBits)0);
                if (krc == KERN_SUCCESS) {
                    const void *total_vram_number = CFDictionaryGetValue(properties, CFSTR("VRAM,totalMB"));
                    if (total_vram_number)
                        CFNumberGetValue((CFNumberRef) total_vram_number, kCFNumberSInt64Type, total);
                    CFMutableDictionaryRef perf_properties = (CFMutableDictionaryRef)CFDictionaryGetValue(properties, CFSTR("PerformanceStatistics") );

                    // Look for a number of keys (this is mostly reverse engineering and best-guess effort)
                    const void *free_vram_number = CFDictionaryGetValue(perf_properties, CFSTR("vramFreeBytes"));
                    if (free_vram_number) {
                        ssize_t vramFreeBytes;
                        CFNumberGetValue((CFNumberRef)free_vram_number, kCFNumberSInt64Type, &vramFreeBytes);
                        return vramFreeBytes;
                    }
                }
                if (properties)
                    CFRelease(properties);
                IOObjectRelease(object);
            }
            IOObjectRelease(deviceIterator);
        }
    }
    return 0;
}

int main() {
    uint64_t total_vram = 0;
    uint64_t free_vram = currentFreeVRAM(&total_vram);
    if (total_vram > 0)
        printf("Total VRAM availabile: %zu MB\n", (size_t)total_vram);
    if (free_vram > 0)
        printf("Free VRAM availabile: %zu MB (%zu Bytes)\n", (size_t)(free_vram/(1024*1024)), (size_t)free_vram);

    void *mtl = dlopen("/System/Library/Frameworks/Metal.framework/Metal", RTLD_LAZY);
    id (*create_dev)(void) = (id (*)(void))dlsym(mtl, "MTLCreateSystemDefaultDevice");
    if (create_dev) {
        id device = create_dev();
        if (device) {
            id name     = objc_msgSend(device, sel_registerName("name"));
            id lowpower = objc_msgSend(device, sel_registerName("isLowPower"));
            id headless = objc_msgSend(device, sel_registerName("isHeadless"));
            printf("Metal device name: %s\n", (const char *)objc_msgSend(name, sel_registerName("UTF8String")));
            printf("Low Power: %s\n", lowpower ? "Yes" : "No");
            printf("Headless: %s\n", headless ? "Yes" : "No");
        } else {
            printf("Metal is not supported by your hardware!\n");
        }
    } else {
        printf("Metal is not supported by this operating system!\n");
    }
}
