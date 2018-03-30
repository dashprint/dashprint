import { Component, OnInit } from '@angular/core';
import { Printer } from './Printer';
import { PrintService } from './print.service';
import { BsModalService } from "ngx-bootstrap/modal";
import { BsModalRef } from 'ngx-bootstrap/modal/bs-modal-ref.service';
import { AddprinterComponent } from './addprinter/addprinter.component';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  title = 'app';
  bsModalRef: BsModalRef;
  printers: Printer[];
  selectedPrinter: Printer;

  constructor(private printService: PrintService, private modalService: BsModalService) {

  }

  ngOnInit() {
      // Get printer list
      this.updatePrinterList();
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
      this.bsModalRef = this.modalService.show(AddprinterComponent);
  }
}
