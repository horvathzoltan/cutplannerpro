#pragma once

#include "common/surface/surfacetype.h"
#include <QString>

#include <common/color/namedcolor.h>

#include <materials/model/material_master.h>

struct PaintingDecision
{
    bool paintingPossible = false;
    bool paintingNeeded = false;

    bool sameColor = false;
    bool sameSurface = false;

    bool surfaceSpecified = false;

    QString reason;
};


namespace PaintingRequirementHelper
{

PaintingDecision evaluate(
    const NamedColor& requiredColor,
    SurfaceType requiredSurface,
    const MaterialMaster& material);

PaintingDecision evaluate(
    const NamedColor& requiredColor,
    SurfaceType requiredSurface,
    const NamedColor& materialColor,
    SurfaceType materialSurface,
    PaintingMode paintingMode);

}