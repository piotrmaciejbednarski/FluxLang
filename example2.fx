import "std.fx" as std;
using std.io, std.types;

// Define an Item object
object Item {
    def __init(int id, string name, float price, int stock) -> void {
        this.id = id;
        this.name = name;
        this.price = price;
        this.stock = stock;
    };

    def __exit() -> void {
        print(i"Item {} destroyed":{this.id;});
    };

    int id;
    string name;
    float price;
    int stock;

    def sell(int quantity) -> bool {
        if (this.stock >= quantity) {
            this.stock -= quantity;
            return true;
        };
        return false;
    };

    def restock(int quantity) -> void {
        this.stock += quantity;
    };

    def display() -> void {
        print(i"ID: {} | Name: {} | Price: ${} | Stock: {}":{
            this.id;
            this.name;
            this.price;
            this.stock;
        });
    };
};

// Inventory manager object
object Inventory {
    def __init() -> void {
        this.items = {};
        this.next_id = 1;
    };

    dict items;  // Dictionary to store items by ID
    int next_id;

    def add_item(string name, float price, int initial_stock) -> void {
        Item{}(this.next_id, name, price, initial_stock) new_item;
        this.items[this.next_id] = @new_item;
        this.next_id += 1;
        print("Item added successfully!");
    };

    def sell_item(int id, int quantity) -> bool {
        if (id not in this.items) {
            print("Item not found!");
            return false;
        };

        Item* item = this.items[id];
        if (item.sell(quantity)) {
            print(i"Sold {} units of {}":{quantity; item.name;});
            return true;
        } else {
            print("Not enough stock available!");
            return false;
        };
    };

    def restock_item(int id, int quantity) -> void {
        if (id not in this.items) {
            print("Item not found!");
            return;
        };

        Item* item = this.items[id];
        item.restock(quantity);
        print(i"Restocked {} units of {}":{quantity; item.name;});
    };

    def list_items() -> void {
        if (this.items.len() == 0) {
            print("No items in inventory.");
            return;
        };

        print("\nCurrent Inventory:");
        print("-----------------");
        for (id, item_ptr in this.items) {
            Item* item = item_ptr;
            item.display();
        };
        print("-----------------");
    };
};

// Main CLI interface
def main() -> int {
    Inventory{} inventory;
    bool running = true;

    print("Flux Inventory Management System");
    print("--------------------------------");

    while (running) {
        print("\nMenu:");
        print("1. Add new item");
        print("2. Sell item");
        print("3. Restock item");
        print("4. List all items");
        print("5. Exit");

        int choice = input("Enter your choice: ") as int;

        switch (choice) {
            case (1) {  // Add item
                string name = input("Enter item name: ");
                float price = input("Enter item price: ") as float;
                int stock = input("Enter initial stock: ") as int;
                inventory.add_item(name, price, stock);
            };

            case (2) {  // Sell item
                inventory.list_items();
                if (inventory.items.len() > 0) {
                    int id = input("Enter item ID to sell: ") as int;
                    int quantity = input("Enter quantity to sell: ") as int;
                    inventory.sell_item(id, quantity);
                };
            };

            case (3) {  // Restock item
                inventory.list_items();
                if (inventory.items.len() > 0) {
                    int id = input("Enter item ID to restock: ") as int;
                    int quantity = input("Enter restock quantity: ") as int;
                    inventory.restock_item(id, quantity);
                };
            };

            case (4) {  // List items
                inventory.list_items();
            };

            case (5) {  // Exit
                running = false;
                print("Exiting system...");
            };

            case (default) {
                print("Invalid choice! Please try again.");
            };
        };
    };

    return 0;
};