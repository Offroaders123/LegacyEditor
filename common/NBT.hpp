#pragma once

#include <cstring>
#include <unordered_map>
#include <vector>
#include <ranges>

#include "common/dataManager.hpp"
#include "include/lce/processor.hpp"


class DataManager;

enum NBTType : u8 {
    NBT_NONE = 0,
    NBT_INT8 = 1,
    NBT_INT16 = 2,
    NBT_INT32 = 3,
    NBT_INT64 = 4,
    NBT_FLOAT = 5,
    NBT_DOUBLE = 6,
    TAG_BYTE_ARRAY = 7,
    TAG_STRING = 8,
    TAG_LIST = 9,
    TAG_COMPOUND = 10,
    TAG_INT_ARRAY = 11,
    TAG_LONG_ARRAY = 12,
    TAG_PRIMITIVE = 99
};


template<class classType>
class NBTTagTypeArray {
public:
    classType* array = nullptr;
    int size = 0;

    NBTTagTypeArray(classType* dataIn, const size_t sizeIn) : array(dataIn), size(sizeIn) {}
    NBTTagTypeArray() = default;

    MU ND classType* getArray() const { return array; }
};


using NBTTagByteArray = NBTTagTypeArray<u8>;
using NBTTagIntArray = NBTTagTypeArray<i32>;
using NBTTagLongArray = NBTTagTypeArray<i64>;


class NBTTagString;
class NBTTagList;
class NBTTagCompound;

template<typename T>
concept NBT_TAG_TYPE = std::is_same_v<T, NBTTagByteArray> ||
                       std::is_same_v<T, NBTTagString> ||
                       std::is_same_v<T, NBTTagList> ||
                       std::is_same_v<T, NBTTagCompound> ||
                       std::is_same_v<T, NBTTagIntArray> ||
                       std::is_same_v<T, NBTTagLongArray>;


class NBTBase {
public:
    void* data;
    NBTType type;

    NBTBase(void* dataIn, const NBTType typeIn) : data(dataIn), type(typeIn) {}

    NBTBase() : NBTBase(nullptr, NBT_NONE) {}

    NBTBase(void* dataIn, c_int dataSizeIn, const NBTType typeIn) : NBTBase(dataIn, typeIn) {
        std::memcpy(data, dataIn, dataSizeIn);
    }

    void write(DataManager& output) const;

    void read(DataManager& input);

    ND std::string toString() const;

    ND NBTBase copy() const;

    void NbtFree() const;

    static bool equals(NBTBase check);

    ND NBTType getId() const { return type; }

    template<class classType>
    classType toPrim() {
        switch (type) {
            case NBT_INT8:
                return (classType) *(u8*) data;
            case NBT_INT16:
                return (classType) *(i16*) data;
            case NBT_INT32:
                return (classType) *(i32*) data;
            case NBT_INT64:
                return (classType) *(i64*) data;
            case NBT_FLOAT:
                return (classType) *(float*) data;
            case NBT_DOUBLE:
                return (classType) *(double*) data;
            default:
                return 0;
        }
    }

    template<NBT_TAG_TYPE classType>
    classType* toType() const {
        return static_cast<classType*>(this->data);
    }

    template<NBT_TAG_TYPE classType>
    static classType* toType(const NBTBase* nbtPtr) {
        return static_cast<classType*>(nbtPtr->data);
    }

    template<NBT_TAG_TYPE classType>
    static classType* toType(const NBTBase& nbtPtr) {
        return static_cast<classType*>(nbtPtr.data);
    }
};



class NBTTagString {
public:
    char* data;
    i64 size;
    NBTTagString() : data(nullptr), size(0) {}

    explicit NBTTagString(const std::string& dataIn) {
        size = static_cast<int>(dataIn.size());
        data = static_cast<char*>(malloc(size));
        std::memcpy(data, dataIn.c_str(), size);
    }

    ND bool hasNoTags() const { return size != 0; }

    ND std::string getString() const { return std::string(data, size); }

    ND std::string toStringNBT() const {
        std::string stringBuilder = "\"";
        for (const std::string currentString = getString(); const char ch : currentString) {
            if (ch == '\\' || ch == '"') { stringBuilder.append("\\"); }

            stringBuilder.push_back(ch);
        }
        return stringBuilder.append("\"");
    }
};


class NBTTagList;

