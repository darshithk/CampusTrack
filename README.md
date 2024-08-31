# Local Watch - Vehicle Tracking Solution


> Developed as part of the EE542 Coursework at University of Southern California

## Team Members
1. [Swanav Swaroop](https://linkedin.com/in/swanav)
2. [Rutuparna K Patkar](https://linkedin.com/in/rutuparna-k-patkar)
3. [Darshith Karthikeyan](https://linkedin.com/in/darshith-karthikeyan)

## Components

1. `app` - A Flutter powered Mobile App to monitor vehicle statistics, routes, alerts.
2. `firmware` - Firmware on the xDot based endnode resposible for sensing data and relaying it to the gateway.
3. `gateway` - Node-Red flows for the LoRa Conduit collecting data from X-Dot nodes and relaying it to the dashboard.


## Building and Running

### App

#### Flutter Installation
Ensure that Flutter is installed on your machine. If not, follow the official Flutter installation guide.

#### Project Setup
Clone the repository:

```bash
git clone https://github.com/swanav/campus-track.git
cd campus-track
```
#### Install dependencies:

```bash
flutter pub get
```
#### Building

To build the project for development, run:

```bash
flutter build apk
```

To build the project for production, run:

```bash
flutter build apk --release
```
This will generate the APK file in the build/app/outputs/flutter-apk directory.

#### Running the App
Connect a device or start an emulator, then run:

```bash
flutter run
```
This will install and launch the app on the connected device or emulator.

### Firmware 

#### xDot Setup
Connect your xDot device to your development machine using USB.

#### Building the Project
Clone the Mbed project repository:
```bash
mbed import https://github.com/your-username/your-mbed-project.git
cd your-mbed-project
```

Configure the Mbed project for the xDot platform:
```bash
mbed target xDot_L151CC
mbed toolchain GCC_ARM
```

Update the Mbed project dependencies:
```bash
mbed update
```

Build the project
```bash
mbed compile
```

#### Flashing the xDot

Connect the xDot device to your development machine.

```bash
cp BUILD/xDot_L151CC/GCC_ARM/your-mbed-project.bin /path/to/xDot/mounted/drive
```

Replace /path/to/xDot/mounted/drive with the path to the mounted drive of your xDot device.

#### Running the Project
Disconnect and reconnect the xDot device to ensure the flashed binary is loaded.

Power on the xDot.

### Gateway

#### Importing a Flow

1. In the Node-RED editor, click on the menu icon in the upper right corner.
2. Choose the "Import" option from the menu.
3. Click on "Clipboard."
4. Open the JSON file containing your Node-RED flow.
5. Copy the entire content of the JSON file.
6. Paste the content into the clipboard in the Node-RED editor.
7. Click on "Import."

Your flow should now be imported into Node-RED.

#### Running the Flow
After importing the flow, connect any necessary input nodes (e.g., inject nodes, MQTT nodes).
> You will also need to provide the relevant certificates from AWS IoT Core to authenticate the Gateway.

Deploy the flow by clicking the "Deploy" button in the Node-RED editor.



