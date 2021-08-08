//
//  kern_igfx_util.hpp
//  WhateverGreen
//
//  Created by FireWolf on 8/7/21.
//  Copyright © 2021 vit9696. All rights reserved.
//

#ifndef kern_igfx_util_hpp
#define kern_igfx_util_hpp

#include <libkern/c++/OSObject.h>
#include <IOKit/IOLocks.h>
#include <kern/clock.h>
#include "kern_util.hpp"

namespace Value {
	template <typename T>
	struct Value {
		T value;

		explicit Value(T value) : value(value) {}

		template<typename... Ts>
		bool isOneOf(Ts... args) {
			return ((value == args) || ...);
		}

		template <typename... Ts>
		bool isNotOneOf(Ts... args) {
			return ((value != args) && ...);
		}
	};

	template <typename T>
	static Value<T> of(T value) {
		return Value<T>(value);
	}
}

static inline uint64_t MachAbsoluteTime2Nanoseconds(uint64_t abstime) {
	uint64_t ns = 0;
	absolutetime_to_nanoseconds(abstime, &ns);
	return ns;
}

/**
 *  Represents a circular buffer protected by a recursive mutex lock
 */
template <typename T>
struct CircularBuffer {
private:
	/**
	 *  The internal storage
	 */
	T *storage;
	
	/**
	 *  The buffer capacity
	 */
	IOItemCount size;
	
	/**
	 *  The current index for the next read operation
	 */
	IOItemCount indexr;
	
	/**
	 *  The current index for the next write operation
	 */
	IOItemCount indexw;
	
	/**
	 *  The current number of elements in the buffer
	 */
	IOItemCount count;
	
	/**
	 *  The recursive mutex lock that protects the buffer
	 */
	IORecursiveLock *lock;
	
public:
	/**
	 *  Initialize a circular buffer
	 *
	 *  @param buffer A non-null storage buffer
	 *  @param capacity The total number of elements
	 *  @return `true` on success, `false` otherwise.
	 *  @warning The caller is responsbile for managing the lifecycle of the given storage buffer.
	 */
	bool init(T *buffer, IOItemCount capacity) {
		storage = buffer;
		size = capacity;
		indexr = 0;
		indexw = 0;
		count = 0;
		lock = IORecursiveLockAlloc();
		return lock != nullptr;
	}
	
	/**
	 *  Initialize a circular buffer
	 *
	 *  @param storage A storage buffer
	 *  @return `true` on success, `false` otherwise.
	 *  @warning The caller is responsbile for managing the lifecycle of the given storage buffer.
	 */
	template <size_t N>
	bool init(T (&storage)[N]) {
		return init(storage, N);
	}
	
	/**
	 *  Deinitialize the circular buffer
	 */
	void deinit() {
		IORecursiveLockFree(lock);
	}
	
	/**
	 *  Create a circular buffer with the given capacity
	 *
	 *  @param size The total number of elements
	 *  @return A non-null instance on success, `nullptr` if no memory.
	 *  @warning The caller must invoke `CircularBuffer::destory()` to release the returned buffer.
	 */
	static CircularBuffer<T> *withCapacity(IOItemCount size) {
		auto storage = IONewZero(T, size);
		if (storage == nullptr)
			return nullptr;
		
		auto instance = new CircularBuffer<T>();
		if (instance == nullptr)
			goto error1;
		
		if (!instance->init(storage, size))
			goto error2;
		
		return instance;
		
	error2:
		delete instance;
		
	error1:
		IODelete(storage, T, size);
		return nullptr;
	}
	
	/**
	 *  Destory the given circular buffer
	 *
	 *  @param buffer A non-null circular buffer returned by `CircularBuffer::withCapacity()`.
	 */
	static inline void destory(CircularBuffer<T> *buffer NONNULL) {
		IOSafeDeleteNULL(buffer->storage, T, buffer->size);
		buffer->deinit();
		delete buffer;
	}
	
	/**
	 *  Destory the given circular buffer if it is non-null and set it to nullptr
	 *
	 *  @param buffer A nullable circular buffer returned by `CircularBuffer::withCapacity()`.
	 *  @note This function mimics the macro `OSSafeReleaseNULL()`.
	 */
	static inline void safeDestory(CircularBuffer<T> *&buffer) {
		if (buffer != nullptr) {
			destory(buffer);
			buffer = nullptr;
		}
	}
	
	/**
	 *  Check whether the circular buffer is empty
	 *
	 *  @return `true` if the buffer is empty, `false` otherwise.
	 */
	inline bool isEmpty() {
		IORecursiveLockLock(lock);
		bool retVal = (count == 0) && (indexr == indexw);
		IORecursiveLockUnlock(lock);
		return retVal;
	}
	
	/**
	 *  Check whether the circular buffer is full
	 *
	 *  @return `true` if the buffer is full, `false` otherwise.
	 */
	inline bool isFull() {
		IORecursiveLockLock(lock);
		bool retVal = (count == size) && (indexr == indexw);
		IORecursiveLockUnlock(lock);
		return retVal;
	}
	
	/**
	 *  Get the number of elements in the circular buffer
	 *
	 *  @return The current number of elements in the buffer.
	 */
	inline IOItemCount getCount() {
		IORecursiveLockLock(lock);
		IOItemCount retVal = count;
		IORecursiveLockUnlock(lock);
		return retVal;
	}
	
	/**
	 *  Write the given element to the circular buffer
	 *
	 *  @param element The element to write
	 *  @return `true` on success, `false` if the buffer is full.
	 */
	inline bool write(const T &element) {
		if (isFull())
			return false;
		
		IORecursiveLockLock(lock);
		storage[indexw] = element;
		indexw += 1;
		indexw %= size;
		count += 1;
		IORecursiveLockUnlock(lock);
		return true;
	}
	
	/**
	 *  Read the next element from the circular buffer
	 *
	 *  @param element The element read from the buffer
	 *  @return `true` on success, `false` if the buffer is empty.
	 */
	inline bool read(T& element) {
		if (isEmpty())
			return false;
		
		IORecursiveLockLock(lock);
		element = storage[indexr];
		indexr += 1;
		indexr %= size;
		count -= 1;
		IORecursiveLockUnlock(lock);
		return true;
	}
};

/**
 *  Wrap an object that is not an instance of OSObject
 */
class OSObjectWrapper: public OSObject {
	/**
	 *  Constructors & Destructors
	 */
	OSDeclareDefaultStructors(OSObjectWrapper);
	
	using super = OSObject;
	
	/**
	 *  Wrapped object
	 */
	void *object;
	
public:
	/**
	 *  Initialize the wrapper with the given object
	 *
	 *  @param object The wrapped object that is not an `OSObject`
	 *  @return `true` on success, `false` otherwise.
	 */
	bool init(void *object);
	
	/**
	 *  Reinterpret the wrapped object as the given type
	 *
	 *  @return The wrapped object of the given type.
	 */
	template <typename T>
	T *reinterpretWrappedObjectAs() {
		return reinterpret_cast<T*>(object);
	}
	
	/**
	 *  Create a wrapper for the given object that is not an `OSObject`
	 *
	 *  @param object A non-null object
	 *  @return A non-null wrapper on success, `nullptr` otherwise.
	 *  @warning The caller is responsbile for managing the lifecycle of the given object.
	 */
	static OSObjectWrapper *of(void *object);
};

#endif /* kern_igfx_util_hpp */
