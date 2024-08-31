import 'dart:async';
import 'dart:convert';
import 'dart:developer';

import 'package:dashboard/aws_client.dart';
import 'package:dashboard/telemetry.dart';
import 'package:latlong2/latlong.dart';

class BusLocation {
  LatLng coordinates;
  BusLocation(this.coordinates);
}

class LocationService {
  static final LocationService _instance = LocationService._init();
  static LocationService get instance => _instance;

  final AwsClient _client = AwsClient('ee542-user-app');

  final _locationStreamController = StreamController<BusLocation>.broadcast();

  Stream<BusLocation> get locationStream => _locationStreamController.stream;

  LocationService._init();

  void start() async {
    await _startMqtt();
  }

  Future<void> _startMqtt() async {
    try {
      await _client.connect();
      _client.subscribe('gw/#', (topic, message) {
        log('Received message from $topic: $message');
        final telemetry = Telemetry(jsonDecode(message));
        _locationStreamController.sink.add(BusLocation(telemetry.coordinates));
      });
    } catch (e) {
      log('Failed to connect to AWS: $e');
    }
  }

  void stop() {
    // _client.disconnect();
  }
}
