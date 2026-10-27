#include "paintingrequirement_helper.h"


PaintingDecision
PaintingRequirementHelper::evaluate(
    const NamedColor& requiredColor,
    SurfaceType requiredSurface,
    const NamedColor& materialColor,
    SurfaceType materialSurface,
    PaintingMode paintingMode)
{
    PaintingDecision result;

    result.paintingPossible =
        paintingMode != PaintingMode::None;

    if (!requiredColor.isValid())
    {
        result.reason =
            "Nincs szín előírva";

        result.paintingNeeded = false;

        return result;
    }

    // result.paintingPossible =
    //     paintingMode != PaintingMode::None;

    result.sameColor =
        requiredColor.code() ==
        materialColor.code();

    result.surfaceSpecified =
        requiredSurface != SurfaceType::Unknown;

    result.sameSurface =
        requiredSurface == materialSurface;

    bool surfaceOk =
        !result.surfaceSpecified
        ||
        result.sameSurface;

    result.paintingNeeded =
        !(result.sameColor && surfaceOk);

    if (!result.paintingPossible)
    {
        result.reason =
            "Az anyag nem festhető";

        result.paintingNeeded = false;

        return result;
    }

    if (!result.paintingNeeded)
    {
        result.reason =
            "Szín- és felületazonos";
    }
    else if (!result.sameColor && !surfaceOk)
    {
        result.reason =
            "Eltérő szín és felület";
    }
    else if (!result.sameColor)
    {
        result.reason =
            "Eltérő szín";
    }
    else
    {
        result.reason =
            "Eltérő felület";
    }

    return result;
}

PaintingDecision
PaintingRequirementHelper::evaluate(
    const NamedColor& requiredColor,
    SurfaceType requiredSurface,
    const MaterialMaster& material)
{
    return evaluate(
        requiredColor,
        requiredSurface,
        material.color,
        material.surface,
        material.paintingMode);
}

