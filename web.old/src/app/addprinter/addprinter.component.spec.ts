import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { AddprinterComponent } from './addprinter.component';

describe('AddprinterComponent', () => {
  let component: AddprinterComponent;
  let fixture: ComponentFixture<AddprinterComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ AddprinterComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(AddprinterComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
