# FOR DEVELOPMENT ONLY
# this is a simple server that proxies requests to the camera
# it is used to serve the index.html file and the capture image
# it is also used to serve the flash image

from flask import Flask, request, Response, send_file
import requests
import os

app = Flask(__name__)

CAMERA_URL = "http://172.20.10.2/capture"
FLASH_URL = "http://172.20.10.2/flash"


@app.route("/")
def index():
    return send_file("index.html")


@app.route("/flash")
def flash():
    # Forward any query parameters from the original request
    params = request.args.to_dict()

    try:
        # Make request to the camera
        resp = requests.get(FLASH_URL, params=params, stream=True)

        # Create response with same headers and status
        excluded_headers = [
            "content-encoding",
            "content-length",
            "transfer-encoding",
            "connection",
        ]
        headers = [
            (name, value)
            for (name, value) in resp.raw.headers.items()
            if name.lower() not in excluded_headers
        ]

        return Response(resp.content, resp.status_code, headers)

    except requests.RequestException as e:
        return str(e), 500


@app.route("/capture")
def proxy_capture():
    # Forward any query parameters from the original request
    params = request.args.to_dict()

    try:
        # Make request to the camera
        resp = requests.get(CAMERA_URL, params=params, stream=True)

        # Create response with same headers and status
        excluded_headers = [
            "content-encoding",
            "content-length",
            "transfer-encoding",
            "connection",
        ]
        headers = [
            (name, value)
            for (name, value) in resp.raw.headers.items()
            if name.lower() not in excluded_headers
        ]

        return Response(resp.content, resp.status_code, headers)

    except requests.RequestException as e:
        return str(e), 500


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=3009, debug=True)
