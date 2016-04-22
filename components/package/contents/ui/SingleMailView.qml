/*
 * Copyright (C) 2015 Michael Bohlender <michael.bohlender@kdemail.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

import org.kube.framework.domain 1.0 as KubeFramework
import org.kube.framework.theme 1.0

Item {
    id: root
    property variant mail;

    Rectangle {
        id: background

        anchors.fill: parent

        color: ColorPalette.background
    }

    Repeater {
        anchors.fill: parent

        model: KubeFramework.MailListModel {
            mail: root.mail
        }

        delegate: Item {
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent

                Label {
                    text: model.id
                }

                Label {
                    text: model.sender
                }

                Label {
                    text: model.senderName
                }

                Label {
                    text: model.subject
                }

                MailViewer {
                    message: model.mimeMessage
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

            }
        }
    }
}