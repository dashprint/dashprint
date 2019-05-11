import {Renderable} from "./Renderable";
import {ProgramInfo} from "./GLView";

export class Lines extends Renderable {
    vertexBuffer: WebGLBuffer;
    indexBuffer: WebGLBuffer;
    colorBuffer: WebGLBuffer;

    constructor(private vertices: number[], private colors: number[], private width = 1.0, private indices?: Uint16Array) {
        super();
    }

    // This can be called only before adding the object into the scene
    public addLine(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number, color1?: number[], color2?: number[]) {
        this.vertices.push(x1, y1, z1, x2, y2, z2);

        if (color1) {
            Array.prototype.push.apply(this.colors, color1);

            if (color2)
                Array.prototype.push.apply(this.colors, color2);
            else
                Array.prototype.push.apply(this.colors, color1);
        }
    }

    public allocate(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        super.allocate(gl, programInfo);

        if (!this.vertexBuffer)
            this.vertexBuffer = gl.createBuffer();

        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.vertices), gl.STATIC_DRAW);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);

        if (this.indices) {
            if (!this.indexBuffer)
                this.indexBuffer = gl.createBuffer();

            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, this.indices, gl.STATIC_DRAW);
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);
        }

        if (this.colors.length > 4) {
            if (!this.colorBuffer)
                this.colorBuffer = gl.createBuffer();

            gl.bindBuffer(gl.ARRAY_BUFFER, this.colorBuffer);
            gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.colors), gl.STATIC_DRAW);
            gl.bindBuffer(gl.ARRAY_BUFFER, null);
        }
    }

    public deallocate(gl: WebGLRenderingContext) {
        if (this.vertexBuffer)
            gl.deleteBuffer(this.vertexBuffer);
        if (this.indexBuffer)
            gl.deleteBuffer(this.indexBuffer);
        if (this.colorBuffer)
            gl.deleteBuffer(this.colorBuffer);
    }

    public render(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        super.render(gl, programInfo);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);

        if (this.indexBuffer)
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);

        gl.enableVertexAttribArray(programInfo.vertexPosition);
        gl.vertexAttribPointer(programInfo.vertexPosition, 3, gl.FLOAT, false, 0, 0);

        if (this.colors.length > 4) {

            gl.bindBuffer(gl.ARRAY_BUFFER, this.colorBuffer);
            gl.vertexAttribPointer(programInfo.vertexColor, 4, gl.FLOAT, false, 0, 0);
            gl.enableVertexAttribArray(programInfo.vertexColor);
        } else if (this.colors.length == 4) {
            gl.vertexAttrib4fv(programInfo.vertexColor, new Float32Array(this.colors));
        }

        gl.lineWidth(this.width);

        if (this.indexBuffer)
            gl.drawElements(gl.LINES, this.indices.length, gl.UNSIGNED_SHORT, 0);
        else
            gl.drawArrays(gl.LINES, 0, this.vertices.length / 3);

        gl.bindBuffer(gl.ARRAY_BUFFER, null);
        if (this.indexBuffer)
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);

        gl.disableVertexAttribArray(programInfo.vertexPosition);
        gl.disableVertexAttribArray(programInfo.vertexColor);
    }
}
