#pragma once

#if !defined(__EMSCRIPTEN__) && !defined(MRMESH_NO_VOXEL)
#include "MRMeshFwd.h"
#include "MRAffineXf3.h"
#include "MRSimpleVolume.h"
#include "MRProgressCallback.h"
#include "MRSignDetectionMode.h"
#include "MRExpected.h"
#include <climits>

namespace MR
{

// Callback type for positioning marching cubes vertices
// args: position0, position1, value0, value1, iso
using VoxelPointPositioner = std::function<Vector3f( const Vector3f&, const Vector3f&, float, float, float )>;

// linear interpolation positioner
MRMESH_API Vector3f voxelPositionerLinear( const Vector3f& pos0, const Vector3f& pos1, float v0, float v1, float iso );

struct MarchingCubesParams
{
    /// origin point of voxels box
    Vector3f origin;
    /// progress callback
    ProgressCallback cb;
    float iso{ 0.0f };
    bool lessInside{ false }; // should be false for dense volumes, and true for distance volume
    Vector<VoxelId, FaceId>* outVoxelPerFaceMap{ nullptr }; // optional output map FaceId->VoxelId
    // function to calculate position of result mesh points
    // if the function isn't set, `voxelPositionerLinear` will be used
    // note: this function is called in parallel from different threads
    VoxelPointPositioner positioner = {};
    /// if the mesh exceeds this number of vertices, an error returns
    int maxVertices = INT_MAX;
    /// for simple volumes only: omit checks for NaN values
    /// use it if you're aware that the input volume has no NaN values
    bool omitNaNCheck = false;
};
using VolumeToMeshParams [[deprecated]] = MarchingCubesParams;

// makes Mesh from SimpleVolume with given settings using Marching Cubes algorithm
MRMESH_API Expected<Mesh, std::string> marchingCubes( const SimpleVolume& volume, const MarchingCubesParams& params = {} );
[[deprecated( "use marchingCubes()" )]] MRMESH_API Expected<Mesh, std::string> simpleVolumeToMesh( const SimpleVolume& volume, const MarchingCubesParams& params = {} );

// makes Mesh from VdbVolume with given settings using Marching Cubes algorithm
MRMESH_API Expected<Mesh, std::string> marchingCubes( const VdbVolume& volume, const MarchingCubesParams& params = {} );
[[deprecated( "use marchingCubes()" )]] MRMESH_API Expected<Mesh, std::string> vdbVolumeToMesh( const VdbVolume& volume, const MarchingCubesParams& params = {} );

// makes Mesh from FunctionVolume with given settings using Marching Cubes algorithm
MRMESH_API Expected<Mesh, std::string> marchingCubes( const FunctionVolume& volume, const MarchingCubesParams& params = {} );
[[deprecated( "use marchingCubes()" )]] MRMESH_API Expected<Mesh, std::string> functionVolumeToMesh( const FunctionVolume& volume, const MarchingCubesParams& params = {} );

}
#endif