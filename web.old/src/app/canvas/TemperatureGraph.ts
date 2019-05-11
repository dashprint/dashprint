import { OnChanges, Input } from "@angular/core";
import {Printer, PrinterTemperatures, TemperaturePoint, PrinterTemperature} from '../Printer';

const GRAPH_BOTTOM_HEIGHT = 60;

export class TemperatureGraph {
	public temperatures: PrinterTemperatures;
	public temperatureHistory: TemperaturePoint[];

	private targetAreas = {};
	private pxPerDegree : number;
	  
	constructor(private canvas: HTMLCanvasElement, private changeTargetTemp: (name: string, value: number) => void) {
		this.render();

		canvas.addEventListener("mousemove", (event: MouseEvent) => this.onMouseMove(event));
		canvas.addEventListener("mousedown", (event: MouseEvent) => this.onMouseDown(event));
		canvas.addEventListener("mouseup", (event: MouseEvent) => this.onMouseUp(event));
	}

	private fontDefinition(px: number) : string {
		return "normal "+px+"px Metropolis, sans-serif";
	}

	render() {
		let ctx: CanvasRenderingContext2D = this.canvas.getContext('2d');

		ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
		this.targetAreas = {};

		if (!this.temperatures)
			return;

		let maxTemp = this.findMaxTemp();

		// Temp history dimensions
		let graphX = 80;
		let graphY = 0;
		let graphHeight = this.canvas.height - graphY - GRAPH_BOTTOM_HEIGHT;
		let graphWidth = this.canvas.width - graphX - 1;

		ctx.font = this.fontDefinition(12);

		this.renderTempText(ctx, this.temperatures.T, "Tool", "#f00", 65, this.canvas.height-26);

		this.renderTempText(ctx, this.temperatures.B, "Bed", "#00f", 65, this.canvas.height-10);

		// Left side temp scale
		ctx.fillStyle = "#aaa";
		ctx.strokeStyle = "#000";

		ctx.beginPath();
		maxTemp += 10;
		for (let temp = maxTemp-10; temp >= 0; temp -= 50) {
			let text = temp.toFixed(0) + " °C";
			let textW = ctx.measureText(text);
			let y = graphY + (graphHeight/maxTemp)*(maxTemp-temp);

			ctx.fillText(text, graphX - textW.width - 5, y+8);

			ctx.moveTo(graphX+0.5, y+0.5);
			ctx.lineTo(graphX-3.5, y+0.5);
		}
		ctx.stroke();

		// Bottom time scale (30 mins of history)
		if (this.temperatureHistory && this.temperatureHistory.length > 0) {
			this.drawTempGraph(ctx, "T", "#f00", maxTemp, graphX, graphY, graphWidth, graphHeight);
			this.drawTempGraph(ctx, "B", "#00f", maxTemp, graphX, graphY, graphWidth, graphHeight);

			let timeAgo = (new Date().valueOf() - this.temperatureHistory[this.temperatureHistory.length-1].when.valueOf()) / (1000*60);

			ctx.strokeStyle = "#000";

			ctx.beginPath();
			for (let mins = 0; mins <= 30; mins += 6) {
				let text = (-timeAgo-mins).toFixed(0) + " min";
				let textW = ctx.measureText(text).width;

				let x = graphWidth + graphX - (graphWidth/30*mins);
				ctx.fillText(text, x - textW, graphY + graphHeight + 21);

				ctx.moveTo(x, graphY + graphHeight);
				ctx.lineTo(x, graphY + graphHeight + 4);
			}
			ctx.stroke();
		}

		// Black rectange around the graph area
		ctx.strokeStyle = '#000';
		ctx.lineWidth = 1;
		ctx.strokeRect(graphX+0.5, graphY+0.5, graphWidth, graphHeight);

		// Draw target temp triangles
		this.drawTempTriangle(ctx, "T", "#f00", maxTemp, graphX, graphY, 0, graphHeight);
		this.drawTempTriangle(ctx, "B", "#00f", maxTemp, graphX, graphY, 17, graphHeight);

		this.pxPerDegree = graphHeight / maxTemp;
	}

	private renderTempText(ctx: CanvasRenderingContext2D, temp: PrinterTemperature, label: string, color: string, xOffset: number, yOffset: number) : number {
		ctx.fillStyle = "#000";

		let text = label;

		if (temp)
			text += ": " + temp.current.toFixed(1) + " °C (→ "+temp.target.toFixed(1)+" °C)";

		let textW = ctx.measureText(text);

		ctx.fillText(text, xOffset+10, yOffset+8);

		ctx.fillStyle = color;
		ctx.fillRect(xOffset, yOffset, 8, 8);

		return xOffset + 10 + textW.width + 10;
	}

	private findMaxTemp() : number {
		let max: number = 300;

		for (let key in this.temperatures) {
			if (this.temperatures[key].current > max)
				max = this.temperatures[key].current;
			if (this.temperatures[key].target > max)
				max = this.temperatures[key].target;
		}

		for (let i = 0; i < this.temperatureHistory.length; i++) {
			for (let key in this.temperatureHistory[i].values) {
				if (this.temperatureHistory[i].values[key].current > max)
					max = this.temperatureHistory[i].values[key].current;
				if (this.temperatureHistory[i].values[key].target > max)
					max = this.temperatureHistory[i].values[key].target;
			}
		}

		// Round up to 50 deg. C
		max = Math.floor((max + 49) / 50) * 50;

		return max;
	}

