import os
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

from flask import Flask, request, jsonify

app = Flask(__name__)

_script_dir = Path(__file__).parent
DB_PATH = Path(os.getenv("DB_PATH", _script_dir / "readings.db"))
HOST    = os.getenv("HOST", "0.0.0.0")
PORT    = int(os.getenv("PORT", 5000))


def init_db() -> None:
    """Create the readings table if it does not exist.

    Schema is identical to AirQualityPiSensors so the same dashboard works.
    """
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS readings (
                id        INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT NOT NULL,
                pm1_0     REAL,
                pm2_5     REAL,
                pm10      REAL,
                temp      REAL,
                humidity  REAL,
                pressure  REAL,
                altitude  REAL
            )
        """)


@app.route("/health", methods=["GET"])
def health():
    """Quick liveness check — useful for verifying the receiver is reachable."""
    return jsonify({"status": "ok"}), 200


@app.route("/readings", methods=["POST"])
def post_reading():
    """Accept a JSON reading from the WeMos and store it in SQLite."""
    data = request.get_json(silent=True)
    if data is None:
        return jsonify({"error": "Request body must be valid JSON"}), 400

    ts = datetime.now(timezone.utc).isoformat()

    try:
        with sqlite3.connect(DB_PATH) as conn:
            conn.execute(
                "INSERT INTO readings "
                "(timestamp, pm1_0, pm2_5, pm10, temp, humidity, pressure, altitude) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                (
                    ts,
                    data.get("pm1_0"),
                    data.get("pm2_5"),
                    data.get("pm10"),
                    data.get("temperature"),
                    data.get("humidity"),
                    data.get("pressure"),
                    data.get("altitude"),
                ),
            )
    except sqlite3.Error as e:
        return jsonify({"error": str(e)}), 500

    print(f"[{ts}] Stored: {data}")
    return jsonify({"status": "created"}), 201


if __name__ == "__main__":
    init_db()
    print(f"[Receiver] Listening on http://{HOST}:{PORT}")
    print(f"[Receiver] Database: {DB_PATH}")
    from waitress import serve
    serve(app, host=HOST, port=PORT)
