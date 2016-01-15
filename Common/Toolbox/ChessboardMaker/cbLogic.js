//Script
.pragma library

var ppm = 0.0

var dimension = {
    columns : 0,
    rows : 0,
    side : 0
};

function drawChessboard(ctx, ppm) {
    var side = dimension.side * ppm;
    var size = {
        width : side*dimension.columns,
        height : side*dimension.rows
    }
    ctx.clearRect(0,0,size.width+side,size.height+side)
    ctx.fillStyle = ctx.createPattern("black", Qt.SolidPattern);
    for(var i=0; i<dimension.rows; ++i){
        var offsetY = i*side + side/2
        for(var j=0; j<dimension.columns; ++j) {
            var offsetX = j*side + side/2
            if (i%2 != j%2)
                continue
            ctx.fillRect(offsetX, offsetY, side, side)
        }
    }
    return size
}
