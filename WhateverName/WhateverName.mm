//
//  main.mm
//  WhateverName
//

#import <Foundation/Foundation.h>

#include <objc/message.h>
#include <objc/runtime.h>
#include <mach-o/dyld.h>
#include <dlfcn.h>
#include <cstring>
#include <cstdio>

#include "rd_route.h"

#ifdef DEBUG
	#define ERRLOG(str, ...) fprintf(stderr, "WhateverName (ERR): " str "\n", ## __VA_ARGS__)
	#define DBGLOG(str, ...) fprintf(stderr, "WhateverName (DBG): " str "\n", ## __VA_ARGS__)
#else
	#define ERRLOG(str, ...) fprintf(stderr, "WhateverName (ERR): " str "\n", ## __VA_ARGS__)
	#define DBGLOG(str, ...) do { } while(0)
#endif

static constexpr size_t MaxCards {8};
static constexpr size_t EngineLength {64};

using t_InitialiseDeviceInfo = uint32_t (*)(uint8_t *device, mach_port_t port);
t_InitialiseDeviceInfo orgInitialiseDeviceInfo[MaxCards] {};

using t_GetNameInfo = uint32_t (*)(uint8_t *device, uint8_t *code);
t_GetNameInfo orgGetNameInfo[MaxCards] {};

static bool getGPUModel(void *accelDevice, mach_port_t port, char *modelName) {
	auto overr = getenv("WE_MODEL_OVERRIDE");
	if (overr) {
		DBGLOG("Found GPU model override: %s", overr);
		snprintf(modelName, EngineLength, "%s", overr);
		return true;
	}
	
	if (accelDevice) {
		static kern_return_t (*IOAccelDeviceGetName)(void *accel, char *ptr, size_t sz);
		
		if (!IOAccelDeviceGetName) {
			auto r = rd_function_byname("IOAccelDeviceGetName", "/System/Library/PrivateFrameworks/IOAccelerator.framework/Versions/A/IOAccelerator",
										reinterpret_cast<void **>(&IOAccelDeviceGetName));
			if (r != KERN_SUCCESS) {
				ERRLOG("Failed to find IOAccelDeviceGetName %X", r);
				return false;
			}
		}
		
		if (IOAccelDeviceGetName) {
			auto r = IOAccelDeviceGetName(accelDevice, modelName, EngineLength);
			if (r == KERN_SUCCESS)
				return true;
			else
				ERRLOG("Failed to device name (ioaccel) %X", r);
		}
	} else {
		// This is reversed from IOAccelDeviceGetName from /System/Library/PrivateFrameworks/IOAccelerator.framework
		size_t modelLen = EngineLength;
		auto r = IOConnectCallStructMethod(port, 1, 0, 0, modelName, &modelLen);
		if (r == KERN_SUCCESS)
			return true;
		else
			ERRLOG("Failed to get device name (direct) %X", r);
	}
	
	return false;
}

enum class ModelSubstr {
	Family,
	Product
};

const char *getGPUModelSubsr(const char *modelName, ModelSubstr type, int &len) {
	static struct {
		const char *name;
		size_t len;
	} familyPrefixes[] {
		{"HD Graphics", strlen("HD Graphics")},
		{"Iris(TM) Graphics", strlen("Iris(TM) Graphics")},
		{"Iris(TM) Pro Graphics", strlen("Iris(TM) Pro Graphics")},
		{"SKL", strlen("SKL")},
		{"KBL", strlen("KBL")},
		{"GeForce", strlen("GeForce")},
		{"Radeon HD", strlen("Radeon HD")},
		{"Radeon Pro", strlen("Radeon Pro")},
		{"FirePro", strlen("FirePro")},
		{"Radeon R9", strlen("Radeon R9")},
		{"Radeon R7", strlen("Radeon R7")}
	};
	
	for (auto pref : familyPrefixes) {
		if (!strncmp(modelName, pref.name, pref.len) && modelName[pref.len] == ' ') {
			if (type == ModelSubstr::Family) {
				len = static_cast<int>(pref.len);
			} else {
				modelName += pref.len + 1;
				len = static_cast<int>(strlen(modelName));
			}
			
			return modelName;
		}
	}
	
	if (type == ModelSubstr::Family)
		len = 0;
	else
		len = static_cast<int>(strlen(modelName));
	
	return modelName;
}

enum EngineOffset {
	GLEngine,
	CLEngine,
	NumEngines
};

