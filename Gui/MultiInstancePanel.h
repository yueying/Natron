/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef MULTIINSTANCEPANEL_H
#define MULTIINSTANCEPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QThread>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Knob.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/TrackerContext.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief This class represents a multi-instance settings panel.
 **/
struct MultiInstancePanelPrivate;
class MultiInstancePanel
    :  public NamedKnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected: // derives from KnobHolder, parent of TrackerPanelV1
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    MultiInstancePanel(const NodeGuiPtr & node);

public:
    static MultiInstancePanelPtr create(const NodeGuiPtr& node) WARN_UNUSED_RETURN
    {
        return MultiInstancePanelPtr( new MultiInstancePanel(node) );
    }

    virtual ~MultiInstancePanel();

    void createMultiInstanceGui(QVBoxLayout* layout);

    bool isGuiCreated() const;

    void addRow(const NodePtr & node);

    void removeRow(int index);

    int getNodeIndex(const NodePtr & node) const;

    const std::list< std::pair<NodeWPtr, bool > > & getInstances() const;
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL;
    NodePtr getMainInstance() const;

    NodeGuiPtr getMainInstanceGui() const;

    void getSelectedInstances(std::list<NodePtr>* instances) const;

    KnobIPtr getKnobForItem(TableItem* item, int* dimension) const;
    Gui* getGui() const;
    virtual void setIconForButton(const KnobButtonPtr& /*knob*/)
    {
    }

    NodePtr createNewInstance(bool useUndoRedoStack);

    void selectNode(const NodePtr & node, bool addToSelection);

    void selectNodes(const std::list<NodePtr> & nodes, bool addToSelection);

    void removeNodeFromSelection(const NodePtr & node);

    void clearSelection();

    bool isSettingsPanelVisible() const;

    void removeInstances(const NodesList& instances);
    void addInstances(const NodesList& instances);

    void onChildCreated(const NodePtr& node);

    void setRedrawOnSelectionChanged(bool redraw);

public Q_SLOTS:


    void onSelectAllButtonClicked();

    void onSelectionChanged(const QItemSelection & oldSelection, const QItemSelection & newSelection);

    void onItemDataChanged(TableItem* item);

    void onCheckBoxChecked(bool checked);

    void onDeleteKeyPressed();

    void onInstanceKnobValueChanged(ViewSpec view, int dim, int reason);

    void onSettingsPanelClosed(bool closed);

    void onItemRightClicked(TableItem* item);

protected:

    virtual void appendExtraGui(QVBoxLayout* /*layout*/)
    {
    }

    virtual void appendButtons(QHBoxLayout* /*buttonLayout*/)
    {
    }

    NodePtr addInstanceInternal(bool useUndoRedoStack);
    virtual void initializeExtraKnobs()
    {
    }

    virtual void showMenuForInstance(const NodePtr& /*instance*/)
    {
    }

private:

    virtual void onButtonTriggered(const KnobButtonPtr& button);

    void removeInstancesInternal();

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual bool onKnobValueChanged(const KnobIPtr& k,
                                    ValueChangedReasonEnum reason,
                                    double time,
                                    ViewSpec view,
                                    bool originatedFromMainThread) OVERRIDE;
    boost::scoped_ptr<MultiInstancePanelPrivate> _imp;
};

struct TrackerPanelPrivateV1;
class TrackerPanelV1
    : public MultiInstancePanel, public TrackerParamsProvider
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobHolder
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    TrackerPanelV1(const NodeGuiPtr & node);

public:
    static TrackerPanelV1Ptr create(const NodeGuiPtr& node) WARN_UNUSED_RETURN
    {
        return TrackerPanelV1Ptr( new TrackerPanelV1(node) );
    }

    virtual ~TrackerPanelV1();

    ///Each function below returns true if there is a selection, false otherwise
    bool trackBackward(const ViewerInstancePtr& viewer);
    bool trackForward(const ViewerInstancePtr& viewer);
    bool trackPrevious(const ViewerInstancePtr& viewer);
    bool trackNext(const ViewerInstancePtr& viewer);
    void stopTracking();
    bool isTracking() const;

    void clearAllAnimationForSelection();

    void clearBackwardAnimationForSelection();

    void clearForwardAnimationForSelection();

public Q_SLOTS:

    void onAverageTracksButtonClicked();
    void onExportButtonClicked();

    void onTrackingStarted(int step);

    void onTrackingFinished();

Q_SIGNALS:

    void trackingEnded();

private:

    virtual void initializeExtraKnobs() OVERRIDE FINAL;
    virtual void appendExtraGui(QVBoxLayout* layout) OVERRIDE FINAL;
    virtual void appendButtons(QHBoxLayout* buttonLayout) OVERRIDE FINAL;
    virtual void setIconForButton(const KnobButtonPtr& knob) OVERRIDE FINAL;
    virtual void onButtonTriggered(const KnobButtonPtr& button) OVERRIDE FINAL;
    virtual void showMenuForInstance(const NodePtr& item) OVERRIDE FINAL;
    boost::scoped_ptr<TrackerPanelPrivateV1> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // MULTIINSTANCEPANEL_H
