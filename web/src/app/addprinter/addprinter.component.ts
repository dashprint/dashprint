import { Component, OnInit } from '@angular/core';
import {PrintService} from "../print.service";
import {DiscoveredPrinter} from "../Printer";
import {Modal} from "../Modal";

@Component({
  selector: 'modal-addprinter',
  templateUrl: './addprinter.component.html',
  styleUrls: ['./addprinter.component.css']
})
export class AddprinterComponent extends Modal implements OnInit {
  discoveredPrinters: DiscoveredPrinter[];

  constructor(private printService: PrintService) {
      super();
  }

  ngOnInit() {
    this.discoverPrinters();
  }

  discoverPrinters() {
    this.printService.discoverPrinters().subscribe(printers => this.discoveredPrinters = printers);
  }

}
