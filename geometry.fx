import "math.fx";

namespace std {
    class Geometry {
        // Point in 2D/3D space
        struct Point2D {
            float{64} x, y;
        };
        
        struct Point3D {
            float{64} x, y, z;
        };
        
        // Line segment in 2D/3D
        struct Line2D {
            Point2D start, end;
        };
        
        struct Line3D {
            Point3D start, end;
        };
        
        // Circle and sphere
        struct Circle {
            Point2D center;
            float{64} radius;
        };
        
        struct Sphere {
            Point3D center;
            float{64} radius;
        };
        
        // Rectangle and box
        struct Rectangle {
            Point2D topLeft;
            float{64} width, height;
        };
        
        struct Box {
            Point3D minPoint;  // Minimum corner point
            Point3D maxPoint;  // Maximum corner point
        };
        
        // Triangle in 2D/3D
        struct Triangle2D {
            Point2D a, b, c;
        };
        
        struct Triangle3D {
            Point3D a, b, c;
        };

        object Points {
            // Create points
            Point2D create2D(float{64} x, float{64} y) {
                Point2D p;
                p.x = x;
                p.y = y;
                return p;
            };
            
            Point3D create3D(float{64} x, float{64} y, float{64} z) {
                Point3D p;
                p.x = x;
                p.y = y;
                p.z = z;
                return p;
            };
            
            // Calculate distance between points
            float{64} distance2D(Point2D a, Point2D b) {
                float{64} dx = b.x - a.x;
                float{64} dy = b.y - a.y;
                return Math.Exponential.sqrt(dx * dx + dy * dy);
            };
            
            float{64} distance3D(Point3D a, Point3D b) {
                float{64} dx = b.x - a.x;
                float{64} dy = b.y - a.y;
                float{64} dz = b.z - a.z;
                return Math.Exponential.sqrt(dx * dx + dy * dy + dz * dz);
            };
            
            // Convert between 2D and 3D points
            Point3D to3D(Point2D p, float{64} z = 0.0) {
                return create3D(p.x, p.y, z);
            };
            
            Point2D to2D(Point3D p) {
                return create2D(p.x, p.y);
            };
        };

        object Lines {
            // Create lines
            Line2D create2D(Point2D start, Point2D end) {
                Line2D line;
                line.start = start;
                line.end = end;
                return line;
            };
            
            Line3D create3D(Point3D start, Point3D end) {
                Line3D line;
                line.start = start;
                line.end = end;
                return line;
            };
            
            // Calculate line length
            float{64} length2D(Line2D line) {
                return Points.distance2D(line.start, line.end);
            };
            
            float{64} length3D(Line3D line) {
                return Points.distance3D(line.start, line.end);
            };
            
            // Check if point is on line segment
            bool containsPoint2D(Line2D line, Point2D point, float{64} epsilon = 0.001) {
                float{64} d1 = Points.distance2D(line.start, point);
                float{64} d2 = Points.distance2D(point, line.end);
                float{64} lineLen = length2D(line);
                return Math.BasicOps.abs(d1 + d2 - lineLen) < epsilon;
            };
            
            // Find intersection point of two lines
            Point2D intersection2D(Line2D a, Line2D b) {
                float{64} x1 = a.start.x, y1 = a.start.y;
                float{64} x2 = a.end.x, y2 = a.end.y;
                float{64} x3 = b.start.x, y3 = b.start.y;
                float{64} x4 = b.end.x, y4 = b.end.y;
                
                float{64} denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
                if (Math.BasicOps.abs(denom) < 0.001) {
                    // Lines are parallel
                    return Points.create2D(Math.Constants.NAN, Math.Constants.NAN);
                };
                
                float{64} t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
                return Points.create2D(
                    x1 + t * (x2 - x1),
                    y1 + t * (y2 - y1)
                );
            };
        };

        object Circles {
            // Create circle
            Circle create(Point2D center, float{64} radius) {
                Circle c;
                c.center = center;
                c.radius = radius;
                return c;
            };
            
            // Calculate area
            float{64} area(Circle circle) {
                return Math.Constants.PI * circle.radius * circle.radius;
            };
            
            // Calculate circumference
            float{64} circumference(Circle circle) {
                return 2.0 * Math.Constants.PI * circle.radius;
            };
            
            // Check if point is inside circle
            bool containsPoint(Circle circle, Point2D point) {
                return Points.distance2D(circle.center, point) <= circle.radius;
            };
            
