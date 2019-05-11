import {Renderable} from "./Renderable";
import {ProgramInfo} from "./GLView";

export class Triangles extends Renderable {
    vertexBuffer: WebGLBuffer;
    indexBuffer: WebGLBuffer;
    public color: Float32Array = new Float32Array([0,0,0,1]);

    constructor(private vertices: Float32Array, public indices: Uint16Array, public center?: Float32Array, public dimensions?: Float32Array) {
        super();
    }

    allocate(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        super.allocate(gl, programInfo);

        if (!this.vertexBuffer)
            this.vertexBuffer = gl.createBuffer();

        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, this.vertices, gl.STATIC_DRAW);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);

        if (this.indices) {
            if (!this.indexBuffer)
                this.indexBuffer = gl.createBuffer();

            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, this.indices, gl.STATIC_DRAW);
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
        }
    }

    deallocate(gl: WebGLRenderingContext) {
        if (this.vertexBuffer)
            gl.deleteBuffer(this.vertexBuffer);

        if (this.indexBuffer)
            gl.deleteBuffer(this.indexBuffer);
    }

    render(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        super.render(gl, programInfo);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);

        if (this.indexBuffer)
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);

        gl.enableVertexAttribArray(programInfo.vertexPosition);
        gl.vertexAttribPointer(programInfo.vertexPosition, 3, gl.FLOAT, false, 0, 0);

        gl.vertexAttrib4fv(programInfo.vertexColor, this.color);

        if (this.indexBuffer)
            gl.drawElements(gl.TRIANGLES, this.indices.length, gl.UNSIGNED_SHORT, 0);
        else
            gl.drawArrays(gl.TRIANGLES, 0, this.vertices.length/3);

        gl.bindBuffer(gl.ARRAY_BUFFER, null);

        if (this.indexBuffer)
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
    }

    static createPlane() : Triangles {
        let vertices: number[] = [
            -0.5, 0, 0.5,
            -0.5, 0, -0.5,
            0.5, 0, -0.5,
            0.5, 0, 0.5,
        ];

        let indices: number[] = [
            0, 1, 2,
            0, 2, 3
        ];

        return new Triangles(new Float32Array(vertices), new Uint16Array(indices));
    }

}
