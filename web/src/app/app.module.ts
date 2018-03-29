import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { HttpClientModule } from '@angular/common/http';

import { AppComponent } from './app.component';
import { PrintService } from './print.service';
import { AddprinterComponent } from './addprinter/addprinter.component';


@NgModule({
  declarations: [
    AppComponent,
    AddprinterComponent
  ],
  imports: [
    BrowserModule,
    HttpClientModule
  ],
  providers: [PrintService],
  bootstrap: [AppComponent]
})
export class AppModule { }
