import express from "express";
import { execFile } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import path from "node:path";
import { promisify } from "node:util";

// creates express app
const app = express();
const port = Number(process.env.PORT) || 3000;
const clientOrigin = process.env.CLIENT_ORIGIN || "http://localhost:5173";
const repoRoot = path.resolve(__dirname, "../..");
const backendRoot = path.join(repoRoot, "backend");
const backendBuildDir = path.join(backendRoot, "build");
const backendDataDir = path.join(backendRoot, "src", "data");
const backendBinaryPath = path.join(
  backendBuildDir,
  process.platform === "win32" ? "aeroroute.exe" : "aeroroute",
);

// define types for request and response
type PathfindingRequestBody = {
  source?: string;
  destination?: string;
};

type AlgorithmResult = {
  path: string[];
  distance: number;
  timeMs: number;
};

type PathfindingResponse = {
  source: string;
  destination: string;
  results: {
    dijkstra: AlgorithmResult;
    astar: AlgorithmResult;
  };
};

type RouteAlgorithm = keyof PathfindingResponse["results"];

type CsvRows = {
  header: string[];
  rows: string[][];
};

type Coordinate = {
  lat: number;
  lon: number;
  source: "accurate_iata" | "city_fallback";
};

type RouteCoordinate = Coordinate & {
  iata: string;
  city: string;
};

type DataModel = {
  airports: string[];
  airportDisplayName: Record<string, string>;
  coordsByIata: Record<string, Coordinate>;
  unresolvedAirports: string[];
  cityFallbackAirports: string[];
};

type RoutePayload = {
  algorithm: RouteAlgorithm;
  source: string;
  destination: string;
  distance: number;
  elapsed_us: number;
  path: string[];
  coordinates: RouteCoordinate[];
  missing_path_airports: string[];
  missing_airports_global: string[];
  city_fallback_airports: string[];
};

let cachedDataModel: DataModel | null = null;

// parse JSON requests
app.use(express.json());

// CORS
app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", clientOrigin);
  res.header("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.header("Access-Control-Allow-Headers", "Content-Type");

  if (req.method === "OPTIONS") {
    return res.sendStatus(204);
  }

  next();
});

// allows us to run C++ executable via Node
const execFileAsync = promisify(execFile);

// converts to uppercase and trims whitespace
const normalizeAirportCode = (value: string) => value.trim().toUpperCase();

const normalizeCity = (value: string) => {
  const city = value.trim();
  if (!city) {
    return "";
  }

  const beforeComma = city.split(",")[0] || city;
  return beforeComma.trim().toUpperCase();
};

const displayCity = (value: string) => {
  const city = value.trim();
  if (!city) {
    return "";
  }

  const beforeComma = city.split(",")[0] || city;
  return beforeComma.trim();
};

const parseCsvLine = (line: string) => {
  const output: string[] = [];
  let current = "";
  let inQuotes = false;

  for (let index = 0; index < line.length; index += 1) {
    const character = line[index];

    if (inQuotes) {
      if (character === "\"") {
        if (index + 1 < line.length && line[index + 1] === "\"") {
          current += "\"";
          index += 1;
        } else {
          inQuotes = false;
        }
      } else {
        current += character;
      }
      continue;
    }

    if (character === "\"") {
      inQuotes = true;
    } else if (character === ",") {
      output.push(current);
      current = "";
    } else {
      current += character;
    }
  }

  output.push(current);
  return output;
};

const loadCsvRows = (csvPath: string): CsvRows => {
  const raw = readFileSync(csvPath, "utf8");
  const lines = raw.split(/\r?\n/).filter(Boolean);

  if (lines.length < 2) {
    throw new Error(`CSV has no data rows: ${csvPath}`);
  }

  return {
    header: parseCsvLine(lines[0]),
    rows: lines.slice(1).map(parseCsvLine),
  };
};

const valueAt = (row: string[], index: number) => {
  if (index < 0 || index >= row.length) {
    return "";
  }

  return String(row[index] || "").trim();
};

const pickIndex = (header: string[], names: string[]) => {
  for (const name of names) {
    const index = header.indexOf(name);
    if (index >= 0) {
      return index;
    }
  }

  return -1;
};

