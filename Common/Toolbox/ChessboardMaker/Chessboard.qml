import QtQuick 2.5
import QtQuick.Window 2.0
import QtQuick.Dialogs 1.2
import "cbLogic.js" as Cb

Window {
    id: windowCB
    title: "Target"
    flags: Qt.Dialog
    minimumWidth: width
    minimumHeight: height
    maximumWidth: width
    maximumHeight: height

    property real ppm: Cb.ppm
    property bool screenSwitch : true

    Screen.onPixelDensityChanged: {
        ppm = Screen.pixelDensity
        setMinimumWidth(0)
        setMinimumHeight(0)
        setMaximumWidth(9999)
        setMaximumHeight(9999)
        setWidth((Cb.dimension.columns+1) * Cb.dimension.side * Screen.pixelDensity)
        setHeight((Cb.dimension.rows+1) * Cb.dimension.side * Screen.pixelDensity)
        setMinimumWidth(width)
        setMinimumHeight(height)
        setMaximumWidth(width)
        setMaximumHeight(height)
        mycanvas.requestPaint()
    }

//    Screen.onNameChanged:
//        title = Screen.name


    Canvas {
        anchors.fill: parent
        id: mycanvas
        renderTarget: Canvas.Image
        renderStrategy: Canvas.Immediate
        onPaint: {
            var ctx = getContext("2d")
            var size = Cb.drawChessboard(ctx, ppm)
        }
    }

    onScreenChanged:{
//        screenSwitch = true
//        title = Screen.name
    }

    onBeforeRendering: {
//        if (screenSwitch)
//        {
//            //title = Screen.name
//            setMinimumWidth(0)
//            setMinimumHeight(0)
//            setMaximumWidth(9999)
//            setMaximumHeight(9999)
//            setWidth(Cb.dimension.columns * Cb.dimension.side * Screen.pixelDensity)
//            setHeight(Cb.dimension.rows * Cb.dimension.side * Screen.pixelDensity)
//            setMinimumWidth(width)
//            setMinimumHeight(height)
//            setMaximumWidth(width)
//            setMaximumHeight(height)
//            screenSwitch = false
//        }
    }
}


