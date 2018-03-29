export class Printer {
    id: string;
    name: string;
    apiKey: string;
    devicePath: string;
    baudRate: number;
    defaultPrinter: boolean;
    connected: boolean;
}

export class DiscoveredPrinter {
    path: string;
    name: string;
    vendor: string;
    serial: string;
}
