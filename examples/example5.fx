namespace Example
{
    class Animal
    {
        object Attributes
        {
            string name;
            int age;

            def __init(string name, int age) -> void
            {
                this.name = name;
                this.age = age;
                return;
            };
        };

        object Actions
        {
            def speak() -> string
            {
                return i"My name is {} and I am {} years old.":{super.Attributes.name; super.Attributes.age;};
            };
        };
    };

    class Dog<Animal>
    {
        object Actions<Animal.Actions>
        {
            def speak() -> string
            {
                return i"Woof! {}":{super.Animal.Actions.speak();};   // Parser expects another here semicolon but that's wrong
            };
        };
    };
};

def main() -> !void
{
    // Create an instance of Dog
    Example::Dog.Attributes("Buddy", 5){} myDog;
    
    // Call the speak method
    print(myDog.Actions.speak()); // Output: "Woof! My name is Buddy and I am 5 years old."
    
    return 0;
};