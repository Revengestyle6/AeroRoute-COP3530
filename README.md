## AeroRoute Frontend + API Quick Start

The React frontend lives in [frontend](frontend) and calls your C++ algorithm functions through the Express API server in [server](server).

1. Build the backend executable (from the `backend` directory):
	- `cmake -S . -B build`
	- `cmake --build build --target aeroroute`
2. Start the API server (from the `server` directory):
	- `npm install`
	- `npm run dev`
3. Start the frontend (from the `frontend` directory):
	- `npm install`
	- `npm run dev`
4. Open `http://localhost:5173`

The API server runs on `http://localhost:3000` and invokes `backend/build/aeroroute`, which uses:
- `findallroutes(...)`
- `adjacency_list(...)`
- `dijkstras(...)`
- `a_star(...)`
