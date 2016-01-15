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
        anchors.fill: parent
        mouseArea.onClicked: {
            socket.sendTextMessage(qsTr("Hello Server!"));
        }

        Row {
            id: row1
            spacing: 50
            x: 170
            y: 295
            width: 354
            height: 42

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 5
                Label {
                    id: label3
                    width: 120
                    text: qsTr("Remote Control")
                    horizontalAlignment: Text.AlignRight
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
            x: 61
            y: 87
            width: 463
            height: 181
            title: qsTr("Chessboard")

            Label {
                id: label10
                x: 44
                y: 74
                text: qsTr("Rows")
                horizontalAlignment: Text.AlignRight
            }

            TextField {
                id: textSide
                x: 97
                y: 103
                width: 40
                placeholderText: qsTr("0")
            }

            Label {
                id: label4
                x: 0
                y: 105
                width: 84
                height: 16
                text: qsTr("Side Length")
                horizontalAlignment: Text.AlignRight
            }

            Label {
                id: label2
                x: 26
                y: 41
                text: qsTr("Columns")
                horizontalAlignment: Text.AlignRight
            }

            TextField {
                id: textH
                x: 358
                y: 74
                width: 40
                readOnly: true
                placeholderText: qsTr("0")
            }

            TextField {
                id: textW
                x: 358
                y: 41
                width: 40
                readOnly: true
                placeholderText: qsTr("0")
            }

            Label {
                id: label5
                x: 158
                y: 105
                text: qsTr("MM")
            }

            Slider {
                id: sliderW
                x: 97
                y: 38
                width: 250
                tickmarksEnabled: true
                minimumValue: 2
                value: 9
                stepSize: 1
                maximumValue: 20
                onValueChanged: textW.text = value.toString()
            }

            Slider {
                id: sliderH
                x: 97
                y: 71
                width: 250
                value: 6
                minimumValue: 2
                stepSize: 1
                maximumValue: 20
                tickmarksEnabled: true
                onValueChanged: textH.text = value.toString()
            }

        }

        Label {
            id: label1
            x: 61
            y: 55
            width: 128
            height: 16
            text: qsTr("Display Diagonal: ")
        }

        Label {
            id: labelDL
            x: 195
            y: 55
            width: 60
            text: "0 in"
            horizontalAlignment: Text.AlignHCenter
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
