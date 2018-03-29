import { Component, OnInit } from '@angular/core';
import { Printer } from './Printer';
import { PrintService } from './print.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  title = 'app';
  printers: Printer[];

  constructor(private printService: PrintService) {

  }

  ngOnInit() {
      // Get printer list
      this.updatePrinterList();
  }

  updatePrinterList() {
    this.printService.getPrinters().subscribe(printers => this.printers = printers);
  }
}
