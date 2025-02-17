#pragma once

#include "MRMeshFwd.h"

namespace MR
{

enum class Side
{
    Negative,
    Positive
};

/// Tests \param shellPoint from bidirectional shell constructed for an open \param mesh;
/// \param side specifies which side of shell is of interest: negative or positive relative to mesh normals;
/// \return whether the distance from given point to given mesh is of same sign as \param side,
/// always returning false for the points projecting on mesh boundary
[[nodiscard]] MRMESH_API bool isInnerShellVert( const MeshPart & mp, const Vector3f & shellPoint, Side side );

/// Finds inner-shell vertices on bidirectional \param shell constructed for an open \param mesh;
/// \param side specifies which side of shell is of interest: negative or positive relative to mesh normals;
/// The function will return all shell vertices that have distance to mesh of same sign as \param side
/// excluding the vertices projecting on mesh boundary
[[nodiscard]] MRMESH_API VertBitSet findInnerShellVerts( const MeshPart & mp, const Mesh & shell, Side side );

/// Finds inner-shell faces on bidirectional \param shell constructed for an open \param mesh;
/// \param side specifies which side of shell is of interest: negative or positive relative to mesh normals;
/// The function will return all shell faces (after some subdivision) that have distance to mesh of same sign as \param side
/// excluding the face projecting on mesh boundary
[[nodiscard]] MRMESH_API FaceBitSet findInnerShellFacesWithSplits( const MeshPart & mp, Mesh & shell, Side side );

} // namespace MR
