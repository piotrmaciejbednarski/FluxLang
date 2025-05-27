import "std.fx" as std;
using std::io, std::types;

// Define card object
object Card {
    def __init(string suit, string rank, int value) -> void {
        this.suit = suit;
        this.rank = rank;
        this.value = value;
        this.hidden = false;
    };

    string suit;
    string rank;
    int value;
    bool hidden;

    def display() -> string {
        if (this.hidden) {
            return "[X]";
        };
        return i"{} {}":{this.rank; this.suit;};
    };
};

// Define deck object
object Deck {
    def __init() -> void {
        this.cards = [];
        this.reset();
    };

    Card[] cards;

    def reset() -> void {
        this.cards = [];
        string[] suits = ["♥", "♦", "♣", "♠"];
        string[] ranks = ["2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"];
        int[] values = [2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10, 11];

        for (suit in suits) {
            for (i in 0..ranks.len()) {
                Card{}(suit, ranks[i], values[i]) new_card;
                this.cards.append(@new_card);
            };
        };
    };

    def shuffle() -> void {
        for (i in 0..this.cards.len()) {
            int j = random() % this.cards.len();
            Card temp = this.cards[i];
            this.cards[i] = this.cards[j];
            this.cards[j] = temp;
        };
    };

    def deal() -> Card* {
        if (this.cards.len() == 0) {
            this.reset();
            this.shuffle();
        };
        return this.cards.pop();
    };
};

// Define hand object
object Hand {
    def __init() -> void {
        this.cards = [];
    };

    Card[] cards;

    def add_card(Card* card) -> void {
        this.cards.append(card);
    };

    def get_value() -> int {
        int total = 0;
        int aces = 0;

        for (card_ptr in this.cards) {
            Card* card = card_ptr;
            if (not card.hidden) {
                total += card.value;
                if (card.rank == "A") {
                    aces += 1;
                };
            };
        };

        // Handle aces (11 or 1)
        while (total > 21 and aces > 0) {
            total -= 10;
            aces -= 1;
        };

        return total;
    };

    def display() -> void {
        string hand_str = "";
        for (card_ptr in this.cards) {
            Card* card = card_ptr;
            hand_str += card.display() + " ";
        };
        print(i"{} (Total: {})":{hand_str; this.get_value();});
    };
};

// Define player object
object Player {
    def __init(string name) -> void {
        this.name = name;
        this.hand = Hand{};
        this.chips = 1000;
    };

    string name;
    Hand hand;
    int chips;
    int current_bet;

    def place_bet(int amount) -> bool {
        if (amount > this.chips) {
            return false;
        };
        this.current_bet = amount;
        this.chips -= amount;
        return true;
    };

    def win_bet() -> void {
        this.chips += this.current_bet * 2;
        this.current_bet = 0;
    };

    def push() -> void {
        this.chips += this.current_bet;
        this.current_bet = 0;
    };

    def clear_hand() -> void {
        this.hand = Hand{};
    };

    def display() -> void {
        print(i"\n{}'s hand:":{this.name;});
        this.hand.display();
        print(i"Chips: ${} | Current bet: ${}":{this.chips; this.current_bet;});
    };
};

// Main game object
object Blackjack {
    def __init() -> void {
        this.deck = Deck{};
        this.deck.shuffle();
        this.player = Player{}("Player");
        this.dealer = Player{}("Dealer");
    };

    Deck deck;
    Player player;
    Player dealer;

    def deal_initial_cards() -> void {
        this.player.clear_hand();
        this.dealer.clear_hand();

        // Deal two cards to each player
        this.player.hand.add_card(this.deck.deal());
        this.dealer.hand.add_card(this.deck.deal());
        this.player.hand.add_card(this.deck.deal());
        
        // Dealer's second card is hidden
        Card* dealer_card = this.deck.deal();
        dealer_card.hidden = true;
        this.dealer.hand.add_card(dealer_card);
    };

    def player_turn() -> bool {
        while (true) {
            this.player.display();
            this.dealer.display();

            if (this.player.hand.get_value() == 21) {
                print("Blackjack!");
                return false;  // Player stands
            };

            string choice = input("\nHit or stand? (h/s): ");
            if (choice == "h") {
                this.player.hand.add_card(this.deck.deal());
                if (this.player.hand.get_value() > 21) {
                    print("Bust!");
                    return false;
                };
            } else if (choice == "s") {
                return false;
            } else {
                print("Invalid choice!");
            };
        };
        return true;
    };

    def dealer_turn() -> void {
        // Reveal dealer's hidden card
        for (card_ptr in this.dealer.hand.cards) {
            Card* card = card_ptr;
            card.hidden = false;
        };

        while (this.dealer.hand.get_value() < 17) {
            this.dealer.hand.add_card(this.deck.deal());
        };
    };

    def determine_winner() -> void {
        int player_value = this.player.hand.get_value();
        int dealer_value = this.dealer.hand.get_value();

        this.player.display();
        this.dealer.display();

        if (player_value > 21) {
            print("\nYou busted! Dealer wins.");
        } else if (dealer_value > 21) {
            print("\nDealer busted! You win!");
            this.player.win_bet();
        } else if (player_value > dealer_value) {
            print("\nYou win!");
            this.player.win_bet();
        } else if (player_value < dealer_value) {
            print("\nDealer wins!");
        } else {
            print("\nPush! It's a tie.");
            this.player.push();
        };
    };

    def play_round() -> void {
        print(i"\nYou have ${} chips":{this.player.chips;});
        int bet = input("Place your bet: ") as int;

        if (not this.player.place_bet(bet)) {
            print("Not enough chips!");
            return;
        };

        this.deal_initial_cards();
        this.player_turn();
        
        if (this.player.hand.get_value() <= 21) {
            this.dealer_turn();
            this.determine_winner();
        };
    };

    def play_game() -> void {
        print("Welcome to Flux Blackjack!");
        print("-------------------------");

        while (this.player.chips > 0) {
            this.play_round();
            
            if (this.player.chips == 0) {
                print("You're out of chips! Game over.");
                break;
            };

            string choice = input("\nPlay again? (y/n): ");
            if (choice not == "y") {
                break;
            };
        };

        print(i"\nThanks for playing! Final chips: ${}":{this.player.chips;});
    };
};

// Main function to start the game
def main() -> int {
    Blackjack{} game;
    game.play_game();
    return 0;
};