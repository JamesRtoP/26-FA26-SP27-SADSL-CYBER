pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 900
    height: 600
    minimumWidth: 560
    minimumHeight: 380
    visible: true
    title: "Q3-Chat Homeserver"
    color: root.chatBackground
    font.family: "Segoe UI"

    // Session preference. Dark by default, matching the client.
    property bool lightMode: false

    // Shared application colors, the same palette the client uses:
    // light theme first, dark theme second.
    readonly property color chatBackground:
        lightMode ? "#FFFFFF" : "#0D0F12"

    readonly property color panel:
        lightMode ? "#F4F6F9" : "#111820"

    readonly property color surface:
        lightMode ? "#FFFFFF" : "#18222C"

    readonly property color headerColor:
        lightMode ? "#F4F6F9" : "#101820"

    readonly property color foreground:
        lightMode ? "#17212B" : "#E8EEF5"

    readonly property color muted:
        lightMode ? "#526477" : "#9AAEBF"

    readonly property color divider:
        lightMode ? "#D5DEE8" : "#304452"

    readonly property color accent:
        lightMode ? "#096DAA" : "#55BDFF"

    readonly property color accentHover:
        lightMode ? "#075C91" : "#7ACBFF"

    readonly property color accentPressed:
        lightMode ? "#064E7B" : "#329FDE"

    readonly property color onAccent:
        lightMode ? "#FFFFFF" : "#07131D"

    readonly property color hoverSurface:
        lightMode ? "#E7EDF4" : "#243441"

    readonly property color pressedSurface:
        lightMode ? "#D7E2ED" : "#304452"

    readonly property color selection:
        lightMode ? "#096DAA" : "#245F86"

    readonly property color selectedText: "#FFFFFF"

    // Colors inherited by built-in Qt controls.
    palette.window: root.panel
    palette.windowText: root.foreground
    palette.base: root.chatBackground
    palette.text: root.foreground
    palette.button: root.surface
    palette.buttonText: root.foreground
    palette.highlight: root.selection
    palette.highlightedText: root.selectedText

    // Log entries shown in the log panel. Nothing writes to it yet. To add a
    // line later, call: logModel.append({ message: "text" })
    ListModel {
        id: logModel
    }

    component AppButton: Button {
        id: control
        property bool primary: false

        implicitHeight: 36
        implicitWidth: Math.max(72, contentItem.implicitWidth + 28)
        hoverEnabled: true

        contentItem: Text {
            text: control.text
            color: control.primary ? root.onAccent : root.foreground
            font.family: "Consolas"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 6

            color: control.primary
                   ? (control.down ? root.accentPressed
                      : control.hovered ? root.accentHover
                      : root.accent)
                   : (control.down ? root.pressedSurface
                      : control.hovered ? root.hoverSurface
                      : root.surface)

            border.color: control.activeFocus
                          ? root.foreground
                          : control.primary ? root.accent : root.divider
            border.width: control.activeFocus ? 2 : 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Brand header, styled like the client's sidebar header.
        Rectangle {
            id: header
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            color: root.headerColor

            gradient: Gradient {
                orientation: Gradient.Horizontal

                GradientStop {
                    position: 0.0
                    color: root.lightMode ? root.headerColor : "#122230"
                }

                GradientStop {
                    position: 0.55
                    color: root.lightMode ? root.headerColor : "#0D171F"
                }

                GradientStop {
                    position: 1.0
                    color: root.lightMode ? root.headerColor : "#101922"
                }
            }

            // Static constellation background. It only redraws when the
            // header is resized or the theme changes.
            Canvas {
                id: headerStars
                anchors.fill: parent
                opacity: root.lightMode ? 0.75 : 0.55

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()

                Connections {
                    target: root

                    function onLightModeChanged() {
                        headerStars.requestPaint()
                    }
                }

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    const points = [
                        [0.08, 0.22],
                        [0.24, 0.70],
                        [0.43, 0.28],
                        [0.61, 0.62],
                        [0.78, 0.18],
                        [0.93, 0.54]
                    ]

                    const links = [
                        [0, 2],
                        [1, 2],
                        [2, 3],
                        [3, 4],
                        [3, 5]
                    ]

                    ctx.lineWidth = 1
                    ctx.strokeStyle = root.lightMode ? "#475569" : "#23506A"

                    for (let i = 0; i < links.length; ++i) {
                        const first = points[links[i][0]]
                        const second = points[links[i][1]]

                        ctx.beginPath()
                        ctx.moveTo(first[0] * width, first[1] * height)
                        ctx.lineTo(second[0] * width, second[1] * height)
                        ctx.stroke()
                    }

                    ctx.fillStyle = root.lightMode ? "#334155" : "#3A7597"

                    for (let j = 0; j < points.length; ++j) {
                        ctx.beginPath()
                        ctx.arc(points[j][0] * width,
                                points[j][1] * height,
                                j === 2 ? 2 : 1.4,
                                0,
                                Math.PI * 2)
                        ctx.fill()
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 22
                anchors.rightMargin: 22
                spacing: 14

                // Q3 badge.
                Rectangle {
                    Layout.preferredWidth: 42
                    Layout.preferredHeight: 42
                    radius: 8
                    color: root.accent

                    Text {
                        anchors.centerIn: parent
                        text: "Q3"
                        color: root.onAccent
                        font.family: "Consolas"
                        font.pixelSize: 17
                        font.weight: Font.Bold
                    }
                }

                // Breadcrumb: Q3-Chat / Homeserver.
                RowLayout {
                    spacing: 9

                    Text {
                        text: "Q3-Chat"
                        color: root.muted
                        font.family: "Consolas"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }

                    Text {
                        text: "/"
                        color: root.muted
                        font.family: "Consolas"
                        font.pixelSize: 12
                        font.weight: Font.Bold
                    }

                    Text {
                        text: "Homeserver"
                        color: root.foreground
                        font.family: "Consolas"
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                // Theme switch.
                AppButton {
                    text: "Dark"
                    primary: !root.lightMode
                    onClicked: root.lightMode = false
                }

                AppButton {
                    text: "Light"
                    primary: root.lightMode
                    onClicked: root.lightMode = true
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: root.divider
            }
        }

        // Main area.
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: root.chatBackground

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 28
                spacing: 16

                // Section heading, same style as the client's sign-in divider.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.divider
                    }

                    Text {
                        text: "SERVER LOG"
                        color: root.muted
                        font.family: "Consolas"
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        font.letterSpacing: 2
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.divider
                    }
                }

                // Log panel.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 8
                    color: root.surface
                    border.color: root.divider
                    border.width: 1

                    ListView {
                        id: logList
                        anchors.fill: parent
                        anchors.margins: 14
                        clip: true
                        spacing: 4
                        model: logModel

                        ScrollBar.vertical: ScrollBar {}

                        delegate: Text {
                            required property string message

                            width: ListView.view.width
                            text: message
                            textFormat: Text.PlainText
                            color: root.foreground
                            font.family: "Consolas"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }
                    }

                    // Empty state, shown until the first log line is added.
                    Text {
                        anchors.centerIn: parent
                        visible: logList.count === 0
                        text: "No log entries yet"
                        color: root.muted
                        font.family: "Consolas"
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "WSU DATA SCIENCE LAB"
                    color: root.muted
                    font.family: "Consolas"
                    font.pixelSize: 9
                    font.letterSpacing: 1.4
                }
            }
        }
    }
}
