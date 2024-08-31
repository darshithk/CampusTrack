import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart' as mqtt;
import 'package:mqtt_client/mqtt_server_client.dart' as mqtt;
import 'package:mqtt_client/mqtt_client.dart';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/services.dart';

void main() => runApp(MyApp());

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'MQTT App',
      home: MQTTView(),
    );
  }
}

class MQTTView extends StatefulWidget {
  @override
  State<StatefulWidget> createState() {
    return _MQTTViewState();
  }
}

class _MQTTViewState extends State<MQTTView> {
  final String topic = "gw/#"; // Your topic
  late mqtt.MqttServerClient client;
  String receivedMessage = '';

  @override
  void initState() {
    super.initState();
    _connect();
  }

  void _connect() async {
    client = mqtt.MqttServerClient(
        'a3dt6q7niaetkx-ats.iot.us-east-2.amazonaws.com', 'basicPubSub');

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
    client.port = 8883; // AWS IoT uses port 8883 for MQTT
    client.secure = true;
    client.logging(on: true);

    // Connect to the client
    try {
      await client.connect();
    } catch (e) {
      print('Exception: $e');
      client.disconnect();
    }

    // Check if connection is successful
    if (client.connectionStatus?.state == mqtt.MqttConnectionState.connected) {
      print('AWS IoT Connected');
      client.subscribe(topic, mqtt.MqttQos.atLeastOnce);
      print("subscribed");
    } else {
      print(
          'ERROR AWS IoT Connection failed - disconnecting, status is ${client.connectionStatus}');
      client.disconnect();
    }
    final builder = MqttClientPayloadBuilder();
    builder.addString('Hello from mqtt_client');
    print("published");
    // Handle incoming messages
    client.updates
        ?.listen((List<mqtt.MqttReceivedMessage<mqtt.MqttMessage>> c) {
      print("here");
      final mqtt.MqttPublishMessage recMess =
          c[0].payload as mqtt.MqttPublishMessage;
      final String message = mqtt.MqttPublishPayload.bytesToStringAsString(
          recMess.payload.message);

      setState(() {
        receivedMessage = message;
      });

      print('Received message: $message');
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('MQTT App'),
      ),
      body: Center(
        child: Text('Received Message: $receivedMessage'),
      ),
    );
  }

  @override
  void dispose() {
    client.disconnect();
    super.dispose();
  }
}
