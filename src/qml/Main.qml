// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: window

    title: qsTr("Plasma Wallpaper Sync")
    minimumWidth: Kirigami.Units.gridUnit * 28
    minimumHeight: Kirigami.Units.gridUnit * 20

    pageStack.initialPage: Kirigami.ScrollablePage {
        title: qsTr("Wallpaper surfaces")

        ColumnLayout {
            width: parent.width
            spacing: Kirigami.Units.largeSpacing

            Repeater {
                model: syncEngine.surfaceIds

                delegate: Pane {
                    id: card
                    required property string modelData
                    readonly property var surface: syncEngine.surface(modelData)

                    Layout.fillWidth: true

                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Heading {
                            level: 3
                            text: card.surface ? card.surface.displayName : card.modelData
                        }

                        Label {
                            Layout.fillWidth: true
                            text: card.surface && card.surface.currentImagePath
                                ? card.surface.currentImagePath
                                : qsTr("(no wallpaper recorded)")
                            elide: Text.ElideMiddle
                            opacity: 0.7
                            font.family: "monospace"
                        }
                    }
                }
            }
        }
    }
}
