#pragma once

#include "include/lce/processor.hpp"

#include "common/error_status.hpp"


class NBTBase;

namespace editor::chunk {


    class ChunkData {
    public:
        // old version
        u8_vec oldBlocks;
        u8_vec blockData;

        // new version
        u16_vec newBlocks;
        u16_vec submerged;
        bool hasSubmerged = false;

        // all versions
        u8_vec blockLight;          //
        u8_vec skyLight;            //
        u8_vec heightMap;           //
        u8_vec biomes;              //
        NBTBase* NBTData = nullptr; //
        i16 terrainPopulated = 0;   //
        i64 lastUpdate = 0;         //
        i64 inhabitedTime = 0;      //

        /// Used to skip the lights in the chunk
        size_t DataGroupCount = 0;

        i32 chunkX = 0;
        i32 chunkZ = 0;

        i32 lastVersion = 0;
        bool validChunk = false;

        ~ChunkData();

        MU ND std::string getCoords() const;

        void defaultNBT();

        // MODIFIERS

        MU void convertNBTToAquatic();
        MU void convertOldToAquatic();
        MU void convert114ToAquatic();


        MU void placeBlock(int xIn, int yIn, int zIn, u16 block, u16 data, bool waterlogged, bool submerged = false);
        MU void placeBlock(int xIn, int yIn, int zIn, u16 block, bool submerged = false);


        /// Returns (blockID << 4 | dataTag).
        u16 getBlock(int xIn, int yIn, int zIn);


    };
}
