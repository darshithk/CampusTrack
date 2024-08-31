import 'dart:io';

import 'package:flutter/services.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class AwsClient {
  late MqttServerClient _client;

  final String server = '<iot-core-id>.iot.us-east-2.amazonaws.com';
  final int port = 8883;

  AwsClient(String clientId) {
    _client = MqttServerClient.withPort(server, clientId, port,
        maxConnectionAttempts: 5);
  }

  Future<MqttClientConnectionStatus?> connect() async {
    _client.logging(on: true);
    _client.setProtocolV311();
    _client.secure = true;
    _client.autoReconnect = true;
    _client.keepAlivePeriod = 20;

    final context = SecurityContext(withTrustedRoots: true);
    context.setTrustedCertificatesBytes(
        (await rootBundle.load('certs/AmazonRootCA1.pem'))
            .buffer
            .asUint8List());
    context.useCertificateChainBytes(
        (await rootBundle.load('certs/thingsboard-dashboard.pem.crt'))
            .buffer
            .asUint8List());
    context.usePrivateKeyBytes(
        (await rootBundle.load('certs/thingsboard-dashboard-private.pem.key'))
            .buffer
            .asUint8List());
    _client.securityContext = context;
    return await _client.connect();
  }

  void subscribe(String topic, void Function(String, String) callback) {
    _client.subscribe(topic, MqttQos.atLeastOnce);
    _client.updates?.listen((List<MqttReceivedMessage<MqttMessage>> messages) {
      for (MqttReceivedMessage<MqttMessage> message in messages) {
        final MqttPublishMessage publishMessage =
            message.payload as MqttPublishMessage;
        final String payload = MqttPublishPayload.bytesToStringAsString(
            publishMessage.payload.message);
        callback(message.topic, payload);
      }
    });
  }
}
