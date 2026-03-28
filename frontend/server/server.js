import express from 'express';
import path from 'path';
import { execFile } from 'child_process';
import { fileURLToPath } from 'url';
import fs from 'fs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const backendRoot = path.resolve(repoRoot, 'backend');

const app = express();
const PORT = 3001;

app.use((req, res, next) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET,OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
  if (req.method === 'OPTIONS') {
    res.status(204).end();
    return;
  }
  next();
});

let cachedDataModel = null;

function parseCsvLine(line) {
  const out = [];
  let current = '';
  let inQuotes = false;

  for (let i = 0; i < line.length; i += 1) {
    const ch = line[i];

    if (inQuotes) {
      if (ch === '"') {
        if (i + 1 < line.length && line[i + 1] === '"') {
          current += '"';
          i += 1;
        } else {
          inQuotes = false;
        }
      } else {
        current += ch;
      }
      continue;
    }

    if (ch === '"') {
      inQuotes = true;
    } else if (ch === ',') {
      out.push(current);
      current = '';
    } else {
      current += ch;
    }
  }

  out.push(current);
  return out;
}

function valueAt(row, idx) {
  if (idx < 0 || idx >= row.length) {
    return '';
  }
  return String(row[idx] || '').trim();
}

function pickIndex(header, names) {
  for (const name of names) {
    const idx = header.indexOf(name);
    if (idx >= 0) {
      return idx;
    }
  }
  return -1;
}

function normalizeIata(value) {
  return String(value || '').trim().toUpperCase();
}

function normalizeCity(value) {
  const city = String(value || '').trim();
  if (!city) {
    return '';
  }

  const beforeComma = city.split(',')[0] || city;
  return beforeComma.trim().toUpperCase();
}

function displayCity(value) {
  const city = String(value || '').trim();
  if (!city) {
    return '';
  }

  const beforeComma = city.split(',')[0] || city;
  return beforeComma.trim();
}

function toRadians(value) {
  return (value * Math.PI) / 180;
}

function haversineMiles(a, b) {
  const dLat = toRadians(b.lat - a.lat);
  const dLon = toRadians(b.lon - a.lon);
  const lat1 = toRadians(a.lat);
  const lat2 = toRadians(b.lat);

  const h =
    Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(lat1) * Math.cos(lat2) * Math.sin(dLon / 2) * Math.sin(dLon / 2);
  const c = 2 * Math.asin(Math.sqrt(h));
  return 3959 * c;
}

function addEdge(graph, from, to, weight) {
  if (!graph[from]) {
    graph[from] = new Map();
  }

  const existing = graph[from].get(to);
  if (existing === undefined || weight < existing) {
    graph[from].set(to, weight);
  }
}

function reconstructPath(predecessor, source, destination) {
  if (source === destination) {
    return [source];
  }

  if (!predecessor[destination]) {
    return [];
  }

  const path = [destination];
  let current = destination;
  while (current !== source) {
    current = predecessor[current];
    if (!current) {
      return [];
    }
    path.push(current);
  }

  path.reverse();
  return path;
}

function shortestPathDijkstra(graph, source, destination) {
  const distances = { [source]: 0 };
  const predecessor = {};
  const queue = [[0, source]];

  while (queue.length > 0) {
    queue.sort((a, b) => a[0] - b[0]);
    const [currentCost, node] = queue.shift();
    if (currentCost !== distances[node]) {
      continue;
    }

    if (node === destination) {
      break;
    }

    const neighbors = graph[node] || new Map();
    for (const [nextNode, weight] of neighbors.entries()) {
      const tentative = currentCost + weight;
      if (distances[nextNode] === undefined || tentative < distances[nextNode]) {
        distances[nextNode] = tentative;
        predecessor[nextNode] = node;
        queue.push([tentative, nextNode]);
      }
    }
  }

  const path = reconstructPath(predecessor, source, destination);
  const distance = distances[destination] === undefined ? 0 : Math.round(distances[destination]);
  return { distance, path };
}

