import { Component, OnInit, ViewContainerRef, ViewChild } from '@angular/core';
import { Printer } from './Printer';
import { PrintService } from './print.service';
import { ModalService } from './modal.service';
import { AddprinterComponent } from './addprinter/addprinter.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  title = 'app';
  printers: Printer[];
  selectedPrinter: Printer;

  @ViewChild('modals', {
    read: ViewContainerRef
  }) viewContainerRef: ViewContainerRef;

  constructor(private printService: PrintService, private modalService: ModalService) {
  }

  ngOnInit() {
      // Get printer list
      this.updatePrinterList();

      this.modalService.setRootViewContainerRef(this.viewContainerRef);
  }

  updatePrinterList() {
    this.printService.getPrinters().subscribe(printers => {
      this.printers = printers;

      // Select the default printer if none is selected
      if (!this.selectedPrinter && this.printers) {
        this.printers.forEach(p => {
          if (p.defaultPrinter)
            this.selectedPrinter = p;
        });
      }
    });
  }

  addPrinter() {
      // HOWTO: https://medium.com/front-end-hacking/dynamically-add-components-to-the-dom-with-angular-71b0cb535286
      this.modalService.showModal(AddprinterComponent);
  }
}