class NBTTagCompound {
    typedef const std::string& STR;

public:
    std::unordered_map<std::string, NBTBase> tagMap;

    static void writeEntry(STR name, NBTBase data, DataManager& output);
    int getSize() const;

    // set tags
    void setTag(STR key, NBTBase value);
    void setByte(STR key, u8 value);
    void setShort(STR key, i16 value);
    void setInteger(STR key, i32 value);
    void setLong(STR key, i64 value);
    void setFloat(STR key, float value);
    void setDouble(STR key, double value);
    void setString(STR key, STR value);
    void setByteArray(STR key, c_u8* value, int size);
    void setIntArray(STR key, c_int* value, int size);
    void setLongArray(STR key, const i64* value, int size);
    void setCompoundTag(STR key, NBTTagCompound* compoundTag);
    void setListTag(STR key, NBTTagList* listTag);
    void setBool(STR key, u8 value);

    bool hasUniqueId(STR key);
    NBTBase getTag(STR key);
    NBTType getTagId(STR key);

    bool hasKey(STR key) const;
    bool hasKey(STR key, int type);
    bool hasKey(STR key, NBTType type);
    std::vector<std::string> getKeySet();
    template<typename classType>
    classType getPrimitive(STR key) {
        if (hasKey(key, TAG_PRIMITIVE)) {
            return tagMap.at(key).toPrim<classType>();
        }
        return static_cast<classType>(0);
    }
    std::string getString(STR key);
    NBTTagByteArray* getByteArray(STR key);
    NBTTagIntArray* getIntArray(STR key);
    NBTTagLongArray* getLongArray(STR key);
    NBTTagCompound* getCompoundTag(STR key);
    NBTTagList* getListTag(STR key);
    bool getBool(STR key);
    void removeTag(STR key);
    bool hasNoTags() const;
    void merge(NBTTagCompound* other);

    /// it will free everything inside the tag map
    /// so no need to worry about memory management
    void deleteAll();
};


class NBTTagList {
public:
    std::vector<NBTBase> tagList;
    NBTType tagType;

    NBTTagList() : tagType(NBT_NONE) {}

    MU void appendTag(NBTBase nbt);
    void set(const uint32_t index, const NBTBase nbt);
    void insert(const uint32_t index, const NBTBase nbt);
    MU void removeTag(const uint32_t index);
    void deleteAll();
    MU ND bool hasNoTags() const;

    template<typename classType>
    MU classType getPrimitiveAt(c_int index) {
        if (tagType < 7 && tagType > 0) {
            if (index >= 0 && index < static_cast<int>(tagList.size())) {
                NBTBase nbtBase = tagList.at(index);
                return nbtBase.toPrim<classType>();
            }
        }

        return static_cast<classType>(0);
    }

    MU ND NBTTagByteArray* getByteArrayAt(const uint32_t index) const;
    MU ND std::string getStringTagAt(const uint32_t index) const;
    MU ND NBTTagList* getListTagAt(const uint32_t index) const;
    MU ND NBTTagCompound* getCompoundTagAt(const uint32_t index) const;
    MU ND NBTTagIntArray* getIntArrayAt(const uint32_t index) const;
    MU ND NBTTagLongArray* getLongArrayAt(const uint32_t index) const;
    MU ND NBTBase get(const uint32_t index) const;
    MU ND int tagCount() const;
    MU ND NBTType getTagType() const;
};

/*
class NBTUtil {
public:
    static UUIDJava getUUIDFromTag(NBTTagCompound* tag);
    static NBTTagCompound* createUUIDTag(UUIDJava uuid);
    static BlockPos getPosFromTag(NBTTagCompound* tag);
    static NBTTagCompound* createPosTag(BlockPos pos);
};
 */


class NBT {
public:
    MU static bool isCompoundTag(const NBTType type) { return type == TAG_COMPOUND; }
    static void writeTag(const NBTBase* tag, DataManager& output);
    static NBTBase* readTag(DataManager& input);
    static NBTBase* readNBT(NBTType tagID, const std::string& key, DataManager& input);
};


MU static NBTBase createNBT_INT8(c_i8 dataIn) {
    NBTBase nbtBase;
    nbtBase.data = malloc(sizeof(dataIn));
    std::memcpy(nbtBase.data, &dataIn, sizeof(dataIn));
    nbtBase.type = NBT_INT8;
    return nbtBase;
}