            // Find intersection points of two circles
            Point2D[] intersectionPoints(Circle a, Circle b) {
                float{64} d = Points.distance2D(a.center, b.center);
                
                // Circles don't intersect or are identical
                if (d > a.radius + b.radius || d < Math.BasicOps.abs(a.radius - b.radius)) {
                    return [];
                };
                
                float{64} dx = b.center.x - a.center.x;
                float{64} dy = b.center.y - a.center.y;
                
                // Calculate intersection points using circle intersection formula
                float{64} l = (a.radius * a.radius - b.radius * b.radius + d * d) / (2.0 * d);
                float{64} h = Math.Exponential.sqrt(a.radius * a.radius - l * l);
                
                float{64} x2 = a.center.x + (l * dx - h * dy) / d;
                float{64} y2 = a.center.y + (l * dy + h * dx) / d;
                float{64} x3 = a.center.x + (l * dx + h * dy) / d;
                float{64} y3 = a.center.y + (l * dy - h * dx) / d;
                
                return [Points.create2D(x2, y2), Points.create2D(x3, y3)];
            };
        };

        object Triangles {
            // Create triangles
            Triangle2D create2D(Point2D a, Point2D b, Point2D c) {
                Triangle2D t;
                t.a = a;
                t.b = b;
                t.c = c;
                return t;
            };
            
            Triangle3D create3D(Point3D a, Point3D b, Point3D c) {
                Triangle3D t;
                t.a = a;
                t.b = b;
                t.c = c;
                return t;
            };
            
            // Calculate triangle area
            float{64} area2D(Triangle2D tri) {
                return Math.BasicOps.abs(
                    (tri.b.x - tri.a.x) * (tri.c.y - tri.a.y) -
                    (tri.c.x - tri.a.x) * (tri.b.y - tri.a.y)
                ) / 2.0;
            };
            
            float{64} area3D(Triangle3D tri) {
                // Convert points to vectors
                Math.Vectors.Vector3 v1 = Math.Vectors.create(
                    tri.b.x - tri.a.x,
                    tri.b.y - tri.a.y,
                    tri.b.z - tri.a.z
                );
                
                Math.Vectors.Vector3 v2 = Math.Vectors.create(
                    tri.c.x - tri.a.x,
                    tri.c.y - tri.a.y,
                    tri.c.z - tri.a.z
                );
                
                // Area is half the magnitude of cross product
                Math.Vectors.Vector3 cross = Math.Vectors.cross(v1, v2);
                return Math.Vectors.magnitude(cross) / 2.0;
            };
            
            // Check if point is inside triangle
            bool containsPoint2D(Triangle2D tri, Point2D p) {
                // Using barycentric coordinates
                float{64} denominator = ((tri.b.y - tri.c.y) * (tri.a.x - tri.c.x) +
                                       (tri.c.x - tri.b.x) * (tri.a.y - tri.c.y));
                
                float{64} a = ((tri.b.y - tri.c.y) * (p.x - tri.c.x) +
                              (tri.c.x - tri.b.x) * (p.y - tri.c.y)) / denominator;
                float{64} b = ((tri.c.y - tri.a.y) * (p.x - tri.c.x) +
                              (tri.a.x - tri.c.x) * (p.y - tri.c.y)) / denominator;
                float{64} c = 1.0 - a - b;
                
                return a >= 0.0 && a <= 1.0 &&
                       b >= 0.0 && b <= 1.0 &&
                       c >= 0.0 && c <= 1.0;
            };
        };

        object Rectangles {
            // Create rectangle
            Rectangle create(Point2D topLeft, float{64} width, float{64} height) {
                Rectangle r;
                r.topLeft = topLeft;
                r.width = width;
                r.height = height;
                return r;
            };
            
            // Calculate area
            float{64} area(Rectangle rect) {
                return rect.width * rect.height;
            };
            
            // Calculate perimeter
            float{64} perimeter(Rectangle rect) {
                return 2.0 * (rect.width + rect.height);
            };
            
            // Check if point is inside rectangle
            bool containsPoint(Rectangle rect, Point2D point) {
                return point.x >= rect.topLeft.x &&
                       point.x <= rect.topLeft.x + rect.width &&
                       point.y >= rect.topLeft.y &&
                       point.y <= rect.topLeft.y + rect.height;
            };
            
            // Check if two rectangles intersect
            bool intersects(Rectangle a, Rectangle b) {
                return !(b.topLeft.x > a.topLeft.x + a.width ||
                        b.topLeft.x + b.width < a.topLeft.x ||
                        b.topLeft.y > a.topLeft.y + a.height ||
                        b.topLeft.y + b.height < a.topLeft.y);
            };
            
            // Get intersection rectangle of two rectangles
            Rectangle intersection(Rectangle a, Rectangle b) {
                float{64} x = Math.BasicOps.max(a.topLeft.x, b.topLeft.x);
                float{64} y = Math.BasicOps.max(a.topLeft.y, b.topLeft.y);
                float{64} w = Math.BasicOps.min(a.topLeft.x + a.width, b.topLeft.x + b.width) - x;
                float{64} h = Math.BasicOps.min(a.topLeft.y + a.height, b.topLeft.y + b.height) - y;
                
                if (w < 0.0 || h < 0.0) {
                    // No intersection
                    return create(Points.create2D(0.0, 0.0), 0.0, 0.0);
                };
                
                return create(Points.create2D(x, y), w, h);
            };
        };
    };
};
