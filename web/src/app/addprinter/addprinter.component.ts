import { Component, OnInit, ViewChild, ElementRef } from '@angular/core';
import {PrintService} from "../print.service";
import {DiscoveredPrinter, Printer} from "../Printer";
import {Modal} from "../Modal";
import {FormControl, FormGroup, Validators, FormBuilder} from "@angular/forms";
import {GLView} from "../webgl/GLView";
import {PrinterView} from "../webgl/PrinterView";
import {StlmodelService} from "../webgl/stlmodel.service";
import {ClrWizardPage} from "@clr/angular";

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
  formPageTwo: FormGroup;
  @ViewChild("finishPage") finishPage: ClrWizardPage;

  @ViewChild('printerPreview') printerPreview: ElementRef;
  printerView: PrinterView;

  constructor(private printService: PrintService, private stlLoader: StlmodelService, fb: FormBuilder) {
      super();

      this.formPageOne = fb.group({
          "selectedDevice": null,
          "devicePath":["", Validators.required],
          "baudRate":[115200, [Validators.required, Validators.min(1200)]],
          "printerName": ["", Validators.required]
      });

      this.formPageTwo = fb.group({
          "shape": "rectangular",
          "height": [200, [Validators.required, Validators.min(10)]],
          "width": [200, [Validators.required, Validators.min(10)]],
          "depth": [200, [Validators.required, Validators.min(10)]]
      });
  }

  ngOnInit() {
    this.discoverPrinters();
  }

  ngAfterViewInit() {
      //try {
          this.printerView = new PrinterView(this.printerPreview.nativeElement, this.stlLoader);
          this.printerView.initialize();
      //} catch (e) {
      //    console.error("Cannot setup printer preview: " + e);
      //}
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

  onDimensionsChanged() {
      let x = this.formPageTwo.get('width').value;
      let y = this.formPageTwo.get('depth').value;
      let z = this.formPageTwo.get('height').value;
      if (x > 0 && y > 0 && z > 0)
        this.printerView.setDimensions(x, y, z);
  }

  onCreate() {
      // TODO:
      let printer: Printer = new Printer();
      printer.width = this.formPageTwo.get('width').value;
      printer.height = this.formPageTwo.get('height').value;
      printer.depth = this.formPageTwo.get('depth').value;

      printer.baudRate = this.formPageOne.get('baudRate').value;
      printer.devicePath = this.formPageOne.get('devicePath').value;
      printer.name = this.formPageOne.get('printerName').value;

      this.printService.addPrinter(printer).subscribe((url : string) => {
            console.log("Printer created at " + url);
            this.finishPage.completed = true;

            // TODO: trigger printer list reload
      });
  }

}
