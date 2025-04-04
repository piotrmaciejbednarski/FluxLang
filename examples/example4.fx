// Struct with predefined objects
import "std.fx" as std;

using std.io.output;

object Point
{
    def __init(int x, int y) -> void
    {
        this.x = x;
        this.y = y;
        return;
    };
};

Point(1, 2){} p1;
Point(3, 4){} p2;

struct Line
{
    p1;
    p2;
};

def main() -> !void
{
    Line line;
    print(i"Line from ({}, {}) to ({}, {})":{line.p1.x; line.p1.y; line.p2.x; line.p2.y;});
    return 0;
};