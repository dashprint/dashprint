export class Printer {
    id: string;
    name: string;
    apiKey: string;
    devicePath: string;
    baudRate: number;
    defaultPrinter: boolean;
    connected: boolean;
    stopped: boolean;
    width: number;
    height: number;
    depth: number;
}

export class DiscoveredPrinter {
    path: string;
    name: string;
    vendor: string;
    serial: string;
}
