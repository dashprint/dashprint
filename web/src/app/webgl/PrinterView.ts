import {GLView} from "./GLView";
import * as mat4 from "gl-matrix-mat4";
import {Triangles} from "./Triangles";
import {StlmodelService} from "./stlmodel.service";
import {Lines} from "./Lines";

const mmScale: number = 200;

export class PrinterView extends GLView {
    public printerShape: string = "rectangular";
    private x: number = 200;
    private y: number = 200;
    private z: number = 200;

    model: Triangles;
    printBox: Lines;
    axes: Lines;

    constructor(canvas: HTMLCanvasElement, private stlLoader: StlmodelService) {
        super(canvas);

        // Load the bed/platform model
        this.stlLoader.getByURL('/assets/meshes/prusai3_platform.stl').subscribe((model) => {
            console.log("Got a model: " + model);
            this.model = model;

            // Scale it down to 1.0
            this.model.setScale(0.005);

            this.model.color = new Float32Array([0.3, 0.3, 0.3, 0.15]);

            // Place the model beneath the printing area
            // this.model.position = new Float32Array([0, -0.5, 0]);
            this.addRenderable(this.model);
            this.drawScene();
        });

        this.camera.lookAt = new Float32Array([0, 0.25, 0]);
    }

    createBuffers() {
        super.createBuffers();

        // Add a print area box
        const vertices = [
            -0.5, -0.5,  0.5,
            -0.5,  0.5,  0.5,
             0.5,  0.5,  0.5,
             0.5, -0.5,  0.5,
            -0.5, -0.5, -0.5,
            -0.5,  0.5, -0.5,
            0.5,  0.5, -0.5,
            0.5, -0.5, -0.5
        ];

        const indices = new Uint16Array([
            0, 1,  1, 2,
            2, 3,  3, 0,
            4, 5,  5, 6,
            6, 7,  7, 4,
            0, 4,  1, 5,
            2, 6,  3, 7
        ]);

        const color = [0.75, 0.75, 1, 1];

        this.printBox = new Lines(vertices, color, 1.0, indices);
        this.addRenderable(this.printBox);

        // Add origin colored lines
        // X=red, Y=green, Z=blue (3D printing axes)
        this.axes = new Lines([], [], 2);
        this.axes.addLine(0, 0, 0, 0.1, 0, 0, GLView.glColor(255, 0, 0, 255));
        this.axes.addLine(0, 0, 0, 0, 0, -0.1, GLView.glColor(0, 255, 0, 255));
        this.axes.addLine(0, 0, 0, 0, 0.1, 0, GLView.glColor(0, 0, 255, 255));
        this.addRenderable(this.axes);

        this.setDimensions(this.x, this.y, this.z);
    }

    setDimensions(x: number, y: number, z: number) {
        // let base = Math.min(x, y, z);
        this.x = x;
        this.y = y;
        this.z = z;
        // this.plateScale = base;

        this.printBox.setScale(new Float32Array([x / mmScale, z / mmScale, y / mmScale]));
        this.printBox.position[1] = z/mmScale/2;
        this.printBox.updateMatrix();

        this.axes.position[0] = -x/mmScale/2;
        this.axes.position[2] = y/mmScale/2;
        this.axes.updateMatrix();

        this.requestRender();
    }

}
