#pragma once
#include "MRMeshFwd.h"
#include "MRVector3.h"
#include "MRPch/MRJson.h"
#include <array>

namespace MR
{

/// class with CNC machine emulation settings
class CNCMachineSettings
{
public:
    // enumeration of axes of rotation
    enum class RotationAxisName
    {
        A,
        B,
        C
    };
    using RotationAxesOrder = std::vector<RotationAxisName>;

    static int getAxesCount() { return int( RotationAxisName::C ) + 1; }

    // rotationAxis length will be more then 0.01
    MRMESH_API void setRotationAxis( RotationAxisName paramName, const Vector3f& rotationAxis );
    MRMESH_API const Vector3f& getRotationAxis( RotationAxisName paramName ) const;
    // duplicated values will be removed (ABAAC - > ABC)
    MRMESH_API void setRotationOrder( const RotationAxesOrder& rotationAxesOrder );
    const RotationAxesOrder& getRotationOrder() const { return rotationAxesOrder_; }
    // set feedrate idle. valid range - [0, 100000]
    // 0 - feedrate idle set as maximum feedrate on any action, or 100 if feedrate is not set in any action
    MRMESH_API void setFeedrateIdle( float feedrateIdle );
    float getFeedrateIdle() const { return feedrateIdle_; }

    MRMESH_API Json::Value saveToJson() const;
    MRMESH_API bool loadFromJson( const Json::Value& jsonValue );

private:
    // direction of axes around which the rotation occurs A, B, C
    std::array<Vector3f, 3> rotationAxes_ = { Vector3f::minusX(), Vector3f::minusY(), Vector3f::plusZ() };
    // order of application of rotations
    RotationAxesOrder rotationAxesOrder_ = { RotationAxisName::A, RotationAxisName::B, RotationAxisName::C };
    // feedrate idle. 0 - feedrate idle set as maximum feedrate on any action, or 100 if feedrate is not set in any action
    float feedrateIdle_ = 10000.f;
};

}
