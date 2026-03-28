import express from "express";
import { execFile } from "node:child_process";
import path from "node:path";
import { promisify } from "node:util";

// creates express app
const app = express();
const port = Number(process.env.PORT) || 3000;
const clientOrigin = process.env.CLIENT_ORIGIN || "http://localhost:5173";

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

// parse JSON requests
app.use(express.json());

// CORS
app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", clientOrigin);
  res.header("Access-Control-Allow-Methods", "POST,OPTIONS");
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
    // path to C++ executable
    const buildDir = path.resolve(__dirname, "../../backend/build");
    const binaryPath = path.join(buildDir, "aeroroute");

    // execute the C++ binary at binaryPath
    // (passes source and destination to C++ exec for main to run)
    const { stdout } = await execFileAsync(binaryPath, [
      normalizedSource,
      normalizedDestination,
    ], {
      cwd: buildDir,
    });
    console.log("C++ stdout:", stdout);

    // converts cpp output to JSON
    const results = JSON.parse(stdout);

    return res.json({
      source: normalizedSource,
      destination: normalizedDestination,
      results,
    });
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
