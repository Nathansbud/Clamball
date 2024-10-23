from http.server import HTTPServer, SimpleHTTPRequestHandler

PORT = 6813
DIRECTORY = "."

class RequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        # Print details of the GET request to the console
        print(f"Received GET request: Path = {self.path}, Headers = {self.headers}")
        
        # Call the parent class's method to handle the GET request as usual
        super().do_GET()

if __name__ == "__main__":
    # This is bad practice, as it accepts all inbound connections; for testing, do this though!
    with HTTPServer(("0.0.0.0", PORT), SimpleHTTPRequestHandler) as httpd:
        httpd.serve_forever()

