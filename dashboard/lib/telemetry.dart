import 'dart:developer';

import 'package:latlong2/latlong.dart';

class Telemetry {
  String deviceId;
  String deviceName;
  DateTime timestamp;
  LatLng coordinates;
  double speed;
  Telemetry(dynamic data)
      : deviceId = '',
        deviceName = '',
        timestamp = DateTime.now(),
        coordinates = const LatLng(0, 0),
        speed = 0 {
    final json = Map.from(data);
    try {
      deviceName = json.containsKey('device_id') ? json['device_id'] : '';
      timestamp = json.containsKey('timestamp')
          ? DateTime.fromMillisecondsSinceEpoch(json['timestamp'])
          : DateTime.now();
      // speed = json.containsKey('speed') ? double.tryParse(json['speed']) ?? 0.0 : 0.0;
      if (json.containsKey('latitude') && json.containsKey('longitude')) {
        coordinates = LatLng(json['latitude'], json['longitude']);
      }
    } catch (e) {
      log(e.toString());
    }
  }
}
