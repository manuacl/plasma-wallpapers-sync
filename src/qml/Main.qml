// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: window

    title: qsTr("Plasma Wallpaper Sync")
    minimumWidth: Kirigami.Units.gridUnit * 30
    minimumHeight: Kirigami.Units.gridUnit * 40
    width: 540
    height: 748

    // Picked-image and per-surface selection are ephemeral session
    // state — no persistence on purpose. selectedSurfaceIds is a JS
    // object keyed by surface id; toggleSurface swaps it whole so
    // QML re-evaluates bindings that depend on it.
    property url pickedImage: ""
    property var selectedSurfaceIds: ({})

    function toggleSurface(id, checked) {
        const updated = Object.assign({}, selectedSurfaceIds);
        if (checked) {
            updated[id] = true;
        } else {
            delete updated[id];
        }
        selectedSurfaceIds = updated;
    }

    function selectedIds() {
        return Object.keys(selectedSurfaceIds);
    }

    readonly property bool canApply: pickedImage !== ""
                                     && selectedIds().length > 0
                                     && !syncEngine.applying

    WallpaperPicker {
        id: wallpaperPicker
        onWallpaperPicked: (path) => {
            window.pickedImage = path;
        }
    }

    // Buffer per-surface failure reasons emitted during a batch so
    // applyFinished can surface them alongside the failed-ids list.
    property var failureReasons: ({})

    Connections {
        target: syncEngine
        function onSurfaceApplyFailed(id, reason) {
            const next = Object.assign({}, window.failureReasons);
            next[id] = reason;
            window.failureReasons = next;
        }
        function onApplyFinished(succeeded, failed) {
            const reasons = window.failureReasons;
            window.failureReasons = ({});

            if (succeeded.length === 0 && failed.length === 0) {
                return;
            }
            if (failed.length === 0) {
                feedback.type = Kirigami.MessageType.Positive;
                feedback.text = qsTr("Applied to: %1").arg(succeeded.join(", "));
            } else {
                const detailed = failed.map(id =>
                    reasons[id] ? `${id} (${reasons[id]})` : id).join(", ");
                if (succeeded.length === 0) {
                    feedback.type = Kirigami.MessageType.Error;
                    feedback.text = qsTr("Failed: %1").arg(detailed);
                } else {
                    feedback.type = Kirigami.MessageType.Warning;
                    feedback.text = qsTr("Applied to %1; failed on %2")
                        .arg(succeeded.join(", "))
                        .arg(detailed);
                }
            }
            feedback.visible = true;
        }
    }

    pageStack.initialPage: Kirigami.ScrollablePage {
        title: qsTr("Wallpaper surfaces")

        actions: [
            Kirigami.Action {
                text: qsTr("Choose wallpaper…")
                icon.name: "preferences-desktop-wallpaper"
                onTriggered: wallpaperPicker.show()
            },
            Kirigami.Action {
                text: qsTr("Apply")
                icon.name: "dialog-ok-apply"
                enabled: window.canApply
                onTriggered: syncEngine.applyToSurfaces(
                    window.pickedImage.toString(), window.selectedIds())
            }
        ]

        ColumnLayout {
            width: parent.width
            spacing: Kirigami.Units.largeSpacing

            Kirigami.InlineMessage {
                id: feedback
                Layout.fillWidth: true
                showCloseButton: true
                visible: false
            }

            // Picked-image banner — shown once the user has chosen a file.
            Pane {
                visible: window.pickedImage !== ""
                Layout.fillWidth: true

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    Rectangle {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 6
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 6 * 9 / 16
                        color: Kirigami.Theme.alternateBackgroundColor
                        radius: Kirigami.Units.smallSpacing
                        clip: true

                        Image {
                            anchors.fill: parent
                            sourceSize.width: 320
                            source: window.pickedImage
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Heading {
                            level: 4
                            text: qsTr("Selected wallpaper")
                        }

                        Label {
                            Layout.fillWidth: true
                            text: window.pickedImage.toString()
                            elide: Text.ElideMiddle
                            opacity: 0.7
                            font.family: "monospace"
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Kirigami.Units.largeSpacing
                rowSpacing: Kirigami.Units.largeSpacing

                Repeater {
                    model: syncEngine.surfaceIds

                    delegate: Pane {
                    id: card
                    required property string modelData
                    readonly property var surface: syncEngine.surface(modelData)

                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    Layout.alignment: Qt.AlignTop

                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing

                            Kirigami.Heading {
                                level: 3
                                text: card.surface ? card.surface.displayName : card.modelData
                            }

                            Kirigami.Icon {
                                source: "documentinfo-symbolic"
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                                opacity: 0.6
                                visible: card.surface && card.surface.description !== ""

                                HoverHandler { id: cardInfoHover }
                                ToolTip.visible: cardInfoHover.hovered
                                ToolTip.delay: Kirigami.Units.toolTipDelay
                                ToolTip.text: card.surface ? card.surface.description : ""
                            }

                            Item { Layout.fillWidth: true }

                            CheckBox {
                                text: qsTr("Update")
                                checked: !!window.selectedSurfaceIds[card.modelData]
                                onToggled: window.toggleSurface(card.modelData, checked)
                            }
                        }

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
                                sourceSize.width: 480
                                // previewImagePath equals currentImagePath
                                // for every surface except login, which
                                // returns a $USER-readable source URL when
                                // available (/var/lib/plasmalogin/wallpapers/
                                // is opaque from outside the plasmalogin user).
                                source: card.surface ? card.surface.previewImagePath : ""
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

                // Empty 4th slot — the session splash surface is
                // deliberately not yet implemented (Plasma's splash is
                // theme-based, not image-based; reconciling that with
                // the "one image, four surfaces" model is deferred).
                // The empty pane keeps the 2×2 grid visually balanced.
                Item {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    visible: syncEngine.surfaceIds.length < 4
                }
            }
        }
    }
}
