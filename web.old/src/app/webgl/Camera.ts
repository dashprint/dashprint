import * as mat4 from "gl-matrix-mat4";
import * as vec3 from "gl-matrix-vec3";
import {ProgramInfo} from "./GLView";

export class Camera {
    private _lookAt: Float32Array = new Float32Array([0, 0, 0]);
    // private position: Float32Array = new Float32Array([0, 5, 5]);
    private _xAngle: number = 0;
    private _yAngle: number = 0;
    private _distance: number = 5;
    private up: Float32Array = new Float32Array([0, 1, 0]);
    private matrix: Float32Array;
    private mousePressed: boolean = false;

    public onCameraChange: () => void;
    public cameraMoveButton: number = 2;

    constructor(private canvas?: HTMLCanvasElement) {
        this.updateMatrix();

        if (canvas) {
            canvas.addEventListener('wheel', this.handleScroll.bind(this), true);

            canvas.addEventListener('mousedown', this.handleMouseDown.bind(this), true);
            canvas.addEventListener('mouseup', this.handleMouseUp.bind(this), true);
            canvas.addEventListener('mousemove', this.handleMouseMove.bind(this));
            canvas.addEventListener('mouseleave', this.handleMouseLeave.bind(this));
            canvas.addEventListener('contextmenu', (e) => e.preventDefault());
        }
    }

    apply(gl: WebGLRenderingContext, programInfo: ProgramInfo) {
        gl.uniformMatrix4fv(
            programInfo.cameraMatrix,
            false,
            this.matrix);
    }

    private updateMatrix() {
        let vec = vec3.create();

        vec[2] = this._distance;
        vec3.rotateX(vec, vec, this._lookAt, -this._yAngle * Math.PI / 180);
        vec3.rotateY(vec, vec, this._lookAt, -this._xAngle * Math.PI / 180);

        let hadMatrix = this.matrix;
        this.matrix = mat4.create();
        mat4.lookAt(this.matrix, vec, this._lookAt, this.up);

        if (hadMatrix && this.onCameraChange)
            this.onCameraChange();
    }

    get distance(): number {
        return this._distance;
    }

    set distance(distance: number) {
        this._distance = distance;
        this.updateMatrix();
    }

    get xAngle() : number {
        return this._xAngle;
    }

    set xAngle(xAngle: number) {
        this._xAngle = xAngle % 360;
        this.updateMatrix();
    }

    get yAngle() : number {
        return this._yAngle;
    }

    set yAngle(yAngle: number) {
        this._yAngle = yAngle % 360;
        this.updateMatrix();
    }

    get lookAt() : Float32Array {
        return this._lookAt;
    }

    set lookAt(pos: Float32Array) {
        this._lookAt = pos;
        this.updateMatrix();
    }

    private handleScroll(e: WheelEvent) {
        var delta;

        if (e.deltaMode === WheelEvent.DOM_DELTA_PIXEL)
            delta = e.deltaY;
        else if (e.deltaMode === WheelEvent.DOM_DELTA_LINE)
            delta = e.deltaY * 14;
        else if (e.deltaMode === WheelEvent.DOM_DELTA_PAGE)
            delta = e.deltaY * 140;

        this.distance += delta / 200;

        e.preventDefault();
        return false;
    }

    private handleMouseDown(e: MouseEvent) {
        if (e.button == this.cameraMoveButton) {
            this.mousePressed = true;
            e.preventDefault();
            return false;
        }
    }

    private handleMouseMove(e: MouseEvent) {
        if (this.mousePressed) {
            this.xAngle += e.movementX/this.canvas.width*180;
            this.yAngle += e.movementY/this.canvas.height*180;
            e.preventDefault();
            return false;
        }
    }

    private handleMouseUp(e: MouseEvent) {
        if (e.button == this.cameraMoveButton) {
            this.mousePressed = false;
            e.preventDefault();
        }
    }

    private handleMouseLeave(e: MouseEvent) {
        this.mousePressed = false;
    }
}
