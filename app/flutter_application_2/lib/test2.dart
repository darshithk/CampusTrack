import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:flutter_map_geojson/flutter_map_geojson.dart';

import 'package:mqtt_client/mqtt_client.dart' as mqtt;
// import 'package:mqtt_client/mqtt_server_client.dart' as mqtt;
// import 'package:mqtt_client/mqtt_client.dart';

import 'geojson.dart';

void main() => runApp(MyApp());

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'MQTT Flutter Map Demo',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      debugShowCheckedModeBanner: false,
      home: MyHomePage(title: 'Campus Track : USC C Route'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  final String title;

  MyHomePage({Key? key, required this.title}) : super(key: key);

  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  final String topic = "gw/#"; // Your topic
  final MqttServerClient client = MqttServerClient.withPort(
      'a3dt6q7niaetkx-ats.iot.us-east-2.amazonaws.com', 'basicPubSub', 8883);

  late LatLng currentLatLng;

  GeoJsonParser geoJsonParser = GeoJsonParser(
    defaultMarkerColor: Colors.red,
    defaultPolygonBorderColor: Colors.red,
    defaultPolygonFillColor: Colors.red.withOpacity(0.1),
    defaultCircleMarkerColor: Colors.red.withOpacity(0.25),
  );

  bool loadingData = false;

  bool myFilterFunction(Map<String, dynamic> properties) {
    if (properties['section'].toString().contains('Point M-4')) {
      return false;
    } else {
      return true;
    }
  }

  // this is callback that gets executed when user taps the marker
  void onTapMarkerFunction(Map<String, dynamic> map) {
    // ignore: avoid_print
    print('onTapMarkerFunction: $map');
  }

  Future<void> processData() async {
    geoJsonParser.parseGeoJsonAsString(testGeoJson);
  }

  @override
  void initState() {
    super.initState();

    geoJsonParser.setDefaultMarkerTapCallback(onTapMarkerFunction);
    geoJsonParser.filterFunction = myFilterFunction;
    loadingData = true;
    Stopwatch stopwatch2 = Stopwatch()..start();
    processData().then((_) {
      setState(() {
        loadingData = false;
      });
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('GeoJson Processing time: ${stopwatch2.elapsed}'),
          duration: const Duration(milliseconds: 5000),
          behavior: SnackBarBehavior.floating,
          backgroundColor: Colors.green,
        ),
      );
    });

    //old
    currentLatLng = LatLng(34.03088, -118.28213); // Initial position
    _initializeMqttClient();
  }

  void _initializeMqttClient() async {
    client.logging(on: true);
    client.onConnected = _onConnected;
    client.onDisconnected = _onDisconnected;
    client.onSubscribed = _onSubscribed;

    ByteData rootCA = await rootBundle.load('keys/AmazonRootCA1.pem');
    ByteData deviceCert = await rootBundle.load(
        'keys/4f8762bd4ee46e50c7f70c9b49fdd1ff9f7fcb639f1b4b49d025488f0da32112-certificate.pem.crt');
    ByteData privateKey = await rootBundle.load(
        'keys/4f8762bd4ee46e50c7f70c9b49fdd1ff9f7fcb639f1b4b49d025488f0da32112-private.pem.key');

    SecurityContext context = SecurityContext.defaultContext;
    context.setClientAuthoritiesBytes(rootCA.buffer.asUint8List());
    context.useCertificateChainBytes(deviceCert.buffer.asUint8List());
    context.usePrivateKeyBytes(privateKey.buffer.asUint8List());

    client.securityContext = context;
    // client.port = 8883; // AWS IoT uses port 8883 for MQTT
    client.secure = true;

    // final connMess = MqttConnectMessage()
    //     .withClientIdentifier('flutter_client')
    //     .startClean()
    //     .withWillQos(MqttQos.atLeastOnce);
    // client.connectionMessage = connMess;

    await _connectClient();
  }

  Future<void> _connectClient() async {
    try {
      await client.connect();
    } on NoConnectionException catch (e) {
      print('NoConnectionException: $e');
      client.disconnect();
    } on SocketException catch (e) {
      print('SocketException: $e');
      client.disconnect();
    }

    if (client.connectionStatus!.state == MqttConnectionState.connected) {
      _subscribeToTopic('gw/#');
    } else {
      print('ERROR: MQTT client connection failed - exiting');
      exit(-1);
    }
  }

  void _subscribeToTopic(String topic) {
    print('Subscribing to the $topic topic');
    client.subscribe(topic, MqttQos.atLeastOnce);

    client.updates!.listen((List<MqttReceivedMessage<MqttMessage>> c) {
      final MqttPublishMessage recMess = c[0].payload as MqttPublishMessage;
      final String message =
          MqttPublishPayload.bytesToStringAsString(recMess.payload.message);

      print('Received message: $message from topic: ${c[0].topic}>');
      _updateLocation(message);
    });
  }

  // void _updateLocation(String message) {
  //   final latLngData = message.split(',');
  //   if (latLngData.length == 2) {
  //     print('currentlatlong22: $currentLatLng');
  //     final lat = double.tryParse(latLngData[0]);
  //     final lng = double.tryParse(latLngData[1]);
  //     if (lat != null && lng != null) {
  //       setState(() {
  //         currentLatLng = LatLng(lat, lng);
  //         print('currentlatlon23g: $currentLatLng');
  //       });
  //     }
  //   }
  // }

  void _updateLocation(String message) {
    // List<String> stringParts =
    //     message.substring(1, message.length - 1).split(', ');

    // List<double> coordinates =
    //     stringParts.map((str) => double.parse(str)).toList();

    dynamic event = jsonDecode(message);

    double lat = event['latitude'];
    double lng = event['longitude'];
    print('coordinates $lat, $lng'); // This will output: [-118.28115, 34.02577]

    setState(() {
      print('current latlon here ee');
      currentLatLng = LatLng(lat, lng);
    });
  }

  void _onConnected() {
    print('Connected to MQTT broker');
  }

  void _onDisconnected() {
    print('Disconnected from MQTT broker');
  }

  void _onSubscribed(String topic) {
    print('Subscribed to topic: $topic');
  }

  @override
  void dispose() {
    client.disconnect();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    print('currentlatlong2: $currentLatLng');
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: FlutterMap(
        mapController: MapController(),
        options: MapOptions(
          initialCenter: LatLng(34.03088, -118.28213),
          //center: LatLng(45.720405218, 14.406593302),
          initialZoom: 20,
        ),
        children: [
          TileLayer(
              urlTemplate: "https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png",
              subdomains: ['a', 'b', 'c']),
          loadingData
              ? const Center(child: CircularProgressIndicator())
              : PolygonLayer(
                  polygons: geoJsonParser.polygons,
                ),
          if (!loadingData) PolylineLayer(polylines: geoJsonParser.polylines),
          if (!loadingData) MarkerLayer(markers: geoJsonParser.markers),
          if (!loadingData) CircleLayer(circles: geoJsonParser.circles),
          MarkerLayer(markers: [
            Marker(
              point: currentLatLng,
              child: Icon(Icons.directions_bus,
                  size: 30.0, color: Color.fromARGB(255, 36, 7, 124)),
              //builder: (context) => FlutterLogo()
            )
          ]),
        ],
      ),
    );
  }
}
