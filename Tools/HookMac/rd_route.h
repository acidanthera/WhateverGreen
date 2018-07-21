// rd_route.h
// Copyright (c) 2014-2015 Dmitry Rodionov
//
// This software may be modified and distributed under the terms
// of the MIT license.  See the LICENSE file for details.

#ifndef RD_ROUTE
	#define RD_ROUTE

#ifdef __cplusplus
	extern "C" {
#endif

/**
 * Override `function` to jump directly into `replacement` location. Caller can later
 * access an original function's implementation via `original_ptr` (if passed).
 * Note that `original_ptr` won't be equal to `function` due to remapping the latter
 * into a different memory space.
 *
 * @param  function    pointer to a function to override;
 * @param  replacement pointer to new implementation;
 * @param  original    will be set to an address of the original implementation's copy;
 *
 * @return             KERN_SUCCESS if succeeded, or other value if failed
 */
int rd_route(void *function, void *replacement, void **original);

/**
 * The same as rd_route(), but the target function is defined with its name, not its symbol pointer.
 * If the `image_name` provided, rd_route_byname() looks for the function within it.
 * Otherwise it iterates all images loaded into the current process' address space and, if there is more
 * than one image containing a function with the specifed name, it will choose the first one.
 *
 * @param  function_name name of a function to override;
 * @param  image_name    name of a mach-o image containing the function. It may be NULL, a full file path or
 *                       just a file name (e.g. "CoreFoundation");
 * @param  replacement   pointer to new implementation of the function;
 * @param  original      will be set to an address of the original implementation's copy;
 *
 * @return               see rd_route() for the list of possible return values
 */
int rd_route_byname(const char *function_name, const char *image_name, void *replacement, void **original);

/**
 * Copy `function` implementation into another (first available) memory region.
 * @param  function  pointer to a function to override;
 * @param  duplicate will be set to an address of the function's implementation copy
 *
 * @return KERN_SUCCESS if succeeded, or other value if failed
 */
int rd_duplicate_function(void *function, void **duplicate);

#ifdef __cplusplus
	}
#endif

#endif /* RD_ROUTE */
