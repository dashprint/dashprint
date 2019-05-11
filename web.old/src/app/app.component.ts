import { Component, OnInit, ViewContainerRef, ViewChild, ElementRef } from '@angular/core';
import {Printer, PrinterTemperatures, TemperaturePoint} from './Printer';
import { PrintService } from './print.service';
import { ModalService } from './modal.service';
import { AddprinterComponent } from './addprinter/addprinter.component';
import {WebsocketService} from "./websocket.service";
import {Subscription} from "rxjs/Subscription";
import { TemperatureGraph } from './canvas/TemperatureGraph';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  title = 'app';
  printers: Printer[];

  selectedPrinter: Printer;
  temperatures: PrinterTemperatures;
  temperatureHistory: TemperaturePoint[];

  selectedPrinterSubscription: Subscription;
  temperaturesSubscription: Subscription;

  dragOver: number = 0;
  runningEnableDisable: boolean = false;

  newTargetT;
  newTargetB;

  temperaturesGraph: ElementRef;
  temperaturesGraphRenderer: TemperatureGraph;
  @ViewChild("temperaturesGraph") set content(content: ElementRef) {
    this.temperaturesGraph = content;

    if (this.temperaturesGraph) {
      this.temperaturesGraphRenderer = new TemperatureGraph(this.temperaturesGraph.nativeElement, (name, value) => {
        let changes = {};
        changes[name] = value;

        this.printService.setPrinterTargetTemperatures(this.selectedPrinter, changes).subscribe(result => {
          console.log("Target temp change result: " + result);
        });
      });
      this.updateTemperaturesGraph();
    }
  }

  @ViewChild('modals', {
    read: ViewContainerRef
  }) viewContainerRef: ViewContainerRef;

  constructor(private printService: PrintService, private modalService: ModalService, private websocketService: WebsocketService) {
  }

  ngOnInit() {
      // Get printer list
      this.updatePrinterList();
      this.websocketService.subscribeToPrinterList(() => this.updatePrinterList());

      this.modalService.setRootViewContainerRef(this.viewContainerRef);
  }

  updatePrinterList() {
    console.log("updatePrinterList() called");

    this.printService.getPrinters().subscribe(printers => {
      this.printers = printers;

      // Select the default printer if none is selected
      if (!this.selectedPrinter && this.printers && this.printers.length) {
        this.printers.forEach(p => {
          if (p.defaultPrinter)
            this.switchPrinter(p);
        });

        if (!this.selectedPrinter)
          this.switchPrinter(this.printers[0]);
      }
    });
  }

  addPrinter() {
      // HOWTO: https://medium.com/front-end-hacking/dynamically-add-components-to-the-dom-with-angular-71b0cb535286
      this.modalService.showModal(AddprinterComponent);
  }

  private pushTempHistory() {
    const MAX_TEMPERATURE_HISTORY = 30*60*1000;
    // clone current temperatures
    var temps: PrinterTemperatures = JSON.parse(JSON.stringify(this.temperatures));
    var now: Date = new Date();

    this.temperatureHistory.push({ when: now, values: temps });

    // Kill old data
    while (this.temperatureHistory.length > 0 && (now.getUTCMilliseconds() - this.temperatureHistory[0].when.getUTCMilliseconds()) > MAX_TEMPERATURE_HISTORY)
      this.temperatureHistory.shift();
  }

  switchPrinter(printer: Printer) {
    if (printer === this.selectedPrinter)
      return;

    if (this.selectedPrinterSubscription) {
      this.selectedPrinterSubscription.unsubscribe();
      this.selectedPrinterSubscription = null;
    }

    if (this.temperaturesSubscription) {
      this.temperaturesSubscription.unsubscribe();
      this.temperaturesSubscription = null;
    }

    this.selectedPrinter = printer;

    this.selectedPrinterSubscription = this.websocketService
        .subscribeToPrinter(this.selectedPrinter).subscribe((printer: Printer) => {});

    this.temperatures = null;
    this.temperatureHistory = null;
    this.updateTemperaturesGraph();

    this.printService.getPrinterTemperatures(this.selectedPrinter).subscribe((temps) => {
      this.temperatureHistory = temps;

      if (temps && temps.length > 0)
        this.temperatures = temps[temps.length-1].values;
      else
        this.temperatures = {};
      this.updateTemperaturesGraph();

      this.temperaturesSubscription = this.websocketService.subscribeToPrinterTemperatures(this.selectedPrinter, this.temperatures)
          .subscribe((temps) => {
              this.pushTempHistory();
              this.updateTemperaturesGraph();
      });
    });
  }

  private updateTemperaturesGraph() {
    if (!this.temperaturesGraphRenderer)
      return;

    this.temperaturesGraphRenderer.temperatureHistory = this.temperatureHistory;
    this.temperaturesGraphRenderer.temperatures = JSON.parse(JSON.stringify(this.temperatures));
    this.temperaturesGraphRenderer.render();
  }

  setTargetTemperatures() {
    let changes = {
      'B': parseInt(this.newTargetB),
      'T': parseInt(this.newTargetT)
    }
    this.printService.setPrinterTargetTemperatures(this.selectedPrinter, changes).subscribe(result => {
      console.log("Target temp change result: " + result);
    });
  }

  onFileDrop(event: DragEvent) {
    event.preventDefault();

    if (event.dataTransfer.items) {
      for (let i = 0; i < event.dataTransfer.items.length; i++) {
        if (event.dataTransfer.items[i].kind === 'file') {
          let file: File = event.dataTransfer.items[i].getAsFile();
          this.processFile(file);
          break;
        }
      }
    } else {
      for (let i = 0; i < event.dataTransfer.files.length; i++) {
        this.processFile(event.dataTransfer.files[i]);
        break;
      }
    }
  }

  private processFile(file: File) {
    if (!file.name.toLowerCase().endsWith(".gcode")) {
      // TODO: Display an error
        return;
    }
  }

  onDragEnter(event: DragEvent) {
    console.debug("onDragEnter");
    this.dragOver++;
    event.preventDefault();
    event.stopPropagation();
  }

  onDragLeave(event: DragEvent) {
    console.debug("onDragLeave");
    this.dragOver--;
  }

  onDragOver(event: DragEvent) {
    event.preventDefault();
  }

  startStopPrinter(printer: Printer, start: boolean) {
    this.runningEnableDisable = true;
    this.printService.modifyPrinter(printer, { stopped: !start })
        .subscribe(x => { this.runningEnableDisable = false; });
  }

  editPrinter(printer: Printer) {
    // TODO
  }
}
