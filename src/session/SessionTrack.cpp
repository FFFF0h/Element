/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "session/Sequence.h"
#include "MediaManager.h"

namespace Element {

ClipModel Sequence::Track::addClip (const File &file, double startTime)
{

    ClipModel invalid;
#if 0
    if (! supportsFile (file))
        return invalid;

    AssetItem root (session->assets().root());

    if (! root.findItemForFile (file).isValid())
    {
        if (! root.addFile (file, -1, false)) {
            std::clog << "couldn't create asset for file during clip addition\n";
            return invalid;
        }
    }

    AssetItem item (root.findItemForFile (file));

#if 0
    if (PatternPtr pat = session->getPatternObject (item))
    {
        ClipModel clip (startTime, pat->totalBars() * 4 * Shuttle::PPQ);
        clip.node().setProperty ("media", mediaType(), nullptr);
        clip.node().setProperty (Slugs::assetId, item.getId(), nullptr);
        trackData.addChild (clip.node(), -1, session->undoManager());
        return clip;
    }
#endif
#endif
    return invalid;
}

Sequence::Track Sequence::Track::next() const
{
    return Track (session, session->node().getChild (
                  session->node().indexOf (trackData) + 1));
}

Sequence::Track Sequence::Track::previous() const
{
    return Track (session, session->node().getChild (
                  session->node().indexOf (trackData) - 1));
}

void
Sequence::Track::removeFromSession()
{        
    trackData.removeAllChildren (undoManager());
    session->sequenceNode().removeChild (trackData, undoManager());
}

bool Sequence::Track::supportsAsset (const AssetItem &asset) const
{
    return supportsFile (asset.getFile());
}

bool
Sequence::Track::supportsClip (const ClipModel &clip) const
{
    return clip.node().getProperty("media").equals (trackData.getProperty ("type"));
}

bool
Sequence::Track::supportsFile (const File &file) const
{
    return session->media().canOpenFile (file);
}

}
