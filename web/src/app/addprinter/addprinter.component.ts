import { Component, OnInit } from '@angular/core';
import {PrintService} from "../print.service";
import {DiscoveredPrinter} from "../Printer";
import {Modal} from "../Modal";
import {FormControl, FormGroup, Validators, FormBuilder} from "@angular/forms";

@Component({
  selector: 'modal-addprinter',
  templateUrl: './addprinter.component.html',
  styleUrls: ['./addprinter.component.css']
})
export class AddprinterComponent extends Modal implements OnInit {
  discoveredPrinters: DiscoveredPrinter[];
  //selectedDevice: DiscoveredPrinter;
  //devicePath: string;
  //baudRate: number = 115200;
  //printerName: string;
  formPageOne: FormGroup;

  constructor(private printService: PrintService, fb: FormBuilder) {
      super();

      this.formPageOne = fb.group({
          "selectedDevice": null,
          "devicePath":["", Validators.required],
          "baudRate":[115200, [Validators.required, Validators.min(1200)]],
          "printerName": ["", Validators.required]
      });
  }

  ngOnInit() {
    this.discoverPrinters();
  }

  discoverPrinters() {
    this.printService.discoverPrinters().subscribe(printers => {
      this.discoveredPrinters = printers;

      let custom: DiscoveredPrinter = new DiscoveredPrinter();
      custom.name = "Custom device";
      custom.path = "custom";

      this.discoveredPrinters.push(custom);

      // this.selectedDevice = this.discoveredPrinters[0];
    });
  }

  onDeviceChanged() {
    let selectedDevice = this.formPageOne.get('selectedDevice').value;

    if (selectedDevice.path != "custom") {
        this.formPageOne.get('printerName').setValue(selectedDevice.name);
        this.formPageOne.get('devicePath').setValue(selectedDevice.path);
    }
  }

}
