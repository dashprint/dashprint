import { Injectable } from '@angular/core';
import {Printer, DiscoveredPrinter, PrinterTemperature, PrinterTemperatures} from './Printer';
import { Observable } from 'rxjs/Observable';
import { of } from 'rxjs/observable/of';
import 'rxjs/add/operator/map';
import { HttpClient, HttpHeaders } from '@angular/common/http';

@Injectable()
export class PrintService {

  constructor(private http: HttpClient) { }

  getPrinters() : Observable<Printer[]> {
      return this.http.get<Object>('/api/v1/printers').map(data => {
          let rv: Printer[] = [];

          Object.keys(data).forEach(key => {
              let printer = new Printer();
              printer.id = key;
              printer.name = data[key].name;
              printer.defaultPrinter = data[key]['default'];
              printer.apiKey = data[key].api_key;
              printer.devicePath = data[key].device_path;
              printer.baudRate = data[key].baud_rate;
              printer.stopped = data[key].stopped;
              printer.connected = data[key].connected;
              printer.width = data[key].width;
              printer.height = data[key].height;
              printer.depth = data[key].depth;
              rv.push(printer);
          });

          return rv;
      });
  }

  discoverPrinters() : Observable<DiscoveredPrinter[]> {
      return this.http.post<DiscoveredPrinter[]>('/api/v1/printers/discover', '');
  }

  addPrinter(printer: Printer) : Observable<string> {
      return Observable.create((observer) => {
          this.http.post('/api/v1/printers', {
              name: printer.name,
              default_printer: printer.defaultPrinter,
              device_path: printer.devicePath,
              baud_rate: printer.baudRate,
              stopped: printer.stopped,
              width: printer.width,
              height: printer.height,
              depth: printer.depth
          }, {observe: "response"}).subscribe(resp => {
              // TODO: Error handling
              let newurl = resp.headers.get('Location');
              observer.next(newurl);
              observer.complete();
          });
      });
  }

  getPrinterTemperatures(printer: Printer) : Observable<PrinterTemperatures> {
      return this.http.get<PrinterTemperatures>('/api/v1/printers/'+printer.id+'/temperatures');
  }

}
