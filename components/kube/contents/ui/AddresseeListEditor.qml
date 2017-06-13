/*
 *  Copyright (C) 2017 Michael Bohlender, <michael.bohlender@kdemail.net>
 *  Copyright (C) 2017 Christian Mollekopf, <mollekopf@kolabsys.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


import QtQuick 2.7
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

import org.kube.framework 1.0 as Kube

Item {
    id: root
    property variant completer
    property alias model: listView.model

    signal added(string text)
    signal removed(string text)

    implicitHeight: listView.height + lineEdit.height
    height: implicitHeight

    Column {
        anchors.fill: parent
        ListView {
            id: listView
            anchors {
                left: parent.left
                right: parent.right
            }
            height: contentHeight
            delegate: Text {
                text: display
            }
        }
        Kube.AutocompleteLineEdit {
            id: lineEdit
            anchors {
                left: parent.left
                right: parent.right
            }
            placeholderText: "Add recepient"
            model: root.completer.model
            onSearchTermChanged: root.completer.searchString = searchTerm
            onAccepted: {
                root.added(text);
                console.warn("Accepted input: ", text)
                clear()
            }
        }
    }
}