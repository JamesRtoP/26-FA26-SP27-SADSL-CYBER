import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine

ApplicationWindow {
    id: window

    width: 1400
    height: 850
    minimumWidth: 1050
    minimumHeight: 650
    visible: true

    title: "Post Quantum Chat"
    color: "#0f1115"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.preferredWidth: 180
            Layout.fillHeight: true

            color: "#15181d"

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: "#272b32"
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 0

                Label {
                    text: "PQ3 Chat"
                    color: "#f2f3f5"

                    font.family: "Segoe UI"
                    font.pixelSize: 18
                    font.weight: Font.DemiBold

                    Layout.leftMargin: 8
                    Layout.topMargin: 8
                    Layout.bottomMargin: 32
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    radius: 5
                    color: "#2b6de0"

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter

                        text: "Metrics"
                        color: "white"

                        font.family: "Segoe UI"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }

        // Main area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "#0f1115"

            ColumnLayout {
                anchors.fill: parent

                anchors.leftMargin: 24
                anchors.rightMargin: 24
                anchors.topMargin: 22
                anchors.bottomMargin: 24

                spacing: 16

                ColumnLayout {
                    spacing: 3

                    Label {
                        text: "System metrics"
                        color: "#f2f3f5"

                        font.family: "Segoe UI"
                        font.pixelSize: 23
                        font.weight: Font.DemiBold
                    }

                    Label {
                        text: "Last 15 minutes"
                        color: "#949ba4"

                        font.family: "Segoe UI"
                        font.pixelSize: 13
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    radius: 6
                    color: "#ffffff"

                    border.width: 1
                    border.color: "#2b3038"

                    clip: true

                    WebEngineView {
                        id: kibanaView

                        anchors.fill: parent

                        url: "http://localhost:5601/app/dashboards#/view/24468e7f-53b1-4ac2-b06c-d3c4eac92492?embed=true&_g=%28refreshInterval%3A%28pause%3A%21f%2Cvalue%3A10000%29%2Ctime%3A%28from%3Anow-15m%2Cto%3Anow%29%29&hide-filter-bar=true"
                    }
                }
            }
        }
    }
}