void lookupOffsets(uint8_t *device, char *engines[NumEngines], const char *&vendorName) {
	for (size_t i = 0, e = 0; i < 1024 && e < NumEngines; i++) {
		if (device[i] == 'A') {
			if (!memcmp(&device[i], "AMD ", strlen("AMD "))) {
				engines[e++] = reinterpret_cast<char *>(&device[i]);
				i += EngineLength - 1;
				vendorName = "AMD";
			} else if (!memcmp(&device[i], "ATI ", strlen("ATI "))) {
				engines[e++] = reinterpret_cast<char *>(&device[i]);
				i += EngineLength - 1;
				vendorName = "ATI";
			}
		} else if (device[i] == 'N') {
			if (!memcmp(device + i, "NVIDIA ", strlen("NVIDIA "))) {
				engines[e++] = reinterpret_cast<char *>(&device[i]);
				i += EngineLength - 1;
				vendorName = "NVIDIA";
			}
		}
	}
}

template <size_t Index>
static uint32_t initialiseDeviceInfo(uint8_t *device, mach_port_t port) {
	if (!orgInitialiseDeviceInfo[Index]) {
		ERRLOG("Failed to determine the original InitFunc for %zu", Index);
		return 0;
	} else {
		DBGLOG("InitFunc %zu is ready for service", Index);
	}
	
	auto ret = orgInitialiseDeviceInfo[Index](device, port);
	
	char modelName[EngineLength] {};
	if (getGPUModel(nullptr, port, modelName)) {
		DBGLOG("Found GPU name: %s", modelName);
		
		// Intel is special, don't mess with it
		// Fix two spaces for "Compute Engine" on NVIDIA and the complete mess on ATI/AMD
		if (strncmp(modelName, "Intel", strlen("Intel")) != 0) {
			char *engines[NumEngines] {};
			const char *vendorName = nullptr;
			lookupOffsets(device, engines, vendorName);
			
			if (engines[GLEngine]) {
				DBGLOG("Found GL Engine: %s at %lX", engines[GLEngine], static_cast<uintptr_t>(reinterpret_cast<uint8_t *>(engines[GLEngine]) - device));
				snprintf(engines[GLEngine], EngineLength, "%s %s OpenGL Engine", vendorName, modelName);
				DBGLOG("Replaced GL Engine with: %s", engines[GLEngine]);
			}
			
			if (engines[CLEngine]) {
				DBGLOG("Found CL Engine: %s at %lX", engines[CLEngine], static_cast<uintptr_t>(reinterpret_cast<uint8_t *>(engines[CLEngine]) - device));
				snprintf(engines[CLEngine], EngineLength, "%s %s Compute Engine", vendorName, modelName);
				DBGLOG("Replaced CL Engine with: %s", engines[CLEngine]);
			}
		}
	} else {
		ERRLOG("Failed to detect the model name");
	}
	
	return ret;
}

t_InitialiseDeviceInfo wrapInitialiseDeviceInfo[MaxCards] {
	initialiseDeviceInfo<0>,
	initialiseDeviceInfo<1>,
	initialiseDeviceInfo<2>,
	initialiseDeviceInfo<3>,
	initialiseDeviceInfo<4>,
	initialiseDeviceInfo<5>,
	initialiseDeviceInfo<6>,
	initialiseDeviceInfo<7>
};

template <size_t Index>
static uint32_t getNameInfo(uint8_t *device, uint8_t *info) {
	if (!orgGetNameInfo[Index]) {
		ERRLOG("Failed to determine the original nameInfo for %zu", Index);
		return 0;
	} else {
		DBGLOG("NameInfo %zu is ready for service", Index);
	}
	
	auto ret = orgGetNameInfo[Index](device, info);

	if (device && info) {
		char *engines[NumEngines] {};
		const char *vendorName = nullptr;
		lookupOffsets(device, engines, vendorName);
		
		if (engines[CLEngine]) {
			DBGLOG("Found CL Engine in getNameInfo: %s at %lX", engines[CLEngine], static_cast<uintptr_t>(reinterpret_cast<uint8_t *>(engines[CLEngine]) - device));
		
			if (!strncmp(engines[CLEngine], "AMD ", strlen("AMD ")) ||
				!strncmp(engines[CLEngine], "ATI ", strlen("ATI "))) {
				
				DBGLOG("Older ATI/AMD are special, checking...");
				
				bool ok {false};
				auto infoptr = reinterpret_cast<const char **>(info);
				
				for (size_t i = 0; i < 64; i++) {
					DBGLOG("Checking %zu: %p with %p", i, infoptr[i], engines[CLEngine]);
					
					if (infoptr[i] == engines[CLEngine]) {
						DBGLOG("This ATI/AMD is not bugged!");
						ok = true;
						break;
					}
				}
				
				if (!ok) {
					DBGLOG("This ATI/AMD is recomputing its names, fixing...");
					
					Dl_info tinfo {};
					if (dladdr(reinterpret_cast<const void *>(orgGetNameInfo[Index]), &tinfo)) {
						DBGLOG("Got dlinfo for original getNameInfo %p", tinfo.dli_fbase);
						
						Dl_info info {};
						for (size_t i = 0; i < 64; i++) {
							DBGLOG("Checking %zu: %p with %p", i, infoptr[i], engines[CLEngine]);
							
							if (infoptr[i] && dladdr(infoptr[i], &info)) {
								DBGLOG("Got info at %zu for %p", i, info.dli_fbase);
								
								if (info.dli_fbase == tinfo.dli_fbase) {
									DBGLOG("Testing text for %zu is %s", i, infoptr[i]);
									
									if (infoptr[i][0] == 'A' && infoptr[i][1] == 'T' && infoptr[i][2] == 'I' && infoptr[i][3] == ' ') {
										DBGLOG("Found the wrong engine %s fixing...", infoptr[i]);
										infoptr[i] = engines[CLEngine];
										break;
									}
								}
							}
						}
					} else {
						ERRLOG("Failed to get dlinfo for original getNameInfo");
					}
				}
			}
		}
	} else {
		ERRLOG("Invalid arguments to nameInfo (%p %p)", device, info);
	}
	
	return ret;
}

