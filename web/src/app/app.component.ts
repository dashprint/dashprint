import { Component, OnInit } from '@angular/core';
import { Printer } from './Printer';
import { PrintService } from './print.service';
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

  constructor(private printService: PrintService) {

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

  }
}
