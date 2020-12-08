#include "SpiPacketParser.hpp"

#include <cstdio>

// standard
#include <memory>

// libraries
#include <nlohmann/json.hpp>

// shared
#include "depthai-shared/datatype/DatatypeEnum.hpp"
#include "depthai-shared/datatype/RawBuffer.hpp"
#include "depthai-shared/datatype/RawImgFrame.hpp"
#include "depthai-shared/datatype/RawNNData.hpp"
#include "depthai-shared/datatype/RawImgDetections.hpp"


// StreamPacket structure ->  || imgframepixels... , serialized_object, object_type, serialized_object_size ||
// object_type -> DataType(int), serialized_object_size -> int

namespace dai {

// Reads int from little endian format
inline int readIntLE(uint8_t* data) {
    return data[0] + data[1] * 256 + data[2] * 256 * 256 + data[3] * 256 * 256 * 256;
}

//DatatypeEnum parseDatatype(uint8_t* metaPointer, int metaLength){
//    auto objectType = static_cast<DatatypeEnum>(readIntLE(metaPointer + metaLength - 8));
//    return objectType;
//}

/*
std::shared_ptr<RawBuffer> parseMetadata(uint8_t* metaPointer, int metaLength) {

    int serializedObjectSize = readIntLE(metaPointer + metaLength - 4);
    auto objectType = static_cast<DatatypeEnum>(readIntLE(metaPointer + metaLength - 8));

    if(serializedObjectSize < 0) {
        printf("Bad packet, couldn't parse\n");
    }

    nlohmann::json jser = nlohmann::json::from_msgpack(metaPointer, metaPointer + serializedObjectSize);

    // empty data
    std::vector<uint8_t> data;
    
    // RawBuffer is special case, no metadata is actually serialized
    if(objectType == DatatypeEnum::Buffer) {
        auto pBuf = std::make_shared<RawBuffer>();
        pBuf->data = std::move(data);
        return pBuf;
    }

    switch(objectType) {
        case DatatypeEnum::ImgFrame:
            return parseDatatype<RawImgFrame>(jser, data);
            break;

        case DatatypeEnum::NNData:
            return parseDatatype<RawNNData>(jser, data);
            break;

        case DatatypeEnum::ImgDetections : 
            return parseDatatype<RawImgDetections>(jser, data);
            break;

        default:
            printf("Bad packet, couldn't parse\n");
            break;
    }

    return nullptr;
}



std::vector<std::uint8_t> serializeData(const std::shared_ptr<RawBuffer>& data) {
    std::vector<std::uint8_t> ser;
    if(!data) return ser;

    // Serialization:
    // 1. fill vector with bytes from data.data
    // 2. serialize and append metadata
    // 3. append datatype enum (4B LE)
    // 4. append size (4B LE) of serialized metadata

    std::vector<std::uint8_t> metadata;
    DatatypeEnum datatype;
    data->serialize(metadata, datatype);
    uint32_t metadataSize = metadata.size();

    // 4B datatype & 4B metadata size
    std::uint8_t leDatatype[4];
    std::uint8_t leMetadataSize[4];
    for(int i = 0; i < 4; i++) leDatatype[i] = (static_cast<std::int32_t>(datatype) >> (i * 8)) & 0xFF;
    for(int i = 0; i < 4; i++) leMetadataSize[i] = (metadataSize >> i * 8) & 0xFF;

    ser.insert(ser.end(), data->data.begin(), data->data.end());
    ser.insert(ser.end(), metadata.begin(), metadata.end());
    ser.insert(ser.end(), leDatatype, leDatatype + sizeof(leDatatype));
    ser.insert(ser.end(), leMetadataSize, leMetadataSize + sizeof(leMetadataSize));

    return ser;
}
*/


}  // namespace dai
