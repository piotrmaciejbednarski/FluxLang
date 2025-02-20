// Basic hello world example showing object definition and method calls
object Greeter {
    message: string = "Hello, World!";
    
    sayHello() {
        print(message);
    }
    
    greet(name: string) {
        print("Hello, " + name + "!");
    }
};

// Create and use the greeter
greeter: Greeter = Greeter();
greeter.sayHello();
greeter.greet("Flux");