MU static NBTBase createNBT_INT16(c_i16 dataIn) {
    NBTBase nbtBase;
    nbtBase.data = malloc(sizeof(dataIn));
    std::memcpy(nbtBase.data, &dataIn, sizeof(dataIn));
    nbtBase.type = NBT_INT16;
    return nbtBase;
}


MU static NBTBase createNBT_INT32(c_i32 dataIn) {
    NBTBase nbtBase;
    nbtBase.data = malloc(sizeof(dataIn));
    std::memcpy(nbtBase.data, &dataIn, sizeof(dataIn));
    nbtBase.type = NBT_INT32;
    return nbtBase;
}


MU static NBTBase createNBT_INT64(const i64 dataIn) {
    NBTBase nbtBase;
    nbtBase.data = malloc(sizeof(dataIn));
    std::memcpy(nbtBase.data, &dataIn, sizeof(dataIn));
    nbtBase.type = NBT_INT64;
    return nbtBase;
}


MU static NBTBase createNBT_FLOAT(const float dataIn) {
    NBTBase nbtBase;
    nbtBase.data = malloc(sizeof(dataIn));
    std::memcpy(nbtBase.data, &dataIn, sizeof(dataIn));
    nbtBase.type = NBT_FLOAT;
    return nbtBase;
}


MU static NBTBase createNBT_DOUBLE(const double dataIn) {
    NBTBase nbtBase;
    nbtBase.data = malloc(sizeof(dataIn));
    std::memcpy(nbtBase.data, &dataIn, sizeof(dataIn));
    nbtBase.type = NBT_DOUBLE;
    return nbtBase;
}


MU static NBTBase convertType(NBTBase baseData, NBTType toType) {
    switch (toType) {
        case NBT_INT8: {
            auto valueB = baseData.toPrim<u8>();
            return {&valueB, 1, NBT_INT8};
        }
        case NBT_INT16: {
            auto valueS = baseData.toPrim<i16>();
            return {&valueS, 2, NBT_INT16};
        }
        case NBT_INT32: {
            int value = baseData.toPrim<i32>();
            return {&value, 4, NBT_INT32};
        }
        case NBT_INT64: {
            auto valueL = baseData.toPrim<i64>();
            return {&valueL, 8, NBT_INT64};
        }
        case NBT_FLOAT: {
            auto valueF = baseData.toPrim<float>();
            return {&valueF, 4, NBT_FLOAT};
        }
        case NBT_DOUBLE: {
            auto valueD = baseData.toPrim<double>();
            return {&valueD, 8, NBT_DOUBLE};
        }
        default:
            return baseData.copy();
    }
}


inline NBTBase* createNewByType(NBTType type) {
    switch (type) {
        case NBT_NONE:
        case NBT_INT8:
        case NBT_INT16:
        case NBT_INT32:
        case NBT_INT64:
        case NBT_FLOAT:
        case NBT_DOUBLE:
        default:
            return new NBTBase(nullptr, type);
        case TAG_BYTE_ARRAY:
            return new NBTBase(new NBTTagByteArray(), type);
        case TAG_STRING:
            return new NBTBase(new NBTTagString(), type);
        case TAG_LIST:
            return new NBTBase(new NBTTagList(), type);
        case TAG_COMPOUND:
            return new NBTBase(new NBTTagCompound(), type);
        case TAG_INT_ARRAY:
            return new NBTBase(new NBTTagIntArray(), type);
        case TAG_LONG_ARRAY:
            return new NBTBase(new NBTTagLongArray(), type);
    }
}


MU static void compareNBT(const NBTBase* first, const NBTBase* second) {
    auto* firstNBT = NBTBase::toType<NBTTagCompound>(first)->getCompoundTag("Data");
    auto* secondNBT = NBTBase::toType<NBTTagCompound>(second)->getCompoundTag("Data");

    // Iterate over the keys of firstNBT->tagMap
    for (const auto& entry : firstNBT->tagMap) {
        const auto& key = entry.first;
        if (!secondNBT->hasKey(key)) {
            printf("second does not contain tag '%s'\n", key.c_str());
        }
    }

    // Iterate over the keys of secondNBT->tagMap
    for (const auto& entry : secondNBT->tagMap) {
        const auto& key = entry.first;
        if (!firstNBT->hasKey(key)) {
            printf("first does not contain tag '%s'\n", key.c_str());
        }
    }
}
