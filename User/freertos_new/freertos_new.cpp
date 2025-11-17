/**
 * @file freertos_new.cpp
 * @brief Implementation of C++ new/delete operators using FreeRTOS memory management
 *
 * Note: This implementation does not throw exceptions, suitable for embedded systems.
 * All new operators return nullptr on allocation failure.
 */

#include "freertos_new.h"

#ifdef __cplusplus

#include "FreeRTOS.h"
#include <cstdlib>
#include <new>

/**
 * @brief Override global operator new to use FreeRTOS pvPortMalloc
 *
 * Note: Returns nullptr on failure (no exceptions thrown, suitable for embedded systems)
 */
void *operator new(std::size_t size)
{
    return pvPortMalloc(size);
}

/**
 * @brief Override global operator new[] to use FreeRTOS pvPortMalloc
 *
 * Note: Returns nullptr on failure (no exceptions thrown, suitable for embedded systems)
 */
void *operator new[](std::size_t size)
{
    return pvPortMalloc(size);
}

/**
 * @brief Override global operator new (nothrow version) to use FreeRTOS pvPortMalloc
 */
void *operator new(std::size_t size, const std::nothrow_t &nothrow_tag) noexcept
{
    (void)nothrow_tag; // Unused parameter
    return pvPortMalloc(size);
}

/**
 * @brief Override global operator new[] (nothrow version) to use FreeRTOS pvPortMalloc
 */
void *operator new[](std::size_t size, const std::nothrow_t &nothrow_tag) noexcept
{
    (void)nothrow_tag; // Unused parameter
    return pvPortMalloc(size);
}

/**
 * @brief Override global operator delete to use FreeRTOS vPortFree
 */
void operator delete(void *ptr) noexcept
{
    if (ptr != nullptr)
    {
        vPortFree(ptr);
    }
}

/**
 * @brief Override global operator delete[] to use FreeRTOS vPortFree
 */
void operator delete[](void *ptr) noexcept
{
    if (ptr != nullptr)
    {
        vPortFree(ptr);
    }
}

/**
 * @brief Override global operator delete (size version) to use FreeRTOS vPortFree
 */
void operator delete(void *ptr, std::size_t size) noexcept
{
    (void)size; // Unused parameter, FreeRTOS doesn't need size for deallocation
    if (ptr != nullptr)
    {
        vPortFree(ptr);
    }
}

/**
 * @brief Override global operator delete[] (size version) to use FreeRTOS vPortFree
 */
void operator delete[](void *ptr, std::size_t size) noexcept
{
    (void)size; // Unused parameter, FreeRTOS doesn't need size for deallocation
    if (ptr != nullptr)
    {
        vPortFree(ptr);
    }
}

#endif // __cplusplus