t_GetNameInfo wrapGetNameInfo[MaxCards] {
	getNameInfo<0>,
	getNameInfo<1>,
	getNameInfo<2>,
	getNameInfo<3>,
	getNameInfo<4>,
	getNameInfo<5>,
	getNameInfo<6>,
	getNameInfo<7>
};


void (*orgGpumInitializeIOData)(void *a1, void *a2, uint32_t a3, void *a4, void *a5, uint32_t a6, t_InitialiseDeviceInfo *functable, void *a8);
void wrapGpumInitializeIOData(void *a1, void *a2, uint32_t a3, void *a4, void *a5, uint32_t a6, t_InitialiseDeviceInfo *functable, void *a8) {
	if (functable) {
		auto &orgFunc = functable[2];

		DBGLOG("Processing new init func");
		
		for (size_t i = 0; i < MaxCards; i++) {
			if (wrapInitialiseDeviceInfo[i] == orgFunc) {
				DBGLOG("Found already replaced data init func at %zu", i);
				break;
			} else if (!orgInitialiseDeviceInfo[i]) {
				orgInitialiseDeviceInfo[i] = orgFunc;
				orgFunc = wrapInitialiseDeviceInfo[i];
				DBGLOG("Replaced data init func at %zu", i);
				break;
			}
		}
	}
	
	orgGpumInitializeIOData(a1, a2, a3, a4, a5, a6, functable, a8);
}

uint32_t (*orgGpumCreateDevice)(void *a1, void *a2, void *a3, void *a4, t_GetNameInfo orgFunc);
uint32_t wrapGpumCreateDevice(void *a1, void *a2, void *a3, void *a4, t_GetNameInfo orgFunc) {
	if (orgFunc) {
		DBGLOG("Processing gpumCreateDevice func");
		
		for (size_t i = 0; i < MaxCards; i++) {
			if (orgGetNameInfo[i] == orgFunc) {
				DBGLOG("Found already replaced create init func at %zu", i);
				break;
			} else if (!orgInitialiseDeviceInfo[i]) {
				orgGetNameInfo[i] = orgFunc;
				orgFunc = wrapGetNameInfo[i];
				DBGLOG("Replaced create init func at %zu", i);
				break;
			}
		}
	}
	
	return orgGpumCreateDevice(a1, a2, a3, a4, orgFunc);
}

id productName(id that, SEL sel) {
	Class cls = [that class];
	Ivar var = class_getInstanceVariable(cls, "_deviceRef");
	if (var) {
		ptrdiff_t offset = ivar_getOffset(var);
		void *accelDevice = *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(that) + offset);
		DBGLOG("Got NVMTLDevice deviceRef %p at %td with %p", var, offset, accelDevice);
		
		char modelName[EngineLength] {};
		if (getGPUModel(accelDevice, MACH_PORT_NULL, modelName) && strncmp(modelName, "Intel", strlen("Intel")) != 0) {
			int num {0};
			auto product = getGPUModelSubsr(modelName, ModelSubstr::Product, num);
			DBGLOG("Found GPU name %s its product is %.*s", modelName, num, product);
			return [[[NSString alloc] initWithFormat:@"%.*s", num, product] autorelease];
			
		}
	}
	
	static SEL orgSel = sel_registerName("orgProductName");
	return objc_msgSend(that, orgSel);
}

