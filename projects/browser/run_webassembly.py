from http.server import \
    HTTPServer, SimpleHTTPRequestHandler
import os
import signal
import sys
import webbrowser


# signal handler to terminate the program
def signal_handler(signum, frame):
    print("stop web service")
    exit(0)


# custom http request handler for cross-origin
class CustomHTTPRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("cross-origin-embedder-policy", "require-corp")
        self.send_header("cross-origin-opener-policy", "same-origin")
        SimpleHTTPRequestHandler.end_headers(self)


# main entry point
def main():
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    if len(sys.argv) != 2:
        print("usage: python run_webassembly.py project_name")
        exit(0)

    page, _ = os.path.splitext(sys.argv[1])
    page += ".html"

    port = 8001

    with HTTPServer(("", port), CustomHTTPRequestHandler) as httpd:
        try:
            url = "http://localhost:{}/{}".format(port, page)
            print("start web service at {}".format(url))
            print("press ctrl + scroll to exit the browser from the shell")
            webbrowser.open(url)
            httpd.serve_forever()
        finally:
            httpd.shutdown()


if __name__ == "__main__":
    main()