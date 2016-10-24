/*
  Copyright (C) 2016 Michael Bohlender, <michael.bohlender@kdemail.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

import QtQuick 2.4
import QtQuick.Controls 1.3
import QtWebKit 3.0
// import QtWebEngine 1.3 //This would give use contentsSize
import QtWebEngine 1.2

Item {
    id: root
    property string content: model.htmlContent
    property int contentHeight: helperView.contentHeight;
    //FIXME workaround until QtWebEngine 1.3 with contentsSize

    height: contentHeight
    width: delegateRoot.width

    WebView {
        id: helperView
        visible: false
        Component.onCompleted: loadHtml(content, "file:///")
    }
    WebEngineView {
        id: htmlView
        anchors.fill: parent
        Component.onCompleted: loadHtml(content, "file:///")
    }
    onContentChanged: {
        htmlView.loadHtml(content, "file:///");
        helperView.loadHtml(content, "file:///");
    }
}