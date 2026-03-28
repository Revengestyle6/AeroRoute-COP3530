import express from "express";

const app = express();
const port = Number(process.env.PORT) || 3000;
const clientOrigin = process.env.CLIENT_ORIGIN || "http://localhost:5173";

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

const normalizeAirportCode = (value: string) => value.trim().toUpperCase();

app.post("/pathfinding", (req, res) => {
  const { source, destination } = req.body as PathfindingRequestBody;

  if (!source || !destination) {
    return res.status(400).json({
      error: "source and destination are required.",
    });
  }

  const normalizedSource = normalizeAirportCode(source);
  const normalizedDestination = normalizeAirportCode(destination);

  // TODO: Replace these placeholder results with real calls into your C++ backend.
  const response: PathfindingResponse = {
    source: normalizedSource,
    destination: normalizedDestination,
    results: {
      dijkstra: {
        path: [],
        distance: 0,
        timeMs: 0,
      },
      astar: {
        path: [],
        distance: 0,
        timeMs: 0,
      },
    },
  };

  return res.status(501).json({
    message: "Pathfinding is not wired to the C++ backend yet.",
    ...response,
  });
});

app.listen(port, () => {
  console.log(`Server listening on http://localhost:${port}`);
});
