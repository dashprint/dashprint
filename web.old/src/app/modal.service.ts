import {
  ComponentFactoryResolver,
  ComponentRef,
  Injectable,
  Inject,
  ReflectiveInjector
} from '@angular/core'
import {ViewContainerRef} from "@angular/core";
import {Modal} from "./Modal";

@Injectable()
export class ModalService {
  factoryResolver: ComponentFactoryResolver;
  rootViewContainer: ViewContainerRef;

  constructor(@Inject(ComponentFactoryResolver) factoryResolver) {
    this.factoryResolver = factoryResolver;
  }

  setRootViewContainerRef(viewContainerRef) {
    this.rootViewContainer = viewContainerRef;
  }

  showModal(componentName): Modal {
    const factory = this.factoryResolver.resolveComponentFactory(componentName);
    const component: ComponentRef<Modal> = factory.create(this.rootViewContainer.parentInjector) as ComponentRef<Modal>;
    this.rootViewContainer.insert(component.hostView);

    component.instance.show();

    return component.instance;
  }

}
