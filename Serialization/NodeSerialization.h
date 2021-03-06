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

#ifndef NODESERIALIZATION_H
#define NODESERIALIZATION_H

#include <climits>

#include "Serialization/KnobSerialization.h"
#include "Serialization/TrackerSerialization.h"
#include "Serialization/RotoContextSerialization.h"
#include "Serialization/ImageParamsSerialization.h"
#include "Serialization/SerializationBase.h"
#include "Serialization/SerializationFwd.h"


SERIALIZATION_NAMESPACE_ENTER;

class NodeSerialization : public SerializationObjectBase
{
public:

    ///Used to serialize
    explicit NodeSerialization()
    : _knobsValues()
    , _groupFullyQualifiedScriptName()
    , _nodeLabel()
    , _nodeScriptName()
    , _pluginID()
    , _pluginMajorVersion(-1)
    , _pluginMinorVersion(-1)
    , _masterNodeFullyQualifiedScriptName()
    , _inputs()
    , _rotoContext()
    , _trackerContext()
    , _multiInstanceParentName()
    , _userPages()
    , _pagesIndexes()
    , _children()
    , _userComponents()
    , _nodePositionCoords()
    , _nodeSize()
    , _nodeColor()
    , _overlayColor()
    , _viewerUIKnobsOrder()
    {
        _nodePositionCoords[0] = _nodePositionCoords[1] = INT_MIN;
        _nodeSize[0] = _nodeSize[1] = -1;
        _nodeColor[0] = _nodeColor[1] = _nodeColor[2] = -1;
        _overlayColor[0] = _overlayColor[1] = _overlayColor[2] = -1;

    }


    virtual ~NodeSerialization()
    {
    }



    // Knobs serialization
    KnobSerializationList _knobsValues;

    // The group full script-name or empty if the node is part of the top-level group
    std::string _groupFullyQualifiedScriptName;

    // The node label as seen in the nodegraph
    std::string _nodeLabel;

    // The node script-name as used in Python
    std::string _nodeScriptName;

    // The ID of the plug-in embedded into the node
    std::string _pluginID;

    // Plugin version used by the node
    int _pluginMajorVersion;
    int _pluginMinorVersion;

    // If this node is a clone of another one, this is the full script-name of the master node
    std::string _masterNodeFullyQualifiedScriptName;

    // Serialization of inputs, this is a map of the input label to the script-name (not full) of the input node
    std::map<std::string, std::string> _inputs;

    // If this node has a Roto context, this is its serialization
    boost::shared_ptr<RotoContextSerialization> _rotoContext;
    // If this node has a Tracker context, this is its serialization
    boost::shared_ptr<TrackerContextSerialization> _trackerContext;

    // Deprecated: In Natron 1, the tracker was a bundle with sub instances where each track would be 1 node.
    // This is left here for backward compatibility.
    std::string _multiInstanceParentName;

    // The serialization of the pages created by the user
    std::list<boost::shared_ptr<GroupKnobSerialization> > _userPages;

    // The pages order in the node by script-name
    std::list<std::string> _pagesIndexes;

    // If this node is a group or a multi-instance, this is the children
    NodeSerializationList _children;

    // This is the user created components on the node
    std::list<ImageComponentsSerialization> _userComponents;

    // UI stuff
    double _nodePositionCoords[2]; // x,y  X=Y=INT_MIN if there is no position info
    double _nodeSize[2]; // width, height, W=H=-1 if there is no size info
    double _nodeColor[3]; // node color (RGB), between 0. and 1. If R=G=B=-1 then no color
    double _overlayColor[3]; // overlay color (RGB), between 0. and 1. If R=G=B=-1 then no color

    // Ordering of the knobs in the viewer UI for this node
    std::list<std::string> _viewerUIKnobsOrder;
    
    // If this node was built with a preset, this is the one
    std::string _presetLabel;

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};


class NodePresetSerialization : public SerializationObjectBase
{
public:

    // The plug-in ID of the node to create. For PyPlugs this should be the plug-in ID of a Group
    std::string originalPluginID;

    // If set the preset will appear as a different plug-in than the one set in originalPluginID.
    // If not set it will appear as a preset of originalPluginID
    std::string presetID;

    // You may set a grouping into which the tool should be
    // If not set it will default to the grouping of the plug-in set in originalPluginID
    std::string presetGrouping;

    // The label of the preset as it should appear in the GUI
    std::string presetLabel;

    // Optionally the filename of an icon file (without path) that should be located next to the preset file
    std::string presetIcon;

    // When setting a presetID may be useful to notify the user that the plug-in changed. Only relevant with
    // a non empty presetID
    int version;

    // Optionally the default keyboard symbol (as in Natron::Key) and modifiers (as in Natron::KeyboardModifiers)
    // that should by default be used to instantiate the plug-in. (Note that the user may still change it afterwards
    // from the Shortcut Editor)
    int presetSymbol;
    int presetModifiers;

    // The serialization that should set the state of the node by default
    NodeSerialization node;

    // This flag is used to speed-up preset parsing by Natron: it will only read relevant meta-data from this class
    // to show to the user in the interface instead of the whole node serialization object.
    bool decodeMetaDataOnly;

    NodePresetSerialization()
    : SerializationObjectBase()
    , originalPluginID()
    , presetID()
    , presetGrouping()
    , presetLabel()
    , presetIcon()
    , version(0)
    , presetSymbol(0)
    , presetModifiers(0)
    , node()
    , decodeMetaDataOnly(false)
    {

    }

    virtual ~NodePresetSerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;
};

SERIALIZATION_NAMESPACE_EXIT;

#endif // NODESERIALIZATION_H
