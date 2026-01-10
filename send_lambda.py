from flask import Flask, request, send_from_directory
import serial
import time


SERIAL_PORT = '/dev/cu.usbserial-1140'
BAUD = 115200

ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)
time.sleep(2) 

app = Flask(__name__, static_folder=".")

@app.route("/")
def index():
    return send_from_directory(".", "index.html")

@app.route("/lambda", methods=["POST"])
def recv_lambda():
    data = request.get_json(force=True)
    v = float(data.get("lambda_m", 0.0))
    v = max(0.0, min(1.0, v))

    ser.write(f"{v:.3f}\n".encode())
    print("sent lambda_m:", v)

    return "ok"

if __name__ == "__main__":
    print("🚀 Flask starting on http://127.0.0.1:5000")
    app.run(host="127.0.0.1", port=5000, debug=False)