const loadDataModel = (): DataModel => {
  if (cachedDataModel) {
    return cachedDataModel;
  }

  const routesPath = path.join(backendDataDir, "flight_data.csv");
  const coordinatesPath = path.join(backendDataDir, "accurate_airport_locations.csv");

  if (!existsSync(routesPath)) {
    throw new Error(`Missing flight data CSV: ${routesPath}`);
  }

  if (!existsSync(coordinatesPath)) {
    throw new Error(`Missing airport coordinates CSV: ${coordinatesPath}`);
  }

  const locationCsv = loadCsvRows(coordinatesPath);
  const routeCsv = loadCsvRows(routesPath);

  const iataIndex = pickIndex(locationCsv.header, ["IATA"]);
  const latitudeIndex = pickIndex(locationCsv.header, ["LATITUDE"]);
  const longitudeIndex = pickIndex(locationCsv.header, ["LONGITUDE"]);
  const cityIndex = pickIndex(locationCsv.header, ["CITY"]);

  if (iataIndex < 0 || latitudeIndex < 0 || longitudeIndex < 0 || cityIndex < 0) {
    throw new Error("accurate_airport_locations.csv missing required columns");
  }

  const coordsByIata: Record<string, Coordinate> = {};
  const cityToCoords: Record<string, { lat: number; lon: number }> = {};

  for (const row of locationCsv.rows) {
    const iata = normalizeAirportCode(valueAt(row, iataIndex));
    const city = normalizeCity(valueAt(row, cityIndex));
    const lat = Number(valueAt(row, latitudeIndex));
    const lon = Number(valueAt(row, longitudeIndex));

    if (!iata || Number.isNaN(lat) || Number.isNaN(lon)) {
      continue;
    }

    coordsByIata[iata] = { lat, lon, source: "accurate_iata" };

    if (city && !cityToCoords[city]) {
      cityToCoords[city] = { lat, lon };
    }
  }

  const airportOneIndex = pickIndex(routeCsv.header, ["airport_1"]);
  const airportTwoIndex = pickIndex(routeCsv.header, ["airport_2"]);
  const cityOneIndex = pickIndex(routeCsv.header, ["city1", "Geocoded_City1"]);
  const cityTwoIndex = pickIndex(routeCsv.header, ["city2", "Geocoded_City2"]);

  if (airportOneIndex < 0 || airportTwoIndex < 0) {
    throw new Error("flight_data.csv missing airport_1/airport_2 columns");
  }

  const airportsFromFlights = new Set<string>();
  const airportToCity: Record<string, string> = {};
  const airportDisplayName: Record<string, string> = {};

  for (const row of routeCsv.rows) {
    const airportOne = normalizeAirportCode(valueAt(row, airportOneIndex));
    const airportTwo = normalizeAirportCode(valueAt(row, airportTwoIndex));
    const cityOneRaw = valueAt(row, cityOneIndex);
    const cityTwoRaw = valueAt(row, cityTwoIndex);
    const cityOne = normalizeCity(cityOneRaw);
    const cityTwo = normalizeCity(cityTwoRaw);
    const cityOneDisplay = displayCity(cityOneRaw);
    const cityTwoDisplay = displayCity(cityTwoRaw);

    if (airportOne) {
      airportsFromFlights.add(airportOne);

      if (cityOne && !airportToCity[airportOne]) {
        airportToCity[airportOne] = cityOne;
      }

      if (cityOneDisplay && !airportDisplayName[airportOne]) {
        airportDisplayName[airportOne] = cityOneDisplay;
      }
    }

    if (airportTwo) {
      airportsFromFlights.add(airportTwo);

      if (cityTwo && !airportToCity[airportTwo]) {
        airportToCity[airportTwo] = cityTwo;
      }

      if (cityTwoDisplay && !airportDisplayName[airportTwo]) {
        airportDisplayName[airportTwo] = cityTwoDisplay;
      }
    }
  }

  const cityFallbackAirports: string[] = [];
  const unresolvedAirports: string[] = [];

  for (const airport of airportsFromFlights) {
    if (coordsByIata[airport]) {
      continue;
    }

    const city = airportToCity[airport] || "";
    const fallback = city ? cityToCoords[city] : undefined;

    if (fallback) {
      coordsByIata[airport] = {
        lat: fallback.lat,
        lon: fallback.lon,
        source: "city_fallback",
      };
      cityFallbackAirports.push(airport);
    } else {
      unresolvedAirports.push(airport);
    }
  }

  const airports = Array.from(airportsFromFlights).sort();

  for (const airport of airports) {
    if (!airportDisplayName[airport]) {
      airportDisplayName[airport] = airport;
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
  };

  return cachedDataModel;
};

const runPathfindingBinary = async (
  source: string,
  destination: string,
): Promise<PathfindingResponse> => {
  if (!existsSync(backendBinaryPath)) {
    throw new Error("C++ backend binary not found. Build backend first.");
  }

  const { stdout, stderr } = await execFileAsync(
    backendBinaryPath,
    [source, destination],
    {
      cwd: backendBuildDir,
      maxBuffer: 1024 * 1024,
    },
  );

  console.log("C++ stdout:", stdout);

  if (stderr) {
    console.error("C++ stderr:", stderr);
  }

  try {
    return {
      source,
      destination,
      results: JSON.parse(stdout) as PathfindingResponse["results"],
    };
  } catch {
    throw new Error(`Invalid JSON from C++ backend: ${stdout}`);
  }
};

const buildRoutePayload = (
  algorithm: RouteAlgorithm,
  source: string,
  destination: string,
  result: AlgorithmResult,
): RoutePayload => {
  const dataModel = loadDataModel();
  const pathAirports = Array.isArray(result.path)
    ? result.path.map(normalizeAirportCode)
    : [];

  const coordinates: RouteCoordinate[] = [];
  const missingPathAirports: string[] = [];

  for (const airport of pathAirports) {
    const point = dataModel.coordsByIata[airport];

    if (!point) {
      missingPathAirports.push(airport);
      continue;
    }

    coordinates.push({
      iata: airport,
      lat: point.lat,
      lon: point.lon,
      source: point.source,
      city: dataModel.airportDisplayName[airport] || airport,
    });
  }

  return {
    algorithm,
    source,
    destination,
    distance: result.distance,
    elapsed_us: Math.round(result.timeMs * 1000),
    path: pathAirports,
    coordinates,
    missing_path_airports: missingPathAirports,
    missing_airports_global: dataModel.unresolvedAirports,
    city_fallback_airports: dataModel.cityFallbackAirports,
  };
};

app.get("/api/airports", (_req, res) => {
  try {
    const dataModel = loadDataModel();

    return res.json({
      airports: dataModel.airports,
      airport_names: dataModel.airportDisplayName,
      missing_airports: dataModel.unresolvedAirports,
      city_fallback_airports: dataModel.cityFallbackAirports,
    });
  } catch (error) {
    return res.status(500).json({
      error: error instanceof Error ? error.message : String(error),
    });
  }
});

app.get("/api/missing-airports", (_req, res) => {
  try {
    const dataModel = loadDataModel();

    return res.json({
      missing_airports: dataModel.unresolvedAirports,
      city_fallback_airports: dataModel.cityFallbackAirports,
    });
  } catch (error) {
    return res.status(500).json({
      error: error instanceof Error ? error.message : String(error),
    });
  }
});

app.get("/api/route", async (req, res) => {
  const algorithm = String(req.query.algorithm || "").toLowerCase();
  const source = normalizeAirportCode(String(req.query.origin || ""));
  const destination = normalizeAirportCode(String(req.query.destination || ""));

  if (!source || !destination) {
    return res.status(400).json({
      error: "Both origin and destination are required.",
    });
  }

  if (algorithm !== "dijkstra" && algorithm !== "astar") {
    return res.status(400).json({
      error: "algorithm must be dijkstra or astar",
    });
  }

  try {
    const payload = await runPathfindingBinary(source, destination);

    return res.json(
      buildRoutePayload(algorithm, source, destination, payload.results[algorithm]),
    );
  } catch (error) {
    return res.status(500).json({
      error: error instanceof Error ? error.message : String(error),
    });
  }
});

// POST endpoint (returning results)
app.post("/pathfinding", async (req, res) => {
  const { source, destination } = req.body as PathfindingRequestBody;

  if (!source || !destination) {
    return res.status(400).json({
      error: "source and destination are required.",
    });
  }

  const normalizedSource = normalizeAirportCode(source);
  const normalizedDestination = normalizeAirportCode(destination);

  try {
    return res.json(
      await runPathfindingBinary(normalizedSource, normalizedDestination),
    );
  } catch (error) {
    return res.status(500).json({
      error: "Failed to call C++ backend.",
      details: error instanceof Error ? error.message : String(error),
    });
  }
});

app.listen(port, () => {
  console.log(`Server listening on http://localhost:${port}`);
});
