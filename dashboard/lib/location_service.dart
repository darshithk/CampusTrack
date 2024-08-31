

import 'dart:async';
import 'dart:convert';
import 'dart:developer';

import 'package:dashboard/aws_client.dart';
import 'package:dashboard/telemetry.dart';
import 'package:latlong2/latlong.dart';
import 'package:location/location.dart';

enum LocationSource {
  xdot,
  phone,
}

class BusLocation {
  LatLng coordinates;
  LocationSource source;
  BusLocation(this.coordinates, this.source);
}

class LocationService {
  static final LocationService _instance = LocationService._init();
  static LocationService get instance => _instance;

  final AwsClient _client = AwsClient('ee542-user-app');

  final _locationStreamController = StreamController<BusLocation>.broadcast();

  Stream<BusLocation> get locationStream => _locationStreamController.stream;

  final _userLocation = Location();

  LocationService._init();

  void start() async {
    await _startMqtt();

    try {
      await _startGps();
    } catch (e) {
      log('Failed to start GPS: $e');
    }
  }

  Future<void> _startMqtt() async {
    try {
      await _client.connect();
      _client.subscribe('gw/#', (topic, message) {
        log('Received message from $topic: $message');
        final telemetry = Telemetry(jsonDecode(message));
        _locationStreamController.sink.add(BusLocation(telemetry.coordinates, LocationSource.xdot));
      });
    } catch (e) {
      log('Failed to connect to AWS: $e');
    }
  }

  Future<void> _startGps() async {

    if (!await _userLocation.serviceEnabled()) {
      if (!await _userLocation.requestService()) {
        log('GPS service not enabled');
        return;
      }
    }

    if (await _userLocation.hasPermission() == PermissionStatus.denied) {
      if (await _userLocation.requestPermission() != PermissionStatus.granted) {
        log('GPS permission not granted');
        return;
      }
    }

    await _userLocation.changeSettings(
      accuracy: LocationAccuracy.high,
      interval: 1000,
      distanceFilter: 10,
    );


    Timer.periodic(const Duration(seconds: 1), (timer) async {
      final location = await _userLocation.getLocation();
      if (location.latitude != null && location.longitude != null) {
        final coordinates = LatLng(location.latitude!, location.longitude!);
        log('GPS location from phone: $coordinates');
        _locationStreamController.sink.add(BusLocation(coordinates, LocationSource.phone));
      }
    });

    _userLocation.onLocationChanged.listen((location) {
      final coordinates = LatLng(location.latitude!, location.longitude!);
      log('GPS location from phone stream: $coordinates');
      _locationStreamController.sink.add(BusLocation(coordinates, LocationSource.phone));
    });
  }



  void stop() {
    // _client.disconnect();
  }
  
}