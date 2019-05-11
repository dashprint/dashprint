import * as mat4 from "gl-matrix-mat4";
import {ProgramInfo} from "./GLView";
import {Camera} from "./Camera";

export class Renderable {
    _position: Float32Array = new Float32Array([0,0,0]);
    _scale: Float32Array = new Float32Array([1,1,1]);
    _rotation: Float32Array = new Float32Array([0,0,0]);
    matrix: Float32Array;

    constructor() {
    }

    public allocate(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        this.updateMatrix();
    }

    public render(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        gl.uniformMatrix4fv(programInfo.modelViewMatrix, false, this.matrix);
    }

    public deallocate(gl: WebGLRenderingContext) {
    }

    updateMatrix() {
        this.matrix = mat4.create();
        mat4.translate(this.matrix, this.matrix, this._position);
        mat4.rotateX(this.matrix, this.matrix, this._rotation[0]);
        mat4.rotateY(this.matrix, this.matrix, this._rotation[1]);
        mat4.rotateZ(this.matrix, this.matrix, this._rotation[2]);
        mat4.scale(this.matrix, this.matrix, this._scale);
    }

    get position(): Float32Array {
        return this._position;
    }

    set position(pos: Float32Array) {
        this._position = pos;
        this.updateMatrix();
    }

    get scale(): Float32Array {
        return this._scale;
    }

    setScale(scale: Float32Array|number) {
        if (typeof scale === "object")
            this._scale = <Float32Array>scale;
        else
            this._scale.fill(<number> scale);
        this.updateMatrix();
    }

    get rotation() : Float32Array {
        return this._rotation;
    }

    set rotation(rot: Float32Array) {
        this._rotation = rot;
        this.updateMatrix();
    }

    public shouldRender(camera: Camera): boolean {
        return true;
    }
}
