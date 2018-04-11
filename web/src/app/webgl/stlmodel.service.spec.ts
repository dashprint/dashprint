import { TestBed, inject } from '@angular/core/testing';

import { StlmodelService } from './stlmodel.service';

describe('StlmodelService', () => {
  beforeEach(() => {
    TestBed.configureTestingModule({
      providers: [StlmodelService]
    });
  });

  it('should be created', inject([StlmodelService], (service: StlmodelService) => {
    expect(service).toBeTruthy();
  }));
});
