import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { HttpClientModule } from '@angular/common/http';

import { AppComponent } from './app.component';
import { PrintService } from './print.service';
import { ModalService } from './modal.service';
import { AddprinterComponent } from './addprinter/addprinter.component';
import { ClarityModule } from "@clr/angular";
import {BrowserAnimationsModule} from "@angular/platform-browser/animations";
import {StlmodelService} from "./webgl/stlmodel.service";
import {WebsocketService} from "./websocket.service";

@NgModule({
  declarations: [
    AppComponent,
    AddprinterComponent
  ],
  imports: [
    BrowserModule,
    HttpClientModule,
    ClarityModule,
    BrowserAnimationsModule,
    FormsModule,
    ReactiveFormsModule
  ],
  providers: [PrintService, ModalService, StlmodelService, WebsocketService],
  bootstrap: [AppComponent],
  entryComponents: [
    AddprinterComponent,
  ],
})
export class AppModule { }
