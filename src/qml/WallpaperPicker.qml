// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami

// Plain QtQuick.Window rather than Kirigami.Dialog so the user can
// resize the picker freely. Kirigami.Dialog is fixed-size by design
// (intended for short-form prompts); a long-form grid like this one
// outgrows that shape as soon as the wallpaper library is non-trivial.
// transientParent + WindowModal keeps the picker tied to the main
// application window — focus returns there on close, and the WM
// treats the picker as a modal of the parent rather than a peer.
Window {
    id: pickerWindow

    title: qsTr("Choose a wallpaper")
    width: Kirigami.Units.gridUnit * 48
    height: Kirigami.Units.gridUnit * 32
    minimumWidth: Kirigami.Units.gridUnit * 24
    minimumHeight: Kirigami.Units.gridUnit * 16

    modality: Qt.WindowModal
    transientParent: applicationWindow()
    color: Kirigami.Theme.backgroundColor

    signal wallpaperPicked(url path)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.smallSpacing

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: Kirigami.Units.gridUnit * 10
            cellHeight: Kirigami.Units.gridUnit * 7

            model: wallpaperLibrary

            delegate: ItemDelegate {
                id: tile
                width: GridView.view.cellWidth - Kirigami.Units.smallSpacing
                height: GridView.view.cellHeight - Kirigami.Units.smallSpacing
                clip: true

                required property string name
                required property url previewPath
                required property url applyPath

                onClicked: {
                    pickerWindow.wallpaperPicked(tile.applyPath);
                    pickerWindow.close();
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Kirigami.Theme.alternateBackgroundColor
                        radius: Kirigami.Units.smallSpacing
                        clip: true

                        Image {
                            anchors.fill: parent
                            sourceSize.width: 320
                            source: tile.previewPath
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: tile.name
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Browse…")
                icon.name: "document-open"
                onClicked: customFilePicker.open()
            }
            Button {
                text: qsTr("Cancel")
                icon.name: "dialog-cancel"
                onClicked: pickerWindow.close()
            }
        }
    }

    FileDialog {
        id: customFilePicker
        title: qsTr("Pick a wallpaper image")
        nameFilters: [qsTr("Images (*.jpg *.jpeg *.png *.webp *.bmp)")]
        // Bypass the XDG FileChooser portal so we get real host paths,
        // not /run/user/.../doc/<token>/ URLs that Plasma can't read.
        options: FileDialog.DontUseNativeDialog
        onAccepted: {
            pickerWindow.wallpaperPicked(selectedFile);
            pickerWindow.close();
        }
    }
}
