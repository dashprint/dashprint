import { OnChanges, Input } from "@angular/core";
import {Printer, PrinterTemperatures, TemperaturePoint, PrinterTemperature} from '../Printer';

export class TemperatureGraph {
	public temperatures: PrinterTemperatures;
	public temperatureHistory: TemperaturePoint[];
	  
	constructor(private canvas: HTMLCanvasElement) {
		this.render();
	}

	render() {
		let ctx: CanvasRenderingContext2D = this.canvas.getContext('2d');

		ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

		if (!this.temperatures)
			return;

		let maxTemp = this.findMaxTemp();

		// Reserve 60px on the left for setting target temp, and 50px on the bottom for current temps
		ctx.beginPath();
		ctx.strokeStyle = '#000';
		ctx.lineWidth = 1;
		ctx.moveTo(60.5, 0);
		ctx.lineTo(60.5, this.canvas.height - 50.5);
		ctx.lineTo(this.canvas.width, this.canvas.height - 50.5);
		ctx.stroke();

		ctx.font = "normal 12px Metropolis, sans-serif";

		this.renderTempText(ctx, this.temperatures.T, "Tool", "#f00", 65, this.canvas.height-24);

		this.renderTempText(ctx, this.temperatures.B, "Bed", "#00f", 65, this.canvas.height-10);

		// Temp history dimensions
		let graphX = 60;
		let graphY = 0;
		let graphHeight = this.canvas.height - graphY - 60;
		let graphWidth = this.canvas.width - graphX - 0;

		// Left side temp scale
		ctx.fillStyle = "#aaa";

		for (let temp = maxTemp; temp >= 0; temp -= 50) {
			let text = temp.toFixed(0) + " °C";
			let textW = ctx.measureText(text);
			ctx.fillText(text, graphX - textW.width - 3, graphY + (graphHeight/maxTemp)*(maxTemp-temp)+12);
		}

		// Bottom time scale (30 mins of history)
		// TODO
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
}
