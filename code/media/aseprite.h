//!
//! FILE          code\media\aseprite.h
//!
//! AUTHORS
//!               Ilya Akkuzin <gr3yknigh1@gmail.com>
//!
//! NOTICE        (c) Copyright 2025 by Ilya Akkuzin. All rights reserved.
//!
#pragma once

#include <stdio.h>

#include "garden_runtime.h"

constexpr U16 aseprite_header_magic_number = 0xA5E0;
constexpr U16 aseprite_frame_magic_number = 0xF1FA;

#pragma pack(push, 1)
struct Aseprite_Header {
    U32 file_size;

    //!
    //! @see `aseprite_header_magic_number`
    //!
    U16 magic;

    //!
    //! @brief In pixels
    //!
    U16 width;

    //!
    //! @brief In pixels
    //!
    U16 height;

    //!
    //! @brief Bits per pixel. 32 bpp = RGBA, 16 bpp = Grayscale, 8 bpp = Indexed.
    //!
    U16 color_depth;

    U32 flags;

    //!
    //! @deprecated Use frame duration from frame header.
    //!
    U16 speeed;

    //!
    //! @brief Should be zero.
    //!
    U32 padding0[2];

    //!
    //! @brief Only for "Indexed sprites". TBD.
    //!
    U8 palette_entry;

    //!
    //! @brief Docs says ignore it.
    //!
    U8 padding1[3];

    //!
    //! @brief TBD.
    //!
    U16 colors_count;

    U8 pixel_width;
    U8 pixel_height;

    //!
    //! @brief X position of the grid.
    //!
    S16 x_position;

    //!
    //! @brief Y position of the grid.
    //!
    S16 y_position;

    //!
    //! @brief Zero if no grid present.
    //!
    U16 grid_width;

    //!
    //! @brief Zero if no grid present.
    //!
    U16 grid_height;

    //!
    //! @brief For future as the docs says.
    //!
    U8 padding2[84];
};

struct Aseprite_Frame_Header {
    U32 size;

    //!
    //! @see `aseprite_frame_magic_number`
    //!
    U16 magic;

    //!
    //! @brief If equals 0xFFFF, use `chunks_count_ex`.
    //!
    U16 chunks_count;

    //!
    //! @brief In milliseconds.
    //!
    U16 frame_duration;

    //!
    //! @brief For future as the docs says.
    //!
    U8 padding0[2];

    //!
    //! @brief If equals `0x0000`, use `chunks_count`.
    //!
    U16 chunks_count_ex;
};


struct Aseprite_Palette_Chunk_0x0004 {
    U16 packets_count;

    struct Packet {
        U8 palette_offset;
        U8 colors_count;
    };

    Packet packet[1];
};

enum struct Aseprite_Chunk_Type : U16 {
    //!
    //! @deprecated TDB.
    //!
    Palette_Chunk_0x0004 = 0x0004,

    //!
    //! @deprecated TDB.
    //!
    Palette_Chunk_0x0011 = 0x0011,

    Layer_Chunk = 0x2004,

    Cel_Chunk = 0x2005,
};


struct Aseprite_Chunk {
    U32 size;
    Aseprite_Chunk_Type type;

    U8 data[1];
};

struct Aseprite_File {
    Aseprite_Header header;
};
#pragma pack(pop)


bool aseprite_load_information_from_file(Aseprite_File *output, FILE *file_fd);
