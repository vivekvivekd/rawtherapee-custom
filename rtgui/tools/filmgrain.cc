/*
 *  This file is part of RawTherapee.
 *
 *  Global Film Grain tool.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */
#include "filmgrain.h"

#include "eventmapper.h"

#include "rtengine/procparams.h"

using namespace rtengine;
using namespace rtengine::procparams;

const Glib::ustring FilmGrain::TOOL_NAME = "filmgrain";

FilmGrain::FilmGrain(): FoldableToolPanel(this, TOOL_NAME, M("TP_FILMGRAIN_LABEL"), false, true)
{
    auto m = ProcEventMapper::getInstance();
    EvFilmGrainEnabled  = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_ENABLED");
    EvFilmGrainIso      = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_ISO");
    EvFilmGrainStrength = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_STRENGTH");
    EvFilmGrainScale    = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_SCALE");
    EvFilmGrainGamma    = m->newEvent(LUMINANCECURVE, "HISTORY_MSG_FILMGRAIN_GAMMA");

    set_tooltip_text(M("TP_FILMGRAIN_TOOLTIP"));

    iso      = Gtk::manage(new Adjuster(M("TP_FILMGRAIN_ISO"), 20, 6400, 1, 400));
    strength = Gtk::manage(new Adjuster(M("TP_FILMGRAIN_STRENGTH"), 0, 100, 1, 25));
    scale    = Gtk::manage(new Adjuster(M("TP_FILMGRAIN_SCALE"), 0, 100, 1, 100));
    gamma    = Gtk::manage(new Adjuster(M("TP_FILMGRAIN_GAMMA"), 0.2, 3.0, 0.1, 1.0));

    for (Adjuster* a : {iso, strength, scale, gamma}) {
        a->setAdjusterListener(this);
        a->show();
        pack_start(*a);
    }
}


void FilmGrain::read(const ProcParams *pp, const ParamsEdited *pedited)
{
    disableListener();

    if (pedited) {
        iso->setEditedState(pedited->filmGrain.iso ? Edited : UnEdited);
        strength->setEditedState(pedited->filmGrain.strength ? Edited : UnEdited);
        scale->setEditedState(pedited->filmGrain.scale ? Edited : UnEdited);
        gamma->setEditedState(pedited->filmGrain.gamma ? Edited : UnEdited);
        set_inconsistent(multiImage && !pedited->filmGrain.enabled);
    }

    setEnabled(pp->filmGrain.enabled);
    iso->setValue(pp->filmGrain.iso);
    strength->setValue(pp->filmGrain.strength);
    scale->setValue(pp->filmGrain.scale);
    gamma->setValue(pp->filmGrain.gamma);

    enableListener();
}


void FilmGrain::write(ProcParams *pp, ParamsEdited *pedited)
{
    pp->filmGrain.enabled = getEnabled();
    pp->filmGrain.iso = iso->getIntValue();
    pp->filmGrain.strength = strength->getIntValue();
    pp->filmGrain.scale = scale->getIntValue();
    pp->filmGrain.gamma = gamma->getValue();

    if (pedited) {
        pedited->filmGrain.enabled = !get_inconsistent();
        pedited->filmGrain.iso = iso->getEditedState();
        pedited->filmGrain.strength = strength->getEditedState();
        pedited->filmGrain.scale = scale->getEditedState();
        pedited->filmGrain.gamma = gamma->getEditedState();
    }
}


void FilmGrain::setDefaults(const ProcParams *defParams, const ParamsEdited *pedited)
{
    iso->setDefault(defParams->filmGrain.iso);
    strength->setDefault(defParams->filmGrain.strength);
    scale->setDefault(defParams->filmGrain.scale);
    gamma->setDefault(defParams->filmGrain.gamma);

    if (pedited) {
        iso->setDefaultEditedState(pedited->filmGrain.iso ? Edited : UnEdited);
        strength->setDefaultEditedState(pedited->filmGrain.strength ? Edited : UnEdited);
        scale->setDefaultEditedState(pedited->filmGrain.scale ? Edited : UnEdited);
        gamma->setDefaultEditedState(pedited->filmGrain.gamma ? Edited : UnEdited);
    } else {
        iso->setDefaultEditedState(Irrelevant);
        strength->setDefaultEditedState(Irrelevant);
        scale->setDefaultEditedState(Irrelevant);
        gamma->setDefaultEditedState(Irrelevant);
    }
}


void FilmGrain::adjusterChanged(Adjuster* a, double newval)
{
    if (listener && getEnabled()) {
        if (a == iso) {
            listener->panelChanged(EvFilmGrainIso, a->getTextValue());
        } else if (a == strength) {
            listener->panelChanged(EvFilmGrainStrength, a->getTextValue());
        } else if (a == scale) {
            listener->panelChanged(EvFilmGrainScale, a->getTextValue());
        } else if (a == gamma) {
            listener->panelChanged(EvFilmGrainGamma, a->getTextValue());
        }
    }
}


void FilmGrain::enabledChanged()
{
    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged(EvFilmGrainEnabled, M("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged(EvFilmGrainEnabled, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged(EvFilmGrainEnabled, M("GENERAL_DISABLED"));
        }
    }
}


void FilmGrain::setBatchMode(bool batchMode)
{
    ToolPanel::setBatchMode(batchMode);

    iso->showEditedCB();
    strength->showEditedCB();
    scale->showEditedCB();
    gamma->showEditedCB();
}
