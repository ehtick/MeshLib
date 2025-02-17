#pragma once
#include "MRMeshFwd.h"
#include "MRExpected.h"
#include "MRBuffer.h"
#include <istream>

namespace MR
{

// returns offsets for each new line in monolith char block
MRMESH_API std::vector<size_t> splitByLines( const char* data, size_t size );

// reads input stream to monolith char block
MRMESH_API Expected<Buffer<char>, std::string> readCharBuffer( std::istream& in );

// read coordinates to `v` separated by space
MRMESH_API VoidOrErrStr parseTextCoordinate( const std::string_view& str, Vector3f& v );
MRMESH_API VoidOrErrStr parseObjCoordinate( const std::string_view& str, Vector3f& v );

}