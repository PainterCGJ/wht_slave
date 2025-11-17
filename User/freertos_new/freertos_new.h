/**
 * @file freertos_new.h
 * @brief C++ new/delete operators overridden to use FreeRTOS memory management
 *
 * This file provides global operator new/delete overloads that use FreeRTOS
 * heap management functions (pvPortMalloc/vPortFree) instead of the standard
 * C++ library memory management.
 *
 * Usage: Simply include this header file in your main source file or in a
 * common header that is included early in the compilation.
 */

#ifndef FREERTOS_NEW_H
#define FREERTOS_NEW_H

#ifdef __cplusplus

#include <cstddef>
#include <new>

/**
 * @brief Override global operator new to use FreeRTOS pvPortMalloc
 *
 * @param size Size of memory to allocate in bytes
 * @return Pointer to allocated memory, or nullptr if allocation fails
 * @note Does not throw exceptions (suitable for embedded systems)
 */
void *operator new(std::size_t size);

/**
 * @brief Override global operator new[] to use FreeRTOS pvPortMalloc
 *
 * @param size Size of memory to allocate in bytes
 * @return Pointer to allocated memory, or nullptr if allocation fails
 * @note Does not throw exceptions (suitable for embedded systems)
 */
void *operator new[](std::size_t size);

/**
 * @brief Override global operator new (nothrow version) to use FreeRTOS pvPortMalloc
 *
 * @param size Size of memory to allocate in bytes
 * @param nothrow_tag std::nothrow tag
 * @return Pointer to allocated memory, or nullptr if allocation fails
 */
void *operator new(std::size_t size, const std::nothrow_t &nothrow_tag) noexcept;

/**
 * @brief Override global operator new[] (nothrow version) to use FreeRTOS pvPortMalloc
 *
 * @param size Size of memory to allocate in bytes
 * @param nothrow_tag std::nothrow tag
 * @return Pointer to allocated memory, or nullptr if allocation fails
 */
void *operator new[](std::size_t size, const std::nothrow_t &nothrow_tag) noexcept;

/**
 * @brief Override global operator delete to use FreeRTOS vPortFree
 *
 * @param ptr Pointer to memory to deallocate
 */
void operator delete(void *ptr) noexcept;

/**
 * @brief Override global operator delete[] to use FreeRTOS vPortFree
 *
 * @param ptr Pointer to memory to deallocate
 */
void operator delete[](void *ptr) noexcept;

/**
 * @brief Override global operator delete (size version) to use FreeRTOS vPortFree
 *
 * @param ptr Pointer to memory to deallocate
 * @param size Size of memory block (unused, kept for compatibility)
 */
void operator delete(void *ptr, std::size_t size) noexcept;

/**
 * @brief Override global operator delete[] (size version) to use FreeRTOS vPortFree
 *
 * @param ptr Pointer to memory to deallocate
 * @param size Size of memory block (unused, kept for compatibility)
 */
void operator delete[](void *ptr, std::size_t size) noexcept;

#endif // __cplusplus

#endif // FREERTOS_NEW_H
