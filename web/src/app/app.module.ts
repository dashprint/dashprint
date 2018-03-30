import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { HttpClientModule } from '@angular/common/http';

import { AppComponent } from './app.component';
import { PrintService } from './print.service';
import { AddprinterComponent } from './addprinter/addprinter.component';

import { BsDropdownModule } from 'ngx-bootstrap/dropdown';
import { ModalModule } from 'ngx-bootstrap/modal';

@NgModule({
  declarations: [
    AppComponent,
    AddprinterComponent
  ],
  imports: [
    BrowserModule,
    HttpClientModule,
    BsDropdownModule.forRoot(),
    ModalModule.forRoot(),
  ],
  providers: [PrintService],
  bootstrap: [AppComponent],
  entryComponents: [
    AddprinterComponent,
  ],
})
export class AppModule { }
