import { Component, OnInit } from '@angular/core';
import {PrintService} from "../print.service";
import {DiscoveredPrinter} from "../Printer";
import { BsModalRef } from 'ngx-bootstrap/modal/bs-modal-ref.service';

@Component({
  selector: 'modal-content',
  templateUrl: './addprinter.component.html',
  styleUrls: ['./addprinter.component.css']
})
export class AddprinterComponent implements OnInit {
  discoveredPrinters: DiscoveredPrinter[];

  constructor(private printService: PrintService, public bsModalRef: BsModalRef) { }

  ngOnInit() {
    this.discoverPrinters();
  }

  discoverPrinters() {
    this.printService.discoverPrinters().subscribe(printers => this.discoveredPrinters = printers);
  }

}
