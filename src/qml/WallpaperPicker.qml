// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: pickerDialog

    title: qsTr("Choose a wallpaper")
    width: Math.min(applicationWindow().width * 0.9, Kirigami.Units.gridUnit * 50)
    height: Math.min(applicationWindow().height * 0.9, Kirigami.Units.gridUnit * 36)

    signal wallpaperPicked(url path)

    standardButtons: Kirigami.Dialog.Cancel

    customFooterActions: [
        Kirigami.Action {
            text: qsTr("Browse…")
            icon.name: "document-open"
            onTriggered: customFilePicker.open()
        }
    ]

    GridView {
        id: grid
        anchors.fill: parent
        clip: true
        cellWidth: Kirigami.Units.gridUnit * 10
        cellHeight: Kirigami.Units.gridUnit * 7

        model: wallpaperLibrary

        delegate: ItemDelegate {
            id: tile
            width: GridView.view.cellWidth - Kirigami.Units.smallSpacing
            height: GridView.view.cellHeight - Kirigami.Units.smallSpacing

            required property string name
            required property url previewPath
            required property url applyPath

            onClicked: {
                pickerDialog.wallpaperPicked(tile.applyPath);
                pickerDialog.close();
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

    FileDialog {
        id: customFilePicker
        title: qsTr("Pick a wallpaper image")
        nameFilters: [qsTr("Images (*.jpg *.jpeg *.png *.webp *.bmp)")]
        // Bypass the XDG FileChooser portal so we get real host paths,
        // not /run/user/.../doc/<token>/ URLs that Plasma can't read.
        options: FileDialog.DontUseNativeDialog
        onAccepted: {
            pickerDialog.wallpaperPicked(selectedFile);
            pickerDialog.close();
        }
    }
}
