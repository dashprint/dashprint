import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { HttpClientModule } from '@angular/common/http';

import { AppComponent } from './app.component';
import { PrintService } from './print.service';
import { ModalService } from './modal.service';
import { AddprinterComponent } from './addprinter/addprinter.component';
import { ClarityModule } from "@clr/angular";
import {BrowserAnimationsModule} from "@angular/platform-browser/animations";

@NgModule({
  declarations: [
    AppComponent,
    AddprinterComponent
  ],
  imports: [
    BrowserModule,
    HttpClientModule,
    ClarityModule,
    BrowserAnimationsModule
  ],
  providers: [PrintService, ModalService],
  bootstrap: [AppComponent],
  entryComponents: [
    AddprinterComponent,
  ],
})
export class AppModule { }
