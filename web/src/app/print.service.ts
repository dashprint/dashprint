import { Injectable } from '@angular/core';
import { Printer, DiscoveredPrinter } from './Printer';
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
              rv.push(new class Printer {
                  id = key;
                  name = data[key].name;
                  defaultPrinter = data[key]['default'];
                  apiKey = data[key].api_key;
                  devicePath = data[key].device_path;
                  baudRate = data[key].baud_rate;
                  stopped = data[key].stopped;
                  connected = data[key].connected;
              });
          });

          return rv;
      });
  }

  discoverPrinters() : Observable<DiscoveredPrinter[]> {
      return this.http.post<DiscoveredPrinter[]>('/api/v1/printers/discover', '');
  }

}
