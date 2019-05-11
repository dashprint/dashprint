import { Injectable } from '@angular/core';
import {Printer, PrinterTemperatures} from "./Printer";
import { Observable } from 'rxjs/Observable';

@Injectable()
export class WebsocketService {
  private socket: WebSocket;
  private pendingOperations = [];
  private subscriptions = {};

  constructor() {
    this.openSocket();
  }

  public subscribeToPrinterList(callback) : () => void {
      let handler = (change) => {
          callback();
      };
      this.doSubscribe("PrinterManager", handler);
      return () => {
          this.doUnsubscribe("PrinterManager", handler);
      };
  }

  public subscribeToPrinter(printer: Printer) : Observable<Printer> {
      return Observable.create((observer) => {
        let handler = (change) => {
            for (let key in change) {
                if (change.hasOwnProperty(key))
                    printer[key] = change[key];
            }

            observer.next(printer);
        };

        this.doSubscribe("Printer." + printer.id + ".state", handler);

        return () => {
            this.doUnsubscribe("Printer." + printer.id + ".state", handler);
        };
      });
  }

  public subscribeToPrinterTemperatures(printer: Printer, temps: PrinterTemperatures) : Observable<PrinterTemperatures> {
      return Observable.create((observer) => {
          let handler = (change) => {
              for (let key in change) {
                  if (change.hasOwnProperty(key)) {
                      let dot = key.lastIndexOf('.');
                      if (dot > -1) {
                          let tempName = key.substr(0, dot);
                          let tempParam = key.substr(dot+1);

                          if (!temps[tempName])
                            temps[tempName] = { current: 0, target: 0 };
                          temps[tempName][tempParam] = change[key];
                      }
                  }
              }

              observer.next(temps);
          };

          this.doSubscribe("Printer." + printer.id + ".temperature", handler);

          return () => {
              this.doUnsubscribe("Printer." + printer.id + ".temperature", handler);
          };
      });
  }

  private doSubscribe(what, handler) {
    if (this.socket.readyState != WebSocket.OPEN) {
      this.pendingOperations.push(() => this.doSubscribe(what, handler));
      return;
    }

    if (this.subscriptions[what]) {
        // We're subscribed already, add another callback
        this.subscriptions[what].push(handler);
    } else {
        // A brand new subscription
        this.socket.send(JSON.stringify({ subscribe: what }));
        this.subscriptions[what] = [ handler ];
    }
  }

  private doUnsubscribe(what, handler) {
      if (this.socket.readyState != WebSocket.OPEN) {
          this.pendingOperations.push(() => this.doUnsubscribe(what, handler));
          return;
      }

      if (!this.subscriptions[what])
          return;

      let index = this.subscriptions[what].indexOf(handler);
      if (index > -1)
          this.subscriptions[what].splice(index, 1);

      if (this.subscriptions[what].length === 0) {
          this.socket.send(JSON.stringify({ unsubscribe: what }));
          delete this.subscriptions[what];
      }
  }

  private openSocket() {
      var url: string;

      if (window.location.protocol == "https")
          url = "wss://";
      else
          url = "ws://";

      url += window.location.host + "/websocket";

      this.socket = new WebSocket(url);

      this.socket.onopen = () => {
          // Renew old subscriptions
          for (let key in this.subscriptions) {
              if (this.subscriptions.hasOwnProperty(key)) {
                  this.socket.send(JSON.stringify({ subscribe: key }));
              }
          }
          while (this.pendingOperations.length > 0)
              this.pendingOperations.shift()();
      };

      this.socket.onerror = () => this.reconnect();

      this.socket.onclose = () => this.reconnect();

      this.socket.onmessage = (msg: MessageEvent) => {
        this.processMessage(msg.data);
      };
  }

  private reconnect() {
      console.warn("Websocket connection went down");

      this.socket.onerror = null;
      this.socket.onclose = null;
      this.socket.close();
      setTimeout(() => {
          console.debug("Reconnecting the websocket...");
          this.openSocket();
      }, 1000);
  }

  private processMessage(msg: string) {
    let data = JSON.parse(msg);

    if (data.event) {
        console.debug("Received websocket event: " + msg);

        for (let key in data.event) {
            if (data.event.hasOwnProperty(key)) {
                let event = data.event[key];
                let handlers = this.subscriptions[key];

                if (handlers) {
                    for (let i = 0; i < handlers.length; i++)
                        handlers[i](event);
                }
            }
        }
    } else {
        console.warn("Received unknown websocket message: " + msg);
    }
  }
}
