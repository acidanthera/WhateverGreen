#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <IOKit/IOKitLib.h>

#include "rd_route.h"

uint8_t customMac[] = {0x00, 0x25, 0x00, 0x00, 0x00, 0x00};

CFTypeRef (*org_IORegistryEntryCreateCFProperty)(io_registry_entry_t entry, CFStringRef key, CFAllocatorRef allocator, IOOptionBits options);
CFTypeRef my_IORegistryEntryCreateCFProperty(io_registry_entry_t entry, CFStringRef key, CFAllocatorRef allocator, IOOptionBits options) {
	CFTypeRef obj = org_IORegistryEntryCreateCFProperty(entry, key, allocator, options);
	if (kCFCompareEqualTo == CFStringCompare(key, CFSTR("IOMACAddress"), 0)) {
		uint8_t *mac = (uint8_t *)CFDataGetBytePtr((CFDataRef)obj);
		printf("Routing mac address %0.2X:%0.2X:%0.2X:%0.2X:%0.2X:%0.2X to new one\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		memcpy(mac, customMac, 6);
	}

	return obj;
}

__attribute__((constructor)) void start() {
	char *mac = getenv ("MAC");

	if (!mac || sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &customMac[0], &customMac[1], &customMac[2], &customMac[3], &customMac[4], &customMac[5]) != 6) {
		printf("Custom MAC address in MAC variable is either not provided or invalid, will randomise it\n");
		srand(time(NULL));
		customMac[3] = rand() % 256;
		customMac[4] = rand() % 256;
		customMac[5] = rand() % 256;
	}

	printf("Hi, I will hook your IOMACAddress to %0.2X:%0.2X:%0.2X:%0.2X:%0.2X:%0.2X\n", customMac[0], customMac[1], customMac[2], customMac[3], customMac[4], customMac[5]);
	if (rd_route((void *)IORegistryEntryCreateCFProperty, (void *)my_IORegistryEntryCreateCFProperty, (void **)&org_IORegistryEntryCreateCFProperty) != KERN_SUCCESS) {
		fprintf(stderr, "Failed to route IORegistryEntryCreateCFProperty\n");
	}
}