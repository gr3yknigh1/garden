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

constexpr Int16U aseprite_header_magic_number = 0xA5E0;
constexpr Int16U aseprite_frame_magic_number = 0xF1FA;

#pragma pack(push, 1)
struct Aseprite_Header {
    Int32U file_size;

    //!
    //! @see `aseprite_header_magic_number`
    //!
    Int16U magic;

    //!
    //! @brief In pixels
    //!
    Int16U width;

    //!
    //! @brief In pixels
    //!
    Int16U height;

    //!
    //! @brief Bits per pixel. 32 bpp = RGBA, 16 bpp = Grayscale, 8 bpp = Indexed.
    //!
    Int16U color_depth;

    Int32U flags;

    //!
    //! @deprecated Use frame duration from frame header.
    //!
    Int16U speeed;

    //!
    //! @brief Should be zero.
    //!
    Int32U padding0[2];

    //!
    //! @brief Only for "Indexed sprites". TBD.
    //!
    Int8U palette_entry;

    //!
    //! @brief Docs says ignore it.
    //!
    Int8U padding1[3];

    //!
    //! @brief TBD.
    //!
    Int16U colors_count;

    Int8U pixel_width;
    Int8U pixel_height;

    //!
    //! @brief X position of the grid.
    //!
    Int16S x_position;

    //!
    //! @brief Y position of the grid.
    //!
    Int16S y_position;

    //!
    //! @brief Zero if no grid present.
    //!
    Int16U grid_width;

    //!
    //! @brief Zero if no grid present.
    //!
    Int16U grid_height;

    //!
    //! @brief For future as the docs says.
    //!
    Int8U padding2[84];
};

struct Aseprite_Frame_Header {
    Int32U size;

    //!
    //! @see `aseprite_frame_magic_number`
    //!
    Int16U magic;

    //!
    //! @brief If equals 0xFFFF, use `chunks_count_ex`.
    //!
    Int16U chunks_count;

    //!
    //! @brief In milliseconds.
    //!
    Int16U frame_duration;

    //!
    //! @brief For future as the docs says.
    //!
    Int8U padding0[2];

    //!
    //! @brief If equals `0x0000`, use `chunks_count`.
    //!
    Int16U chunks_count_ex;
};


struct Aseprite_Palette_Chunk_0x0004 {
    Int16U packets_count;

    struct Packet {
        Int8U palette_offset;
        Int8U colors_count;
    };

    Packet packet[1];
};

enum struct Aseprite_Chunk_Type : Int16U {
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
    Int32U size;
    Aseprite_Chunk_Type type;

    Int8U data[1];
};

struct Aseprite_File {
    Aseprite_Header header;
};
#pragma pack(pop)


bool aseprite_load_information_from_file(Aseprite_File *output, FILE *file_fd);
