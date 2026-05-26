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
    minimumHeight: Kirigami.Units.gridUnit * 36

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

                        // 16:9 preview frame. The Image overlays the placeholder
                        // Label only when Image.status === Ready, so missing /
                        // failed / slow loads all surface as a readable
                        // message instead of an empty rectangle.
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: width * 9 / 16
                            color: Kirigami.Theme.alternateBackgroundColor
                            radius: Kirigami.Units.smallSpacing
                            clip: true

                            Label {
                                anchors.centerIn: parent
                                opacity: 0.6
                                text: {
                                    if (!card.surface || !card.surface.currentImagePath)
                                        return qsTr("(no wallpaper recorded)");
                                    if (preview.status === Image.Loading)
                                        return qsTr("Loading preview…");
                                    return qsTr("Failed to load preview");
                                }
                            }

                            Image {
                                id: preview
                                anchors.fill: parent
                                // Cap decode size — Plasma wallpapers can be 4K,
                                // we render a thumbnail. Aspect ratio is
                                // preserved automatically when only width is set.
                                sourceSize.width: 480
                                source: card.surface ? card.surface.currentImagePath : ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                visible: status === Image.Ready
                            }
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
