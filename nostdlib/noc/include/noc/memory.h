#if !defined(NOC_MEMORY_H_INCLUDED)
#define NOC_MEMORY_H_INCLUDED

#include <noc/macros.h>
#include <noc/types.h>

#define KILOBYTES(X) (1024 * (X))
#define MEGABYTES(X) (1024 * 1024 * (X))
#define GIGABYTES(X) (1024 * 1024 * 1024 * (X))


#if !defined(NOC_ALIGN_TO)
    // NOTE(gr3yknigh1): Source https://stackoverflow.com/a/45213645/12456897
    // [2024/07/14]
    #define NOC_ALIGN_TO(X, ALIGNMENT) (((X) + ((ALIGNMENT)-1)) & ~((ALIGNMENT)-1))
#endif

NOC_DEFINE void *noc_allocate(SizeU size);

NOC_DEFINE void noc_free(void *p);

///
/// @brief Sets the buffer to zero.
/// @param buffer Buffer to make zero.
/// @param size Size of buffer.
///
NOC_DEFINE void noc_memory_zero(void *buffer, SizeU size);

///
/// @brief Sets buffer to specified value.
/// @param buffer Buffer to set value.
/// @param size Size of buffer.
/// @param value Value to set.
///
NOC_DEFINE void noc_memory_set(void *buffer, SizeU size, Byte value);

///
/// @brief Copyies from one buffer to another.
/// @param destination Buffer in which will be copied source buffer.
/// @param source Buffer from which will be copy values.
/// @param size Size of source buffer.
///
NOC_DEFINE void noc_memory_copy(void *destination, const void *source, SizeU size);

///
/// @breaf Finds and returns a pointer to first byte with specified value in buffer.
/// @param buffer Buffer in which it should search value.
/// @param size Size of buffer.
/// @param value Value which should be find.
///
NOC_DEFINE const void *noc_memory_find(const void *buffer, SizeU size, Byte value);

///
/// @breaf Aligns specified size to page size of platform.
/// @param size Size which should be aligned.
/// @returns Aligned to page size value.
///
NOC_DEFINE SizeU noc_align_to_page_size(SizeU size);

typedef struct NOC_Arena {
    void *data;
    SizeU capacity;
    SizeU occupied;
} NOC_Arena;

#define NOC_ARENA_HAS_SPACE_FOR(ARENAPTR, SIZE)			\
    ((ARENAPTR)->occupied + (SIZE) <= (ARENAPTR)->capacity)

NOC_NODISCARD NOC_DEFINE NOC_Arena noc_make_arena(SizeU size);
NOC_DEFINE               void      noc_destroy_arena(NOC_Arena *arena);
NOC_NODISCARD NOC_DEFINE void *    noc_arena_alloc(NOC_Arena *arena, SizeU size);


#if defined(NOC_DETECT_LANGUAGE_CXX)

namespace noxx {

inline void *
allocate_raw(SizeU size) noexcept
{
    return noc_allocate(size);
}

inline void
free(void *p)
{
    noc_free(p);
}

template<typename Ty> inline Ty *
allocate(Int64U count = 1) noexcept
{
    return static_cast<Ty *>(noxx::allocate_raw(sizeof(Ty) * count));
}

//!
//! @brief Make offset by number of bytes specified.
//!
template<typename Ty = void> inline Ty *
get_offset(Ty *pointer, SizeU offset) noexcept
{
    return reinterpret_cast<Ty *>(reinterpret_cast<Byte *>(pointer) + offset);
}

// TODO(gr3yknigh1): Rename to `memory_zero_typed`? [2025/06/10]
template <typename Ty> inline void
zero_type(Ty *p, Int64U count = 1)
{
    noc_memory_zero(reinterpret_cast<void *>(p), sizeof(*p) * count);
}

}; // namespace noxx

#endif  // NOC_DETECT_LANGUAGE_CXX

#endif // NOC_MEMORY_H_INCLUDED
