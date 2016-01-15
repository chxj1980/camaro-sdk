import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.2
import QtWebSockets 1.0

import com.dg.rcserver 1.0
import com.dg.geometryhelper 1.0

import "cbLogic.js" as Cb

Window {
    id: root
    width: 600
    height: 400
    minimumWidth: width
    minimumHeight: height
    maximumWidth: width
    maximumHeight: height
    title:  "Chessboard Maker"
    visible: true

    property var component : Qt.createComponent("Chessboard.qml")
    property var mywindow
    property bool screenSwitch : true

    function generateCB()
    {
        if (typeof mywindow != "undefined")
            mywindow.close()
        mywindow = component.createObject(root)
        mywindow.width = (Cb.dimension.columns+1) * Cb.dimension.side * Screen.pixelDensity
        mywindow.height = (Cb.dimension.rows+1) * Cb.dimension.side * Screen.pixelDensity
        var pos = ghelper.getOrigin(Screen.name)
        mywindow.setX(pos.x + (Screen.width-mywindow.width)/2)
        mywindow.setY(pos.y + (Screen.height-mywindow.height)/2)
        if (mywindow.width>Screen.width || mywindow.height>Screen.height)
            mydialog.open()
        else
        {
            mydialog.close()
            invaliddialog.close()
            mywindow.show()
        }
    }

    RCServer
    {
        id: server
        onMessageReceived: {
            Cb.dimension = JSON.parse(message);
            sliderW.value = Cb.dimension.columns - 1;
            sliderH.value = Cb.dimension.rows - 1;
            textSide.text = Cb.dimension.side.toString();
            generateCB();
        }
    }

    GeometryHelper
    {
        id: ghelper
    }

    MainForm {
        id: mainForm1
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.fill: parent
        mouseArea.onClicked: {
            socket.sendTextMessage(qsTr("Hello Server!"));
        }

        Row {
            id: row1
            x: 170
            spacing: 50
            y: 314
            width: 380
            height: 42
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.right: parent.right
            anchors.rightMargin: 60

            Row {
                width: 185
                anchors.verticalCenter: parent.verticalCenter
                spacing: 5
                Label {
                    id: label3
                    width: 140
                    text: qsTr("Remote Control")
                    horizontalAlignment: Text.AlignHCenter
                }
                Switch {
                    checked: false
                    onCheckedChanged: {
                        if (checked)
                            server.start(10080)
                        else
                            server.stop()
                    }
                }
            }

            Button {
                id: buttonCreate
                x: 11
                y: 10
                text: qsTr("Create")
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        GroupBox {
            id: groupBox1
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 110
            anchors.right: parent.right
            anchors.rightMargin: 60
            anchors.left: parent.left
            anchors.leftMargin: 60
            anchors.top: parent.top
            anchors.topMargin: 80
            title: qsTr("Chessboard")

            ColumnLayout {
                id: columnLayout1
                anchors.bottomMargin: 10
                anchors.topMargin: 10
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.top: parent.top

                RowLayout {
                    id: rowLayout1
                    width: 100
                    height: 100
                    spacing: 20

                    Label {
                        id: label2
                        text: qsTr("Columns")
                        Layout.preferredWidth: 100
                        horizontalAlignment: Text.AlignRight
                    }

                    Slider {
                        id: sliderW
                        width: 250
                        Layout.preferredWidth: 250
                        tickmarksEnabled: true
                        minimumValue: 2
                        value: 9
                        stepSize: 1
                        maximumValue: 20
                        onValueChanged: textW.text = value.toString()
                    }

                    TextField {
                        id: textW
                        width: 40
                        text: "9"
                        Layout.preferredWidth: 40
                        readOnly: true
                    }
                }

                RowLayout {
                    id: rowLayout2
                    width: 100
                    height: 100
                    spacing: 20

                    Label {
                        id: label10
                        text: qsTr("Rows")
                        Layout.preferredWidth: 100
                        horizontalAlignment: Text.AlignRight
                    }

                    Slider {
                        id: sliderH
                        width: 250
                        Layout.preferredWidth: 250
                        value: 6
                        minimumValue: 2
                        stepSize: 1
                        maximumValue: 20
                        tickmarksEnabled: true
                        onValueChanged: textH.text = value.toString()
                    }

                    TextField {
                        id: textH
                        width: 40
                        text: "6"
                        Layout.preferredWidth: 40
                        readOnly: true
                    }
                }

                RowLayout {
                    id: rowLayout3
                    width: 100
                    height: 100
                    spacing: 20

                    Label {
                        id: label4
                        width: 84
                        height: 16
                        text: qsTr("Side Length")
                        Layout.preferredWidth: 120
                        horizontalAlignment: Text.AlignRight
                    }

                    TextField {
                        id: textSide
                        width: 40
                        placeholderText: "0"
                        Layout.preferredWidth: 60
                    }

                    Label {
                        id: label5
                        text: qsTr("MM")
                    }
                }
            }

        }

        RowLayout {
            id: rowLayout4
            width: 257
            height: 26
            anchors.left: parent.left
            anchors.leftMargin: 60
            anchors.top: parent.top
            anchors.topMargin: 40
            spacing: 20
            transformOrigin: Item.Center

            Label {
                id: label1
                width: 128
                height: 16
                text: qsTr("Display Diagonal: ")
                Layout.preferredWidth: 130
            }

            Label {
                id: labelDL
                width: 60
                text: "0 in"
                Layout.preferredWidth: 80
                horizontalAlignment: Text.AlignLeft
            }
        }
    }

    Component.onCompleted: {
        //        root.setX(50)
        //        root.setY(50)
    }

    onScreenChanged: screenSwitch = true

    onBeforeRendering: {
        if (screenSwitch)
        {
            Cb.ppm = Screen.pixelDensity
            var size = Math.sqrt(Screen.width*Screen.width+Screen.height*Screen.height)/(Screen.pixelDensity*25.4)
            labelDL.text = size.toFixed(1)+" in"
            screenSwitch = false
        }
    }

    MessageDialog {
        id: mydialog
        title: "Warning"
        text: "Size of the generated chessboard is too large!\n Its resolution is out of screen!"
        icon: StandardIcon.Warning
        standardButtons: StandardButton.Close
    }

    MessageDialog {
        id: invaliddialog
        title: "Warning"
        text: "Side length of square is invalid!"
        icon: StandardIcon.Warning
        standardButtons: StandardButton.Close
    }

    Connections {
        target: buttonCreate
        onClicked: {
            Cb.dimension.columns = parseInt(textW.text, 10) + 1
            Cb.dimension.rows = parseInt(textH.text, 10) + 1
            Cb.dimension.side = parseInt(textSide.text, 10)
            if (isNaN(Cb.dimension.side) || Cb.dimension.side<1)
            {
                invaliddialog.open()
                return
            }
            generateCB()
        }
    }
}
