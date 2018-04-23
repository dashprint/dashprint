import { Component, OnInit, ViewContainerRef, ViewChild } from '@angular/core';
import {Printer, PrinterTemperatures} from './Printer';
import { PrintService } from './print.service';
import { ModalService } from './modal.service';
import { AddprinterComponent } from './addprinter/addprinter.component';
import {WebsocketService} from "./websocket.service";
import {Subscription} from "rxjs/Subscription";

class TemperaturePoint {
  value: number;
  time: Date;
}

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
  toolTempHistory: TemperaturePoint[];
  bedTempHistory: TemperaturePoint[];

  selectedPrinterSubscription: Subscription;
  temperaturesSubscription: Subscription;

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

  private pushTempHistory(target: TemperaturePoint[], source) {
    if (source) {
      if (target.length == 0 || target[target.length-1].value != source.current) {
          target.push({
              time: new Date(),
              value: source.current
          });
      }
    }
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
    this.toolTempHistory = [];
    this.bedTempHistory = [];

    this.printService.getPrinterTemperatures(this.selectedPrinter).subscribe((temps) => {
      this.temperatures = temps;

      this.pushTempHistory(this.toolTempHistory, temps.T);
      this.pushTempHistory(this.bedTempHistory, temps.B);

      this.temperaturesSubscription = this.websocketService.subscribeToPrinterTemperatures(this.selectedPrinter, this.temperatures)
          .subscribe((temps) => {
              this.pushTempHistory(this.toolTempHistory, temps.T);
              this.pushTempHistory(this.bedTempHistory, temps.B);
              // TODO: redraw temperature display
      });
    });
  }
}
