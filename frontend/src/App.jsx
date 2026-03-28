import { useEffect, useMemo, useState } from 'react';
import { geoAlbersUsa, geoPath } from 'd3-geo';
import { feature } from 'topojson-client';
import usAtlas from 'us-atlas/states-10m.json';

function PathMap({ coordinates, missingPathAirports }) {
  const width = 980;
  const height = 520;

  const states = useMemo(() => {
    return feature(usAtlas, usAtlas.objects.states).features;
  }, []);

  const { points, statePaths, missingProjectionAirports } = useMemo(() => {
    const projection = geoAlbersUsa();
    projection.fitExtent(
      [
        [18, 18],
        [width - 18, height - 18]
      ],
      {
        type: 'FeatureCollection',
        features: states
      }
    );

    const pathBuilder = geoPath(projection);
    const stateShapes = states.map((state) => pathBuilder(state)).filter(Boolean);

    const projectedPoints = [];
    const projectionMissing = [];

    for (const airport of coordinates) {
      const position = projection([airport.lon, airport.lat]);
      if (!position) {
        projectionMissing.push(airport.iata);
        continue;
      }

      projectedPoints.push({
        x: position[0],
        y: position[1],
        iata: airport.iata,
        city: airport.city || airport.iata
      });
    }

    return {
      points: projectedPoints,
      statePaths: stateShapes,
      missingProjectionAirports: projectionMissing
    };
  }, [coordinates, states]);

  const polylinePoints = points.map((point) => `${point.x},${point.y}`).join(' ');

  return (
    <div className="map-shell">
      <svg viewBox={`0 0 ${width} ${height}`} className="map-svg" role="img" aria-label="Route map">
        <rect x="0" y="0" width={width} height={height} fill="#9ac3e9" />
        {statePaths.map((shape, idx) => (
          <path key={`state-${idx}`} d={shape} className="us-state" />
        ))}

        {points.length >= 2 && (
          <polyline
            points={polylinePoints}
            stroke="#d33353"
            strokeWidth="4"
            strokeLinecap="round"
            strokeLinejoin="round"
            fill="none"
            strokeOpacity="0.86"
          />
        )}

        {points.map((point) => (
          <g key={`${point.iata}-${point.x}-${point.y}`}>
            <circle cx={point.x} cy={point.y} r="5.5" fill="#991f36" />
            <circle cx={point.x} cy={point.y} r="11" fill="#bf3550" fillOpacity="0.18" />
            <text x={point.x + 8} y={point.y - 8} className="airport-label">
              {point.iata} ({point.city})
            </text>
          </g>
        ))}
      </svg>

      {(missingPathAirports.length > 0 || missingProjectionAirports.length > 0) && (
        <p className="map-warning">
          Missing coordinates/projection for:{' '}
          {[...new Set([...missingPathAirports, ...missingProjectionAirports])].join(', ')}
        </p>
      )}
    </div>
  );
}

function ResultCard({ title, result }) {
  if (!result) {
    return (
      <div className="result-card empty">
        <h3>{title}</h3>
        <p>Not run yet.</p>
      </div>
    );
  }

  return (
    <div className="result-card">
      <h3>{title}</h3>
      <p>
        <strong>Runtime:</strong> {(result.elapsed_us / 1000).toFixed(3)} ms
      </p>
      <p>
        <strong>Distance:</strong> {result.distance.toLocaleString()} miles
      </p>
      <p>
        <strong>Path:</strong> {result.path.join(' -> ')}
      </p>
    </div>
  );
}

