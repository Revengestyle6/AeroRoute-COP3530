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

app.use(express.json());

app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", clientOrigin);
  res.header("Access-Control-Allow-Methods", "POST,OPTIONS");
  res.header("Access-Control-Allow-Headers", "Content-Type");

  if (req.method === "OPTIONS") {
    return res.sendStatus(204);
  }

  next();
});

const execFileAsync = promisify(execFile);

const normalizeAirportCode = (value: string) => value.trim().toUpperCase();

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
    const binaryPath = path.resolve(__dirname, "../../backend/build/aeroroute");

    const { stdout } = await execFileAsync(binaryPath, [
      normalizedSource,
      normalizedDestination,
    ]);

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
