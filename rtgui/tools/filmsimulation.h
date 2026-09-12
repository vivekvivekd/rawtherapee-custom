#pragma once

#include <memory>

#include <gtkmm.h>

#include <glibmm/ustring.h>

#include "guiutils.h"
#include "toolpanel.h"
#include "widgets/basic/adjuster.h"

/**
 * Lightroom-style film simulation browser: a permanently visible tree of
 * folders and HaldCLUT files. Folders expand/collapse on click, films apply
 * on click or when moved to with the arrow keys.
 *
 * (The class keeps its historical name so callers don't need to change.)
 */
class ClutComboBox final :
    public Gtk::TreeView
{
public:
    explicit ClutComboBox(const Glib::ustring &path);
    int foundClutsCount() const;
    Glib::ustring getSelectedClut();
    void setSelectedClut( Glib::ustring filename );
    void setBatchMode(bool yes);

    /// Emitted whenever the user changes the selected row (folder or film).
    sigc::signal<void>& signal_changed();

    static void cleanup();

private:
    void updateUnchangedEntry(); // in batchMode we need to add an extra entry "(Unchanged)". We do this whenever the widget is mapped (connecting to signal_map()), unless options.multiDisplayMode (see the comment below about cm2 in this case)
    void onSelectionChanged();
    void onRowActivated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

    class ClutColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<Glib::ustring> clutFilename;
        Gtk::TreeModelColumn<int> weight;
        ClutColumns();
    };

    class ClutModel {
    public:
        Glib::RefPtr<Gtk::TreeStore> m_model;
        ClutColumns m_columns;
        int count;
        explicit ClutModel(const Glib::ustring &path);
        int parseDir (const Glib::ustring& path);
    };

    Glib::RefPtr<Gtk::TreeStore> &m_model();
    ClutColumns &m_columns();

    Gtk::TreeIter findRowByClutFilename(  Gtk::TreeModel::Children childs, Glib::ustring filename );

    static std::unique_ptr<ClutModel> cm; // we use a shared TreeModel for all the widgets, to save time (no need to reparse the clut dir multiple times)...
    static std::unique_ptr<ClutModel> cm2; // ... except when options.multiDisplayMode (i.e. editors in their own window), where we need two. This is because we might have two widgets displayed at the same time in this case
    bool batchMode;
    Glib::ustring selectedClutFilename; // last *film* chosen; folder rows never overwrite it
    sigc::signal<void> sigChanged;
};

class FilmSimulation : public ToolParamBlock, public AdjusterListener, public FoldableToolPanel
{
public:
    static const Glib::ustring TOOL_NAME;

    FilmSimulation();

    void adjusterChanged(Adjuster* a, double newval) override;
    void setBatchMode(bool batchMode) override;
    void read(const rtengine::procparams::ProcParams* pp, const ParamsEdited* pedited = nullptr) override;
    void write(rtengine::procparams::ProcParams* pp, ParamsEdited* pedited = nullptr) override;
    void setAdjusterBehavior(bool strength);
    void trimValues(rtengine::procparams::ProcParams* pp) override;

private:
    void onClutSelected();
    void enabledChanged() override;

    void updateDisable( bool value );
    void updateCurrentLabel();

    ClutComboBox *m_clutComboBox;
    Gtk::Label *m_currentLabel;
    sigc::connection m_clutComboBoxConn;
    Glib::ustring m_oldClutFilename;

    Adjuster *m_strength;
};