function shortestPathAStar(graph, coordsByIata, source, destination) {
  const gScore = { [source]: 0 };
  const predecessor = {};

  const heuristic = (node) => {
    const a = coordsByIata[node];
    const b = coordsByIata[destination];
    if (!a || !b) {
      return 0;
    }
    return haversineMiles(a, b);
  };

  const queue = [[heuristic(source), source]];

  while (queue.length > 0) {
    queue.sort((a, b) => a[0] - b[0]);
    const [fScore, node] = queue.shift();

    const expected = (gScore[node] ?? Number.POSITIVE_INFINITY) + heuristic(node);
    if (fScore > expected) {
      continue;
    }

    if (node === destination) {
      break;
    }

    const neighbors = graph[node] || new Map();
    for (const [nextNode, weight] of neighbors.entries()) {
      const tentative = gScore[node] + weight;
      if (gScore[nextNode] === undefined || tentative < gScore[nextNode]) {
        gScore[nextNode] = tentative;
        predecessor[nextNode] = node;
        queue.push([tentative + heuristic(nextNode), nextNode]);
      }
    }
  }

  const path = reconstructPath(predecessor, source, destination);
  const distance = gScore[destination] === undefined ? 0 : Math.round(gScore[destination]);
  return { distance, path };
}

function loadCsvRows(csvPath) {
  const raw = fs.readFileSync(csvPath, 'utf8');
  const lines = raw.split(/\r?\n/).filter(Boolean);
  if (lines.length < 2) {
    throw new Error(`CSV has no data rows: ${csvPath}`);
  }

  const header = parseCsvLine(lines[0]);
  const rows = [];
  for (let i = 1; i < lines.length; i += 1) {
    rows.push(parseCsvLine(lines[i]));
  }

  return { header, rows };
}

