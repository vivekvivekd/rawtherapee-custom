/*
 *  This file is part of RawTherapee.
 *
 *  Global Film Grain tool: applies the Local Adjustments grain generator to the
 *  whole image, right after Film Simulation in the Color tab.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */
#pragma once

#include "toolpanel.h"
#include "widgets/basic/adjuster.h"

#include <gtkmm.h>

class FilmGrain final : public ToolParamBlock, public AdjusterListener, public FoldableToolPanel
{
private:
    Adjuster *iso;
    Adjuster *strength;
    Adjuster *scale;
    Adjuster *gamma;

    rtengine::ProcEvent EvFilmGrainEnabled;
    rtengine::ProcEvent EvFilmGrainIso;
    rtengine::ProcEvent EvFilmGrainStrength;
    rtengine::ProcEvent EvFilmGrainScale;
    rtengine::ProcEvent EvFilmGrainGamma;

public:
    static const Glib::ustring TOOL_NAME;

    FilmGrain();

    void read(const rtengine::procparams::ProcParams *pp, const ParamsEdited *pedited = nullptr) override;
    void write(rtengine::procparams::ProcParams *pp, ParamsEdited *pedited = nullptr) override;
    void setDefaults(const rtengine::procparams::ProcParams *defParams, const ParamsEdited *pedited = nullptr) override;
    void setBatchMode(bool batchMode) override;

    void adjusterChanged(Adjuster *a, double newval) override;
    void enabledChanged() override;
};