export default function App() {
  const [airports, setAirports] = useState([]);
  const [airportNames, setAirportNames] = useState({});
  const [origin, setOrigin] = useState('');
  const [destination, setDestination] = useState('');

  const [dijkstraResult, setDijkstraResult] = useState(null);
  const [aStarResult, setAStarResult] = useState(null);
  const [activePath, setActivePath] = useState([]);
  const [activePathAirports, setActivePathAirports] = useState([]);
  const [activePathMissing, setActivePathMissing] = useState([]);
  const [missingAirports, setMissingAirports] = useState([]);
  const [cityFallbackAirports, setCityFallbackAirports] = useState([]);

  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    async function loadAirports() {
      setError('');
      try {
        const response = await fetch('/api/airports');
        if (!response.ok) {
          throw new Error('Failed to load airports. Start the API server and build the backend first.');
        }

        const payload = await response.json();
        const list = payload.airports || [];
        setAirports(list);
        setAirportNames(payload.airport_names || {});
        setMissingAirports(payload.missing_airports || []);
        setCityFallbackAirports(payload.city_fallback_airports || []);

        if (list.length > 1) {
          setOrigin(list[0]);
          setDestination(list[1]);
        }
      } catch (err) {
        setError(err.message);
      }
    }

    loadAirports();
  }, []);

  async function runAlgorithm(algorithm) {
    if (!origin || !destination) {
      setError('Select both origin and destination.');
      return;
    }

    if (origin === destination) {
      setError('Origin and destination must be different airports.');
      return;
    }

    setLoading(true);
    setError('');

    try {
      const query = new URLSearchParams({ algorithm, origin, destination });
      const response = await fetch(`/api/route?${query.toString()}`);
      if (!response.ok) {
        const body = await response.json().catch(() => ({}));
        throw new Error(body.error || 'Unable to calculate route.');
      }

      const payload = await response.json();

      if (algorithm === 'dijkstra') {
        setDijkstraResult(payload);
      } else {
        setAStarResult(payload);
      }

      setActivePath(payload.coordinates || []);
      setActivePathAirports(payload.path || []);
      setActivePathMissing(payload.missing_path_airports || []);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <main className="page">
      <header className="title-block">
        <h1>AeroRoute Command Center</h1>
        <p>Compare Dijkstra and A* using your existing backend functions.</p>
      </header>

      <section className="layout-grid">
        <aside className="controls-panel">
          <div className="picker-wrap">
            <h2>Origin Airport</h2>
            <select size="12" value={origin} onChange={(event) => setOrigin(event.target.value)}>
              {airports.map((airport) => (
                <option key={`origin-${airport}`} value={airport}>
                  {airportNames[airport] ? `${airport} - ${airportNames[airport]}` : airport}
                </option>
              ))}
            </select>
          </div>

          <div className="picker-wrap">
            <h2>Destination Airport</h2>
            <select size="12" value={destination} onChange={(event) => setDestination(event.target.value)}>
              {airports.map((airport) => (
                <option key={`dest-${airport}`} value={airport}>
                  {airportNames[airport] ? `${airport} - ${airportNames[airport]}` : airport}
                </option>
              ))}
            </select>
          </div>

          <div className="algo-grid">
            <button disabled={loading} onClick={() => runAlgorithm('dijkstra')}>
              Search with Dijkstra
            </button>
            <button disabled={loading} onClick={() => runAlgorithm('astar')}>
              Search with A*
            </button>
          </div>

          {error && <p className="error-box">{error}</p>}
        </aside>

        <section className="content-panel">
          <div className="result-grid">
            <ResultCard title="Dijkstra" result={dijkstraResult} />
            <ResultCard title="A*" result={aStarResult} />
          </div>

          <div className="diagnostics-row">
            <p>
              <strong>Airports with city fallback coords:</strong>{' '}
              {cityFallbackAirports.length ? cityFallbackAirports.join(', ') : 'None'}
            </p>
            <p>
              <strong>Airports still missing coordinates:</strong>{' '}
              {missingAirports.length ? missingAirports.join(', ') : 'None'}
            </p>
          </div>

          <div className="path-row">
            <strong>All airports in selected path:</strong>{' '}
            {activePathAirports.length
              ? activePathAirports
                  .map((iata) =>
                    airportNames[iata] ? `${iata} (${airportNames[iata]})` : iata
                  )
                  .join(' -> ')
              : 'Run an algorithm to display path airports.'}
          </div>

          <PathMap coordinates={activePath} missingPathAirports={activePathMissing} />
        </section>
      </section>
    </main>
  );
}