id familyName(id that, SEL sel) {
	Class cls = [that class];
	Ivar var = class_getInstanceVariable(cls, "_deviceRef");
	if (var) {
		ptrdiff_t offset = ivar_getOffset(var);
		void *accelDevice = *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(that) + offset);
		DBGLOG("Got NVMTLDevice deviceRef %p at %td with %p", var, offset, accelDevice);
		
		char modelName[EngineLength] {};
		if (getGPUModel(accelDevice, MACH_PORT_NULL, modelName) && strncmp(modelName, "Intel", strlen("Intel")) != 0) {
			int num {0};
			auto family = getGPUModelSubsr(modelName, ModelSubstr::Family, num);
			DBGLOG("Found GPU name %s its family is %.*s", modelName, num, family);
			return [[[NSString alloc] initWithFormat:@"%.*s", num, family] autorelease];
		}
	}
	
	static SEL orgSel = sel_registerName("orgFamilyName");
	return objc_msgSend(that, orgSel);
}

static void onImageAdd(const mach_header *header, intptr_t slide) {
	static auto productNameSel = sel_registerName("productName");
	static auto orgProductNameSel = sel_registerName("orgProductName");
	static auto familyNameSel = sel_registerName("familyName");
	static auto orgFamilyNameSel = sel_registerName("orgFamilyName");
	
	auto proxyMethod = [](Class cls, SEL curr, SEL backup, IMP repl) {
		Method method = class_getInstanceMethod(cls, curr);
		if (method) {
			auto enc = method_getTypeEncoding(method);
			IMP imp = method_getImplementation(method);
			if (imp) {
				if (class_addMethod(cls, backup, imp, enc)) {
					if (class_replaceMethod(cls, curr, repl, enc))
						return true;
					ERRLOG("Failed to replace -[%s %s]", class_getName(cls), sel_getName(curr));
				} else {
					ERRLOG("Failed to add -[%s %s] method", class_getName(cls), sel_getName(backup));
				}
			} else {
				ERRLOG("Failed to get imp -[%s %s]", class_getName(cls), sel_getName(curr));
			}
		} else {
			ERRLOG("Failed to get method -[%s %s]", class_getName(cls), sel_getName(curr));
		}
		
		return false;
	};
	
	static struct {
		const char *name;
		Class cls;
		bool routed {false};
	} metalClasses[] {
		{"NVMTLDevice"},
		{"BronzeMtlDevice"},
		{"MTLIGAccelDevice"}
	};
	
	for (auto &metalClass : metalClasses) {
		if (metalClass.routed || !(metalClass.cls || (metalClass.cls = objc_getClass(metalClass.name))) ||
			!proxyMethod(metalClass.cls, productNameSel, orgProductNameSel, reinterpret_cast<IMP>(productName)) ||
			!proxyMethod(metalClass.cls, familyNameSel, orgFamilyNameSel, reinterpret_cast<IMP>(familyName)))
			continue;
		metalClass.routed = true;
	}
	
	static bool doneRouting {false}, doneLegacyRouting {false};
	if (!doneRouting || !doneLegacyRouting) {
		uint32_t num = _dyld_image_count();
		for (uint32_t i = 0; i < num; i++) {
			if (_dyld_get_image_header(i) == header) {
				auto name = _dyld_get_image_name(i);
				DBGLOG("Received %s", name);
				if (!doneRouting && !strcmp(name, "/System/Library/PrivateFrameworks/GPUSupport.framework/Versions/A/Libraries/libGPUSupportMercury.dylib")) {
					if (rd_route_byname("gpumInitializeIOData", name, reinterpret_cast<void *>(wrapGpumInitializeIOData),
										reinterpret_cast<void **>(&orgGpumInitializeIOData)) == KERN_SUCCESS)
						DBGLOG("Routed gpumInitializeIOData");
					else
						ERRLOG("Failed to route gpumInitializeIOData");
					
					if (rd_route_byname("gpumCreateDevice", name, reinterpret_cast<void *>(wrapGpumCreateDevice),
										reinterpret_cast<void **>(&orgGpumCreateDevice)) == KERN_SUCCESS)
						DBGLOG("Routed gpumCreateDevice");
					else
						ERRLOG("Failed to route gpumCreateDevice");
					
					doneRouting = true;
					break;
				}
			}
		}
	}
}

@interface WhateverName : NSObject @end
@implementation WhateverName
+ (void)load {
	DBGLOG("Started WhateverName");
	_dyld_register_func_for_add_image(onImageAdd);
}
@end