function loadDataModel() {
  if (cachedDataModel) {
    return cachedDataModel;
  }

  const pathToRoutes = path.join(backendRoot, 'flight_data.csv');
  const pathToCoords = path.join(backendRoot, 'accurate_airport_locations.csv');
  if (!fs.existsSync(pathToRoutes)) {
    throw new Error('Missing flight_data.csv');
  }
  if (!fs.existsSync(pathToCoords)) {
    throw new Error('Missing accurate_airport_locations.csv');
  }

  const locationCsv = loadCsvRows(pathToCoords);
  const routeCsv = loadCsvRows(pathToRoutes);

  const idxIata = pickIndex(locationCsv.header, ['IATA']);
  const idxLat = pickIndex(locationCsv.header, ['LATITUDE']);
  const idxLon = pickIndex(locationCsv.header, ['LONGITUDE']);
  const idxCity = pickIndex(locationCsv.header, ['CITY']);
  if (idxIata < 0 || idxLat < 0 || idxLon < 0 || idxCity < 0) {
    throw new Error('accurate_airport_locations.csv missing required columns');
  }

  const coordsByIata = {};
  const cityToCoords = {};
  for (const row of locationCsv.rows) {
    const iata = normalizeIata(valueAt(row, idxIata));
    const city = normalizeCity(valueAt(row, idxCity));
    const lat = Number(valueAt(row, idxLat));
    const lon = Number(valueAt(row, idxLon));

    if (!iata || Number.isNaN(lat) || Number.isNaN(lon)) {
      continue;
    }

    coordsByIata[iata] = { lat, lon, source: 'accurate_iata' };
    if (city && !cityToCoords[city]) {
      cityToCoords[city] = { lat, lon };
    }
  }

  const idxAirport1 = pickIndex(routeCsv.header, ['airport_1']);
  const idxAirport2 = pickIndex(routeCsv.header, ['airport_2']);
  const idxCity1 = pickIndex(routeCsv.header, ['city1', 'Geocoded_City1']);
  const idxCity2 = pickIndex(routeCsv.header, ['city2', 'Geocoded_City2']);
  const idxDistance = pickIndex(routeCsv.header, ['nsmiles']);
  if (idxAirport1 < 0 || idxAirport2 < 0) {
    throw new Error('flight_data.csv missing airport_1/airport_2 columns');
  }

  const airportsFromFlights = new Set();
  const airportToCity = {};
  const airportDisplayName = {};
  const graph = {};
  for (const row of routeCsv.rows) {
    const a1 = normalizeIata(valueAt(row, idxAirport1));
    const a2 = normalizeIata(valueAt(row, idxAirport2));
    const c1Raw = valueAt(row, idxCity1);
    const c2Raw = valueAt(row, idxCity2);
    const c1 = normalizeCity(c1Raw);
    const c2 = normalizeCity(c2Raw);
    const c1Display = displayCity(c1Raw);
    const c2Display = displayCity(c2Raw);
    const edgeDistance = Number(valueAt(row, idxDistance));

    if (a1) {
      airportsFromFlights.add(a1);
      if (c1 && !airportToCity[a1]) {
        airportToCity[a1] = c1;
      }
      if (c1Display && !airportDisplayName[a1]) {
        airportDisplayName[a1] = c1Display;
      }
    }

    if (a2) {
      airportsFromFlights.add(a2);
      if (c2 && !airportToCity[a2]) {
        airportToCity[a2] = c2;
      }
      if (c2Display && !airportDisplayName[a2]) {
        airportDisplayName[a2] = c2Display;
      }
    }

    if (a1 && a2) {
      const weight = Number.isFinite(edgeDistance) && edgeDistance > 0 ? edgeDistance : 1;
      addEdge(graph, a1, a2, weight);
      addEdge(graph, a2, a1, weight);
    }
  }

  const cityFallbackAirports = [];
  const unresolvedAirports = [];
  for (const airport of airportsFromFlights) {
    if (coordsByIata[airport]) {
      continue;
    }

    const city = airportToCity[airport] || '';
    if (city && cityToCoords[city]) {
      const fallback = cityToCoords[city];
      coordsByIata[airport] = {
        lat: fallback.lat,
        lon: fallback.lon,
        source: 'city_fallback'
      };
      cityFallbackAirports.push(airport);
    } else {
      unresolvedAirports.push(airport);
    }
  }

  const airports = Array.from(airportsFromFlights).sort();
  for (const iata of airports) {
    if (!airportDisplayName[iata]) {
      airportDisplayName[iata] = iata;
    }
  }
  unresolvedAirports.sort();
  cityFallbackAirports.sort();

  cachedDataModel = {
    airports,
    airportDisplayName,
    coordsByIata,
    unresolvedAirports,
    cityFallbackAirports,
    graph
  };

  return cachedDataModel;
}

function coordinateMapFromPayload(payload) {
  const map = {};
  const coordinates = Array.isArray(payload.coordinates) ? payload.coordinates : [];
  for (const entry of coordinates) {
    if (!entry || !entry.iata) {
      continue;
    }

    const iata = normalizeIata(entry.iata);
    const lat = Number(entry.lat);
    const lon = Number(entry.lon);
    if (!iata || Number.isNaN(lat) || Number.isNaN(lon)) {
      continue;
    }

    map[iata] = { lat, lon, source: 'algorithm_output' };
  }

  return map;
}

function enrichRoutePayload(payload) {
  const data = loadDataModel();
  const path = Array.isArray(payload.path) ? payload.path.map((x) => normalizeIata(x)) : [];
  const baseCoords = coordinateMapFromPayload(payload);

  const coordinates = [];
  const missingPathAirports = [];

  for (const iata of path) {
    const fromPayload = baseCoords[iata];
    const fromModel = data.coordsByIata[iata];
    const point = fromPayload || fromModel;

    if (!point) {
      missingPathAirports.push(iata);
      continue;
    }

    coordinates.push({
      iata,
      lat: point.lat,
      lon: point.lon,
      source: point.source,
      city: data.airportDisplayName[iata] || iata
    });
  }

  return {
    ...payload,
    path,
    coordinates,
    missing_path_airports: missingPathAirports,
    missing_airports_global: data.unresolvedAirports,
    city_fallback_airports: data.cityFallbackAirports
  };
}

