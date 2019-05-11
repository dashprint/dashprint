
function updateMinMax(point, pointOffset, min, max) {
    for (let i = 0; i < 3; i++) {
        if (point[pointOffset+i] < min[i])
            min[i] = point[pointOffset+i];
        if (point[pointOffset+i] > max[i])
            max[i] = point[pointOffset+i];
    }
}

function parseVertex(dataView, offset, target, targetOffset, min, max) {
    target[targetOffset] = dataView.getFloat32(offset, true);

    // Swap Y/Z
    target[targetOffset+2] = dataView.getFloat32(offset+4, true);
    target[targetOffset+1] = dataView.getFloat32(offset+8, true);

    if (min && max) {
        updateMinMax(target, targetOffset, min, max);
    }
}

function createResult(vertices, min, max) {
    var result = {};
    result.vertices = vertices;
    result.center = new Float32Array([
        (min[0]+max[0])/2,
        (min[1]+max[1])/2,
        (min[2]+max[2])/2
    ]);
    result.dimensions = new Float32Array([
        max[0]-min[0],
        max[1]-min[1],
        max[2]-min[2]
    ]);

    return result;
}

function parseBinarySTL(data) {
    var dataView = new DataView(data);
    var triangleCount = dataView.getUint32(80, true);

    // TODO: Check that triangleCount looks valid

    var vertices = new Float32Array(triangleCount*3*3);
    var min = new Float32Array(3);
    var max = new Float32Array(3);

    for (let i = 0; i < triangleCount; i++) {
        let offset = 84 + i*(12*4+2);

        for (let j = 0; j < 3; j++)
            parseVertex(dataView, offset + (j + 1) * 12, vertices, i*9+j*3, min, max);
    }

    return createResult(vertices, min, max);
}

function parseTextVertex(text, min, max) {
    var values = text.split(/\s+/);
    if (values.length == 3) {
        var array = new Float32Array([
            parseFloat(values[0]),
            parseFloat(values[1]),
            parseFloat(values[2])
        ]);

        if (min && max)
            updateMinMax(array, min, max);

        return array;
    }
    return null;
}

function parseTextSTL(data) {
    var decoder = new TextDecoder('ascii');
    var vertices = [];
    var triangle = null;
    var min = new Float32Array(3);
    var max = new Float32Array(3);

    var lines = decoder.decode(new Uint8Array(data)).split('\n');

    for (let i = 1; i < lines.length; i++) {
        let line = lines[i].trim();

        if (line.startsWith("outer loop")) {
            // noop
        } else if (line.startsWith("facet normal ")) {
            // triangle = {};
            // triangle.vertices = [];
            // triangle.normalVector = parseTextVertex(line.substr(13).trim());
        } else if (line.startsWith("endfacet")) {
            // triangles.push(triangle);
            // triangle = null;
        } else if (line.startsWith("vertex ")) {
            triangle.vertices.push(parseTextVertex(line.substr(7).trim(), min, max));
        } else if (line.startsWith("endloop")) {
            if (triangle.vertices.length == 3) {
                Array.prototype.push.apply(vertices, triangle.vertices[0]);
                Array.prototype.push.apply(vertices, triangle.vertices[1]);
                Array.prototype.push.apply(vertices, triangle.vertices[2]);
            }
            delete triangle.vertices;
        } else if (line.startsWith("endsolid")) {
            return createResult(new Float32Array(vertices), min, max);;
        } else {
            console.warn("Unknown STL line: " + line);
        }
    }

    return [];
}

function parseSTL(data) {
    console.log("parseSTL called");
    var triangles;
    var decoder = new TextDecoder('ascii');

    if (decoder.decode(new Uint8Array(data, 0, 5)) == "solid")
        triangles = parseTextSTL(data);
    else
        triangles = parseBinarySTL(data);

    postMessage(triangles);
}


addEventListener('message', (message) => {
    // message.data is ArrayBuffer
    // the response is expected to be Triangle[]

    parseSTL(message.data);
});
