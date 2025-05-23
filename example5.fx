namespace http {
    // HTTP Request object
    object Request {
        string method;
        string path;
        dict headers;
        string body;
    };

    // HTTP Response object
    object Response {
        int status_code;
        dict headers;
        string body;

        def set_header(string key, string value) -> void;
        def send(string body) -> void;
        def json(dict data) -> void;
    };

    // Route handler type
    type Handler = def(Request, Response) -> void;

    // Virtual Server object
    object Server {
        def __init() -> void;
        def use(Handler middleware) -> void;
        def get(string path, Handler handler) -> void;
        def post(string path, Handler handler) -> void;
        def put(string path, Handler handler) -> void;
        def delete(string path, Handler handler) -> void;
        def simulate_request(string method, string path, dict headers, string body) -> Response;
    };
};

// Request implementation
object http::Request {
    def __init(string method, string path, dict headers, string body) -> void {
        this.method = method;
        this.path = path;
        this.headers = headers;
        this.body = body;
    };
};

// Response implementation
object http::Response {
    def __init() -> void {
        this.status_code = 200;
        this.headers = {"Content-Type": "text/plain"};
        this.body = "";
    };

    def set_header(string key, string value) -> void {
        this.headers[key] = value;
    };

    def send(string body) -> void {
        this.body = body;
    };

    def json(dict data) -> void {
        this.set_header("Content-Type", "application/json");
        // Simple JSON serialization
        string json_str = "{";
        bool first = true;
        for (key, value in data) {
            if (not first) {
                json_str += ",";
            };
            json_str += i"\"{}\":\"{}\"":{key; value;};
            first = false;
        };
        json_str += "}";
        this.send(json_str);
    };
};

// Server implementation
object http::Server {
    def __init() -> void {
        this.middlewares = [];
        this.routes = {};
    };

    Handler[] middlewares;
    dict routes;  // Key: "METHOD PATH", Value: Handler

    def use(Handler middleware) -> void {
        this.middlewares.append(middleware);
    };

    def _add_route(string method, string path, Handler handler) -> void {
        string key = i"{} {}":{method; path;};
        this.routes[key] = handler;
    };

    def get(string path, Handler handler) -> void {
        this._add_route("GET", path, handler);
    };

    def post(string path, Handler handler) -> void {
        this._add_route("POST", path, handler);
    };

    def put(string path, Handler handler) -> void {
        this._add_route("PUT", path, handler);
    };

    def delete(string path, Handler handler) -> void {
        this._add_route("DELETE", path, handler);
    };

    def simulate_request(string method, string path, dict headers, string body) -> Response {
        Request{}(method, path, headers, body) req;
        Response{} res;

        // Run middlewares
        for (middleware in this.middlewares) {
            middleware(req, res);
            if (res.body not == "") {
                return res;  // Middleware handled the response
            };
        };

        // Find matching route
        string route_key = i"{} {}":{method; path;};
        if (route_key in this.routes) {
            Handler handler = this.routes[route_key];
            handler(req, res);
        } else {
            res.status_code = 404;
            res.send("404 Not Found");
        };

        return res;
    };
};

def logger(http::Request req, http::Response res) -> void {
    print(i"{} {} {}":{req.method; req.path; res.status_code;});
};

def home_handler(http::Request req, http::Response res) -> void {
    res.send("Welcome to Flux HTTP Server!");
};

def json_handler(http::Request req, http::Response res) -> void {
    res.json({
        "message": "Hello World",
        "status": "success"
    });
};

def main() -> int {
    http::Server{} server;

    // Add middleware
    server.use(logger);

    // Add routes
    server.get("/", home_handler);
    server.get("/api", json_handler);

    // Simulate requests
    http::Response res1 = server.simulate_request("GET", "/", {}, "");
    print(i"Response: {} {}":{res1.status_code; res1.body;});

    http::Response res2 = server.simulate_request("GET", "/api", {}, "");
    print(i"Response: {} {}":{res2.status_code; res2.body;});

    http::Response res3 = server.simulate_request("GET", "/notfound", {}, "");
    print(i"Response: {} {}":{res3.status_code; res3.body;});

    return 0;
};