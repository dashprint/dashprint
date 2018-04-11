import {Renderable} from "./Renderable";
import {ProgramInfo} from "./GLView";

export class Triangles extends Renderable {
    vertexBuffer: WebGLBuffer;
    public color: Float32Array = new Float32Array([0,0,0,1]);

    constructor(private vertices: Float32Array, public center: Float32Array, public dimensions: Float32Array) {
        super();
    }

    allocate(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        super.allocate(gl, programInfo);

        this.vertexBuffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, this.vertices, gl.STATIC_DRAW);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);
    }

    render(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        super.render(gl, programInfo);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);

        gl.enableVertexAttribArray(programInfo.vertexPosition);
        gl.vertexAttribPointer(programInfo.vertexPosition, 3, gl.FLOAT, false, 0, 0);

        gl.vertexAttrib4fv(programInfo.vertexColor, this.color);
        gl.drawArrays(gl.TRIANGLES, 0, this.vertices.length/3);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);
    }

}
