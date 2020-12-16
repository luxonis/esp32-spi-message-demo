#pragma once

// standard
#include <memory>

//shared
#include "depthai-shared/datatype/RawBuffer.hpp"

// StreamPacket structure ->  || imgframepixels... , serialized_object, object_type, serialized_object_size ||
// object_type -> DataType(int), serialized_object_size -> int

namespace dai {

std::shared_ptr<RawBuffer> parseMetadata(uint8_t* metaPointer, int metaLength);
std::vector<std::uint8_t> serializeData(const std::shared_ptr<RawBuffer>& data);

DatatypeEnum parseDatatype(uint8_t* metaPointer, int metaLength);


}  // namespace dai