	private drawTempTriangle(ctx: CanvasRenderingContext2D, key: string, color: string, maxTemp, graphX, graphY, offX, graphHeight) {
		let temp = this.temperatures[key] ? this.temperatures[key].target : 0;
		let y = graphY + graphHeight - graphHeight/maxTemp*temp;
		let yMoved = y;

		if (this.draggingTargetTemp === key)
		{
			const fontSize = 16;
			ctx.font = this.fontDefinition(fontSize);

			yMoved += this.draggingOffsetY;
			ctx.fillStyle = "#000";

			const text = "→ " + this.newTargetTemp.toFixed(0) + " °C";

			let textWidth = ctx.measureText(text).width;
			ctx.fillRect(offX + 25, yMoved, textWidth+4, fontSize+4);
			ctx.fillStyle = "#fff";
			ctx.fillText(text, offX+27, yMoved + fontSize);
		}

		ctx.fillStyle = color;

		ctx.beginPath();
		ctx.moveTo(offX, yMoved-10);
		ctx.lineTo(offX, yMoved+10);
		ctx.lineTo(offX+15, yMoved);
		ctx.lineTo(offX, yMoved-10);
		ctx.fill();

		this.targetAreas[key] = {
			x: offX,
			y: y-10,
			w: 15,
			h: 20
		};
	}

	private drawTempGraph(ctx: CanvasRenderingContext2D, key: string, color: string, maxTemp, graphX, graphY, graphWidth, graphHeight) {
		const GRAPH_MINUTES = 30;
		let latestTempDate : Date = this.temperatureHistory[this.temperatureHistory.length-1].when;

		ctx.strokeStyle = color;
		ctx.lineWidth = 1;

		ctx.beginPath();
		for (let i = this.temperatureHistory.length-1; i >= 0; i--) {
			let point = this.temperatureHistory[i];

			let timeDiff = (latestTempDate.valueOf() - point.when.valueOf()) / (60.0*1000);
			if (timeDiff > GRAPH_MINUTES)
				break;

			let temp = point.values[key].current;

			let x = graphX + graphWidth - graphWidth/GRAPH_MINUTES*timeDiff;
			let y = graphY + graphHeight - graphHeight/maxTemp*temp;

			if (i == this.temperatureHistory.length-1)
				ctx.moveTo(x + 0.5, y + 0.5);
			else
				ctx.lineTo(x + 0.5, y + 0.5);
		}
		ctx.stroke();

		ctx.globalAlpha = 0.4;
		ctx.beginPath();
		for (let i = this.temperatureHistory.length-1; i >= 0; i--) {
			let point = this.temperatureHistory[i];

			let timeDiff = (latestTempDate.valueOf() - point.when.valueOf()) / (60.0*1000);
			if (timeDiff > GRAPH_MINUTES)
				break;

			let temp = point.values[key].target;

			let x = graphX + graphWidth - graphWidth/GRAPH_MINUTES*timeDiff;
			let y = graphY + graphHeight - graphHeight/maxTemp*temp;

			if (i == this.temperatureHistory.length-1)
				ctx.moveTo(x + 0.5, y + 0.5);
			else
				ctx.lineTo(x + 0.5, y + 0.5);
		}
		ctx.stroke();

		ctx.globalAlpha = 1;
	}

	private findTargetTempTriangle(event: MouseEvent) : string {
		for (let key in this.targetAreas) {
			let area = this.targetAreas[key];
			let y = event.offsetY;

			if (this.draggingTargetTemp)
				y += this.draggingOffsetY;

			if (event.offsetX >= area.x && y >= area.y
				&& event.offsetX <= area.x+area.w && y <= area.y+area.h) {

					return key;
			}
		}

		return null;
	}

	private draggingTargetTemp: string = null;
	private draggingStartY: number = 0;
	private draggingOffsetY: number = 0;
	private newTargetTemp: number = 0;

	private onMouseMove(event: MouseEvent) {
		if (this.draggingTargetTemp) {
			let newOffsetY = event.offsetY - this.draggingStartY;
			let newTargetTemp = Math.round(this.temperatures[this.draggingTargetTemp].target - newOffsetY/this.pxPerDegree);

			// Target temp may not get below zero!
			if (newTargetTemp < 0)
				newTargetTemp = 0;
			else
				this.draggingOffsetY = newOffsetY;

			this.newTargetTemp = newTargetTemp;
		
			window.requestAnimationFrame(() => this.render());

			this.canvas.style.cursor = 'pointer';
		} else {

			let selectedArea = this.findTargetTempTriangle(event);

			// console.log("move x="+event.offsetX+", y="+event.offsetY);

			if (selectedArea)
				this.canvas.style.cursor = 'pointer';
			else
				this.canvas.style.cursor = 'default';

			// TODO: Show floating popup?
		}
	}

	private onMouseDown(event: MouseEvent) {
		if (!this.draggingTargetTemp && event.button == 0) {
			let selectedArea = this.findTargetTempTriangle(event);

			if (selectedArea) {
				this.draggingStartY = event.offsetY;
				this.draggingTargetTemp = selectedArea;
			}
		}
	}

	private onMouseUp(event: MouseEvent) {
		if (this.draggingTargetTemp && event.button == 0) {

			// Apply target temp
			this.changeTargetTemp(this.draggingTargetTemp, this.newTargetTemp);
			this.temperatures[this.draggingTargetTemp].target = this.newTargetTemp;

			this.newTargetTemp = 0;
			this.draggingTargetTemp = null;
			this.draggingOffsetY = 0;
			this.canvas.style.cursor = 'default';
			window.requestAnimationFrame(() => this.render());
		}
	}
}
