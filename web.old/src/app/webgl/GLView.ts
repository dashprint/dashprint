import * as mat4 from "gl-matrix-mat4";
import {Renderable} from "./Renderable";
import {Camera} from "./Camera";

export interface ProgramInfo {
    program: WebGLProgram;

    vertexPosition: GLint;
    vertexColor: GLint;

    projectionMatrix: WebGLUniformLocation;
    modelViewMatrix: WebGLUniformLocation;
    cameraMatrix: WebGLUniformLocation;
}

export class GLView {
    protected gl: WebGLRenderingContext;
    protected programInfo: ProgramInfo;
    private scene: Renderable[] = [];
    public camera: Camera;

    constructor(private canvas: HTMLCanvasElement) {

        this.canvas.addEventListener("webglcontextlost", (event: Event) => event.preventDefault(), false);
        this.canvas.addEventListener("webglcontextrestored", () => this.initialize(), false);

        this.camera = new Camera(canvas);

        this.camera.xAngle = 20;
        this.camera.yAngle = 20;
        this.camera.distance = 2.5;
        this.camera.onCameraChange = () => this.requestRender();
    }

    initialize() {
        this.setup();
        this.createBuffers();
        this.drawScene();
    }

    setup() {
        this.gl = this.canvas.getContext('webgl', { alpha: false });

        if (!this.gl)
            throw new Error("WebGL is not available");

        const vsSource = `
            attribute vec4 aVertexPosition;
            attribute vec4 aVertexColor;
            
            uniform mat4 uModelViewMatrix;
            uniform mat4 uProjectionMatrix;
            uniform mat4 uCameraMatrix;
            
            varying lowp vec4 vColor;

            void main(void) {
              gl_Position = uProjectionMatrix * uCameraMatrix * uModelViewMatrix * aVertexPosition;
              vColor = aVertexColor;
            }
          `;

        const fsSource = `
            varying lowp vec4 vColor;

            void main(void) {
              gl_FragColor = vColor;
            }
          `;

        let shaderProgram = this.initShaderProgram(vsSource, fsSource);
        let gl = this.gl;

        this.programInfo = {
            program: shaderProgram,

            vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
            vertexColor: gl.getAttribLocation(shaderProgram, 'aVertexColor'),

            projectionMatrix: gl.getUniformLocation(shaderProgram, 'uProjectionMatrix'),
            modelViewMatrix: gl.getUniformLocation(shaderProgram, 'uModelViewMatrix'),
            cameraMatrix: gl.getUniformLocation(shaderProgram, 'uCameraMatrix'),
        };
    }

    public requestRender() {
        requestAnimationFrame(this.drawScene.bind(this));
    }

    createBuffers() {
    }

    initShaderProgram(vsSource, fsSource) {
        let gl = this.gl;

        const vertexShader = this.loadShader(gl.VERTEX_SHADER, vsSource);
        const fragmentShader = this.loadShader(gl.FRAGMENT_SHADER, fsSource);

        // Create the shader program

        const shaderProgram = gl.createProgram();
        gl.attachShader(shaderProgram, vertexShader);
        gl.attachShader(shaderProgram, fragmentShader);
        gl.linkProgram(shaderProgram);

        // If creating the shader program failed, abort

        if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS))
            throw new Error("Cannot compile GL shaders!");

        return shaderProgram;
    }

    loadShader(type, source) {
        let gl = this.gl;

        const shader = gl.createShader(type);

        // Send the source to the shader object

        gl.shaderSource(shader, source);

        // Compile the shader program

        gl.compileShader(shader);

        // See if it compiled successfully

        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            gl.deleteShader(shader);
            throw new Error('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(shader));
        }

        return shader;
    }

    drawScene() {
        let gl = this.gl;

        gl.clearColor(1.0, 1.0, 1.0, 1.0);
        gl.clearDepth(1.0);
        gl.enable(gl.DEPTH_TEST);
        gl.depthFunc(gl.LEQUAL);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
        gl.enable(gl.BLEND);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        const fieldOfView = 45 * Math.PI / 180;   // in radians
        var aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
        const zNear = 0.1;
        const zFar = 100.0;
        const projectionMatrix = mat4.create();

        // FIXME: Use this: http://marcj.github.io/css-element-queries/
        if (isNaN(aspect))
            aspect = 1;

        mat4.perspective(projectionMatrix,
            fieldOfView,
            aspect,
            zNear,
            zFar);

        gl.useProgram(this.programInfo.program);

        gl.uniformMatrix4fv(
            this.programInfo.projectionMatrix,
            false,
            projectionMatrix);

        this.camera.apply(this.gl, this.programInfo);

        this.scene.forEach(r => {
             if (r.shouldRender(this.camera))
                r.render(this.gl, this.programInfo)
        });
    }

    public addRenderable(r: Renderable, prepend: boolean = false) {
        if (prepend)
            this.scene.unshift(r);
        else
            this.scene.push(r);

        r.allocate(this.gl, this.programInfo);
    }

    public removeRenderable(r: Renderable) {
        let index = this.scene.indexOf(r);
        if (index != -1)
            this.scene.splice(index, 1);

        r.deallocate(this.gl);
    }

    static glColor(r: number, g: number, b: number, a: number): number[] {
        return [r/255, g/255, b/255, a/255];
    }
}
