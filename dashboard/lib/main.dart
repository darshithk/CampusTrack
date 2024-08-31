import 'dart:developer';

import 'package:dashboard/geojson.dart';
import 'package:dashboard/location_service.dart';
import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter_map_geojson/flutter_map_geojson.dart';
import 'package:latlong2/latlong.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Campus Track',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'USC C-Route'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  bool _loadingData = true;
  final _mapController = MapController();
  late LatLng _currentLatLng = const LatLng(34.03088, -118.28213);

  LocationSource _chosenDataSource = LocationSource.phone;

  final LocationService _locationService = LocationService.instance;

  final GeoJsonParser _geoJsonParser = GeoJsonParser(
    defaultMarkerColor: Colors.red,
    defaultPolygonBorderColor: Colors.red,
    defaultPolygonFillColor: Colors.red.withOpacity(0.1),
    defaultCircleMarkerColor: Colors.red.withOpacity(0.25),
  );

  @override
  void initState() {
    super.initState();

    _initMapData();

    _locationService.start();

    _locationService.locationStream.listen((event) {
      log('${DateTime.timestamp().millisecondsSinceEpoch} New location: ${event.coordinates}, source: ${event.source}');
      if (_chosenDataSource == event.source) {
        _setLocation(event.coordinates);
      }
    });
  }

  void _initMapData() {
    _geoJsonParser.setDefaultMarkerTapCallback(_handleMarkerTap);
    _geoJsonParser.filterFunction = _mapFilterFunction;
    _geoJsonParser.parseGeoJsonAsString(testGeoJson);
    setState(() {
      _loadingData = false;
    });
  }

  void _handleMarkerTap(Map<String, dynamic> map) {
    log('onTapMarkerFunction: $map');
  }

  bool _mapFilterFunction(Map<String, dynamic> properties) =>
      properties['section'].toString().contains('Point M-4') != true;

  void _setLocation(LatLng coordinates) {
    _mapController.move(coordinates, _mapController.camera.zoom);
    setState(() {
      _currentLatLng = coordinates;
    });
  }

  void _changeLocationSource() {
    setState(() {
      _chosenDataSource = _chosenDataSource == LocationSource.phone
          ? LocationSource.xdot
          : LocationSource.phone;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
        actions: [
         IconButton(
          icon: _chosenDataSource == LocationSource.phone
            ? const Icon(Icons.menu)
            : const Icon(Icons.menu_open),
        onPressed: _changeLocationSource,
      ),
        ],
      ),
      body: FlutterMap(
          mapController: _mapController,
          options: const MapOptions(
            initialCenter: LatLng(34.021880400280466, -118.2828916972758),
            initialZoom: 17,
          ),
          children: [
            TileLayer(
                urlTemplate: "https://tile.openstreetmap.org/{z}/{x}/{y}.png"),
            _loadingData
                ? const Center(child: CircularProgressIndicator())
                : PolygonLayer(polygons: _geoJsonParser.polygons),
            if (!_loadingData)
              PolylineLayer(polylines: _geoJsonParser.polylines),
            if (!_loadingData) MarkerLayer(markers: _geoJsonParser.markers),
            if (!_loadingData) CircleLayer(circles: _geoJsonParser.circles),
            MarkerLayer(markers: [
              Marker(
                point: _currentLatLng,
                child: const Icon(
                  Icons.directions_bus,
                  size: 30.0,
                  color: Color.fromARGB(255, 36, 7, 124),
                ),
              ),
              Marker(
                point: _currentLatLng,
                child: const Icon(
                  Icons.my_location,
                  size: 20.0,
                  color: Colors.blue,
                ),
              ),
            ],),
          ],),
    );
  }
}
