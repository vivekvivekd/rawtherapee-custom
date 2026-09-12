/*
 *  This file is part of RawTherapee.
 *
 *  Halation tool.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */
#include "halation.h"

#include "eventmapper.h"

#include "rtengine/procparams.h"

using namespace rtengine;
using namespace rtengine::procparams;

const Glib::ustring Halation::TOOL_NAME = "halation";

Halation::Halation(): FoldableToolPanel(this, TOOL_NAME, M("TP_HALATION_LABEL"), false, true)
{
    auto m = ProcEventMapper::getInstance();
    EvHalationEnabled   = m->newEvent(HDR, "HISTORY_MSG_HALATION_ENABLED");
    EvHalationStrength  = m->newEvent(HDR, "HISTORY_MSG_HALATION_STRENGTH");
    EvHalationRadius    = m->newEvent(HDR, "HISTORY_MSG_HALATION_RADIUS");
    EvHalationThreshold = m->newEvent(HDR, "HISTORY_MSG_HALATION_THRESHOLD");
    EvHalationHue       = m->newEvent(HDR, "HISTORY_MSG_HALATION_HUE");
    EvHalationBloom     = m->newEvent(HDR, "HISTORY_MSG_HALATION_BLOOM");
    EvHalationBloomRadius = m->newEvent(HDR, "HISTORY_MSG_HALATION_BLOOMRADIUS");

    set_tooltip_text(M("TP_HALATION_TOOLTIP"));

    strength  = Gtk::manage(new Adjuster(M("TP_HALATION_STRENGTH"), 0, 100, 1, 30));
    radius    = Gtk::manage(new Adjuster(M("TP_HALATION_RADIUS"), 1, 300, 1, 40));
    threshold = Gtk::manage(new Adjuster(M("TP_HALATION_THRESHOLD"), 0, 100, 1, 60));
    hue       = Gtk::manage(new Adjuster(M("TP_HALATION_HUE"), 0, 90, 1, 15));
    bloom     = Gtk::manage(new Adjuster(M("TP_HALATION_BLOOM"), 0, 100, 1, 0));
    bloomRadius = Gtk::manage(new Adjuster(M("TP_HALATION_BLOOMRADIUS"), 1, 500, 1, 120));

    for (Adjuster* a : {strength, radius, threshold, hue, bloom, bloomRadius}) {
        a->setAdjusterListener(this);
        a->show();
        pack_start(*a);
    }
}


void Halation::read(const ProcParams *pp, const ParamsEdited *pedited)
{
    disableListener();

    if (pedited) {
        strength->setEditedState(pedited->halation.strength ? Edited : UnEdited);
        radius->setEditedState(pedited->halation.radius ? Edited : UnEdited);
        threshold->setEditedState(pedited->halation.threshold ? Edited : UnEdited);
        hue->setEditedState(pedited->halation.hue ? Edited : UnEdited);
        bloom->setEditedState(pedited->halation.bloom ? Edited : UnEdited);
        bloomRadius->setEditedState(pedited->halation.bloomRadius ? Edited : UnEdited);
        set_inconsistent(multiImage && !pedited->halation.enabled);
    }

    setEnabled(pp->halation.enabled);
    strength->setValue(pp->halation.strength);
    radius->setValue(pp->halation.radius);
    threshold->setValue(pp->halation.threshold);
    hue->setValue(pp->halation.hue);
    bloom->setValue(pp->halation.bloom);
    bloomRadius->setValue(pp->halation.bloomRadius);

    enableListener();
}


void Halation::write(ProcParams *pp, ParamsEdited *pedited)
{
    pp->halation.enabled = getEnabled();
    pp->halation.strength = strength->getIntValue();
    pp->halation.radius = radius->getIntValue();
    pp->halation.threshold = threshold->getIntValue();
    pp->halation.hue = hue->getIntValue();
    pp->halation.bloom = bloom->getIntValue();
    pp->halation.bloomRadius = bloomRadius->getIntValue();

    if (pedited) {
        pedited->halation.enabled = !get_inconsistent();
        pedited->halation.strength = strength->getEditedState();
        pedited->halation.radius = radius->getEditedState();
        pedited->halation.threshold = threshold->getEditedState();
        pedited->halation.hue = hue->getEditedState();
        pedited->halation.bloom = bloom->getEditedState();
        pedited->halation.bloomRadius = bloomRadius->getEditedState();
    }
}


void Halation::setDefaults(const ProcParams *defParams, const ParamsEdited *pedited)
{
    strength->setDefault(defParams->halation.strength);
    radius->setDefault(defParams->halation.radius);
    threshold->setDefault(defParams->halation.threshold);
    hue->setDefault(defParams->halation.hue);
    bloom->setDefault(defParams->halation.bloom);
    bloomRadius->setDefault(defParams->halation.bloomRadius);

    if (pedited) {
        strength->setDefaultEditedState(pedited->halation.strength ? Edited : UnEdited);
        radius->setDefaultEditedState(pedited->halation.radius ? Edited : UnEdited);
        threshold->setDefaultEditedState(pedited->halation.threshold ? Edited : UnEdited);
        hue->setDefaultEditedState(pedited->halation.hue ? Edited : UnEdited);
        bloom->setDefaultEditedState(pedited->halation.bloom ? Edited : UnEdited);
        bloomRadius->setDefaultEditedState(pedited->halation.bloomRadius ? Edited : UnEdited);
    } else {
        strength->setDefaultEditedState(Irrelevant);
        radius->setDefaultEditedState(Irrelevant);
        threshold->setDefaultEditedState(Irrelevant);
        hue->setDefaultEditedState(Irrelevant);
        bloom->setDefaultEditedState(Irrelevant);
        bloomRadius->setDefaultEditedState(Irrelevant);
    }
}


void Halation::adjusterChanged(Adjuster* a, double newval)
{
    if (listener && getEnabled()) {
        if (a == strength) {
            listener->panelChanged(EvHalationStrength, a->getTextValue());
        } else if (a == radius) {
            listener->panelChanged(EvHalationRadius, a->getTextValue());
        } else if (a == threshold) {
            listener->panelChanged(EvHalationThreshold, a->getTextValue());
        } else if (a == hue) {
            listener->panelChanged(EvHalationHue, a->getTextValue());
        } else if (a == bloom) {
            listener->panelChanged(EvHalationBloom, a->getTextValue());
        } else if (a == bloomRadius) {
            listener->panelChanged(EvHalationBloomRadius, a->getTextValue());
        }
    }
}


void Halation::enabledChanged()
{
    if (listener) {
        if (get_inconsistent()) {
            listener->panelChanged(EvHalationEnabled, M("GENERAL_UNCHANGED"));
        } else if (getEnabled()) {
            listener->panelChanged(EvHalationEnabled, M("GENERAL_ENABLED"));
        } else {
            listener->panelChanged(EvHalationEnabled, M("GENERAL_DISABLED"));
        }
    }
}


void Halation::setBatchMode(bool batchMode)
{
    ToolPanel::setBatchMode(batchMode);

    strength->showEditedCB();
    radius->showEditedCB();
    threshold->showEditedCB();
    hue->showEditedCB();
    bloom->showEditedCB();
    bloomRadius->showEditedCB();
}