function mockRoute(algorithm, origin, destination) {
  const { graph, coordsByIata } = loadDataModel();
  const started = process.hrtime.bigint();
  const solverResult =
    algorithm === 'astar'
      ? shortestPathAStar(graph, coordsByIata, origin, destination)
      : shortestPathDijkstra(graph, origin, destination);
  const ended = process.hrtime.bigint();

  const elapsedUs = Number((ended - started) / 1000n);
  const distance = solverResult.distance;
  const path = solverResult.path;

  const coordinates = [];
  for (const airport of path) {
    const point = coordsByIata[airport];
    if (point) {
      coordinates.push({ iata: airport, lat: point.lat, lon: point.lon, source: point.source });
    }
  }

  return {
    algorithm,
    source: origin,
    destination,
    distance,
    elapsed_us: elapsedUs,
    path,
    coordinates,
    mock: true
  };
}

function resolveApiExecutable() {
  const candidates = [
    path.join(backendRoot, 'cmake-build-debug', 'aeroroute_api.exe'),
    path.join(backendRoot, 'cmake-build-debug', 'aeroroute_api'),
    path.join(backendRoot, 'aeroroute_api.exe'),
    path.join(backendRoot, 'aeroroute_api')
  ];

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) {
      return candidate;
    }
  }

  return null;
}

function runCli(args) {
  return new Promise((resolve, reject) => {
    const executable = resolveApiExecutable();
    if (!executable) {
      reject(
        new Error(
          'Could not find aeroroute_api executable. Build backend target aeroroute_api first.'
        )
      );
      return;
    }

    execFile(executable, args, { cwd: path.dirname(executable) }, (error, stdout, stderr) => {
      if (error) {
        reject(new Error(stderr || error.message));
        return;
      }

      try {
        resolve(JSON.parse(stdout));
      } catch {
        reject(new Error(`Invalid JSON from aeroroute_api: ${stdout}`));
      }
    });
  });
}

app.get('/api/airports', async (_req, res) => {
  try {
    const model = loadDataModel();
    res.json({
      airports: model.airports,
      airport_names: model.airportDisplayName,
      missing_airports: model.unresolvedAirports,
      city_fallback_airports: model.cityFallbackAirports
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/missing-airports', (_req, res) => {
  try {
    const model = loadDataModel();
    res.json({
      missing_airports: model.unresolvedAirports,
      city_fallback_airports: model.cityFallbackAirports
    });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/route', async (req, res) => {
  const algorithm = String(req.query.algorithm || '').toLowerCase();
  const origin = String(req.query.origin || '').toUpperCase().trim();
  const destination = String(req.query.destination || '').toUpperCase().trim();

  if (!origin || !destination) {
    res.status(400).json({ error: 'Both origin and destination are required.' });
    return;
  }

  if (algorithm !== 'dijkstra' && algorithm !== 'astar') {
    res.status(400).json({ error: 'algorithm must be dijkstra or astar' });
    return;
  }

  try {
    const payload = await runCli(['route', algorithm, origin, destination]);
    res.json(enrichRoutePayload(payload));
  } catch (err) {
    if (String(err.message || '').includes('Could not find aeroroute_api executable')) {
      res.json(enrichRoutePayload(mockRoute(algorithm, origin, destination)));
      return;
    }
    res.status(500).json({ error: err.message });
  }
});

app.listen(PORT, () => {
  console.log(`AeroRoute API bridge listening at http://localhost:${PORT}`);
});
