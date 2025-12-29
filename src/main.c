// #define DYNAMIC_ARRAY_IMPLEMENTATION
// #include "da.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
TODO:
- make tickets a stack as well (its literly a stack of cards)
- convert Orientation to bool
- break main and get_user_input into smaller functions
*/

#define DEBUG(fmt, ...) fprintf(stderr, "DEBUG %s %d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG(fmt, ...)   fprintf(stderr, "LOG %s %d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define BOARD_SIZE    17 // 16 for real game 17th is for winning camels
#define N_PLAYERS     6  // number of players
#define N_DICE        5  // number of dice per round - one less than total dice
#define N_BETS_COLORS 5  // 1 per regular camel color
#define N_TICKETS     4  // 5,3,2,2
#define N_CAMELS      7  // reg + 2 crazy
#define MAX_WAGERS    N_PLAYERS* N_BETS_COLORS

typedef enum { BRED, BBLUE, BYELLOW, BGREEN, BPURPLE } BetColor;
typedef enum { CRED, CBLUE, CYELLOW, CGREEN, CPURPLE, CWHITE, CBLACK } CamelColor;
typedef enum { DRED, DBLUE, DYELLOW, DGREEN, DPURPLE, DGREY } DiceColor;
typedef enum { FORWARD, REVERSE } Orientation;
typedef enum { WAGER, ROLL, TICKET, SPECTATOR } TurnType;

const char* orient2char(Orientation oreint) {
    if (oreint == FORWARD) {
        return "+";
    }
    return "-";
}

const char* enum2char(CamelColor color) {
    switch (color) {
        case CRED:
            return "R";
        case CBLUE:
            return "B";
        case CYELLOW:
            return "Y";
        case CGREEN:
            return "G";
        case CPURPLE:
            return "P";
        case CBLACK:
            return "K";
        case CWHITE:
            return "W";
        default:
            return NULL;
    }
}

typedef struct {
    size_t count;
    size_t capacity;
    int* items;
} Locations;

typedef struct {
    TurnType turn_type;
    Orientation orientation; // For Wager:  FORWARD == winner, REVERSE == LOOSER
    BetColor color;          // for wafer and ticket
    int position;            // Spectator position
} Turn;

typedef struct {
    CamelColor color;
    int value; // 1, 2, 3
} Roll;

typedef struct {
    size_t count;
    size_t capacity;
    Roll items[N_DICE];
} Dice;

typedef struct {
    int player;
    BetColor color;
} Wager;

typedef struct {
    size_t count;
    size_t capacity;
    Wager items[MAX_WAGERS];
} WagerStack;

typedef struct {
    BetColor color;
    int amount;    // 5, 3, 2, 2
    int player_id; // -1 if available >=0 refers to player holding the ticket
} Ticket;

typedef struct {
    size_t count;
    size_t capacity;
    Ticket items[N_TICKETS];
} TicketStack;

typedef struct {
    CamelColor color;
    Orientation orientation;
    int space; // space the camel is on - NOT NEEDED
} Camel;

typedef struct {
    size_t count;
    size_t capacity;
    Camel items[N_CAMELS];
} CamelStack;

typedef struct {
    int player;              // who placed spec
    Orientation orientation; // +1 or -1
} Spectator;

typedef struct {
    Spectator spec; // spec card on spot
    bool has_spec;  // spec card on spot?
    CamelStack camel_stack;
} Tile;

typedef struct {
    BetColor color;
    bool used;
} Hand;

typedef struct {
    int id;
    int points;
    bool used_spec;
    int hand_size;
    Hand hand[N_BETS_COLORS]; // the color cards you have left to bet for dinner or loser
} Player;

typedef struct {
    bool winner;
    int turn;
    int round;
    Dice dice;
    Tile board[BOARD_SIZE]; // array of tiles each tile can either be a stack of camels or a spec - add one for
                            // winnner spot
    Player players[N_PLAYERS];
    TicketStack tickets[N_BETS_COLORS];
    WagerStack winner_bets; // stacks
    WagerStack loser_bets;  // stacks
} Game;

//////////////////////////////////// Stack //////////////////////////////////////
#define stack_push(s, i) (((s)->count < (s)->capacity) ? (s)->items[(s)->count++] = (i), true : false)
#define stack_pop(s, i)  ((s)->count > 0 ? * i = (s)->items[--(s)->count], (true) : (false))
#define stack_empty(s)   ((s)->count == 0 ? true : false)
#define stack_peak(s)    ((s)->items[(s)->count - 1])
#define stack_count(s)   ((s)->count)

//
// bool wager_push(Wagers* wagers, Wager w) {
//     if (wagers->count < MAX_WAGERS) {
//         wagers->items[wagers->count++] = w;
//         return true;
//     }
//     return false;
// }
//
// bool wager_pop(Wagers* wagers, Wager* w) {
//     if (wagers->count > 0) {
//         *w = wagers->items[--wagers->count];
//         return true;
//     }
//     return false;
// }
//
// bool dice_push(Dice* dice, Roll r) {
//     if (dice->count < N_DICE) {
//         dice->rolled[dice->count++] = r;
//         return true;
//     }
//     return false;
// }
//
// bool dice_pop(Dice* dice, Roll* r) {
//     if (dice->count > 0) {
//         *r = dice->rolled[--dice->count];
//         return true;
//     }
//     return false;
// }
//
// Roll dice_peek(Dice* dice) { return dice->rolled[dice->count - 1]; }
//
//
// bool camels_push(Wagers* wagers, Wager w) {
//     if (wagers->count < MAX_WAGERS) {
//         wagers->items[wagers->count++] = w;
//         return true;
//     }
//     return false;
// }
//
// bool camels_pop(CamelStack* camels, Camel* w) {
//     if (camels->count > 0) {
//         *w = camels->items[--wagers->count];
//         return true;
//     }
//     return false;
// }
//
// bool camels_empty(){
// }

int compare(const void* p1, const void* p2) {
    Player* e1 = (Player*) p1;
    Player* e2 = (Player*) p2;
    return e1->points - e2->points;
}
//////////////////////////////////// Game State //////////////////////////////////////

int rand_range(int low, int high) { return (rand() % (high - low + 1)) + low; }

void reset_tickets(Game* game) {

    for (int i = 0; i < N_BETS_COLORS; i++) {
        BetColor color = (BetColor) i;
        for (int j = 0; j < N_TICKETS; j++) {
            int amount;
            if (j == 0) {
                amount = 5;
            } else if (j == 1) {
                amount = 3;
            } else {
                amount = 2;
            }

            Ticket t = {.amount = amount, .color = color, .player_id = -1};
            stack_push(&game->tickets[i], t);
        }
    }
}

void reset_dice(Game* game) {
    Dice dice     = {0};
    dice.capacity = N_DICE;
    game->dice    = dice;
}

void init_game(Game* game) {
    game->turn = 0;
    reset_dice(game);
    game->winner = false;

    ///// set up board /////////
    for (int i = 0; i < BOARD_SIZE; i++) {
        CamelStack cs              = {0};
        cs.capacity                = N_CAMELS;
        game->board[i].camel_stack = cs;
        game->board[i].has_spec    = false;
    }

    ///// Place camels on board /////////
    for (int c = 0; c < N_CAMELS; c++) {
        int starting_tile = rand_range(0, 2);
        Camel camel       = {0};
        camel.color       = (CamelColor) c;
        camel.orientation = FORWARD;

        if ((CamelColor) c == CBLACK || (CamelColor) c == CWHITE) {
            starting_tile     = BOARD_SIZE - starting_tile - 2;
            camel.orientation = REVERSE;
        }
        camel.space   = starting_tile;
        CamelStack* s = &game->board[starting_tile].camel_stack;
        stack_push(s, camel);
    }

    ///// Players /////////
    for (int i = 0; i < N_PLAYERS; i++) {
        Player p         = {.id        = i,
                            .points    = 0,
                            .used_spec = false,
                            .hand      = {{BRED, false}, {BBLUE, false}, {BYELLOW, false}, {BGREEN, false}, {BPURPLE, false}}};
        game->players[i] = p;
    }
    TicketStack ts = {0};
    ts.capacity    = N_TICKETS * N_BETS_COLORS;
    reset_tickets(game);

    ///// Wagers /////////
    WagerStack losing_bets = {0};
    losing_bets.capacity   = MAX_WAGERS;
    game->loser_bets       = losing_bets;

    WagerStack winning_bets = {0};
    winning_bets.capacity   = MAX_WAGERS;
    game->winner_bets       = winning_bets;
}

void score_wagers(Game* game, BetColor first, BetColor last) {

    // give out points for bets
    Wager w       = {0};
    int scores[6] = {1, 2, 3, 5, 8};
    int n         = 5;
    while (stack_pop(&game->winner_bets, &w)) {
        if (w.color == first) {
            game->players[w.player].points += scores[n];
            n = (n > 0) ? (n - 1) : 0;
        } else {
            game->players[w.player].points--;
        }
    }
    n = 5;
    while (stack_pop(&game->loser_bets, &w)) {
        if (w.color == last) {
            game->players[w.player].points += scores[n];
            n = (n > 0) ? (n - 1) : 0;
        } else {
            game->players[w.player].points--;
        }
    }
}

void assign_points(Game* game, CamelColor top, CamelColor second) {

    // assign points to anyone holding a winning color ticket
    for (int i = 0; i < N_BETS_COLORS; i++) {
        for (int j = 0; j < N_TICKETS; j++) {
            Ticket t = game->tickets[i].items[j];
            if (t.player_id != -1) {
                if (i == (int) top) {
                    game->players[t.player_id].points += t.amount; // give points to winner
                } else if (i == (int) second) {
                    game->players[t.player_id].points++; // give 1 point for second
                } else {
                    game->players[t.player_id].points--; // minus 1 point for 3rd or worse
                }
            }
        }
    }
}

int get_last_camel(Game* game) {
    int last = -1;
    for (int b = 0; b < BOARD_SIZE - 1; b++) {
        CamelStack* stack = &game->board[b].camel_stack;
        for (size_t j = 0; j < stack_count(stack); j++) {
            CamelColor bottom = stack->items[j].color;
            if (bottom != CBLACK && bottom != CWHITE) {
                last = (int) j;
            }
        }
    }
    return last;
}

void get_top_camels(Game* game, int* first, int* second) {

    // get top_camel and second place dont count black and white camel
    *first  = -1;
    *second = -1;

    for (size_t b = BOARD_SIZE - 1; b > 0; b--) {
        CamelStack* stack = &game->board[b].camel_stack;
        for (size_t j = stack_count(stack); j > 0; j--) {
            CamelColor top = stack->items[j - 1].color;
            if (top != CBLACK && top != CWHITE) {
                if (*first == -1) {
                    *first = (int) top;
                } else {
                    *second = (int) top;
                    return;
                }
            }
        }
    }
}

void score_round(Game* game, int* first, int* second) {
    get_top_camels(game, first, second);
    assign_points(game, (CamelColor) *first, (CamelColor) *second);

    if (game->winner) {
        int last = get_last_camel(game);
        assert(last != -1 && "Could not get losing camel");
        score_wagers(game, (BetColor) *first, (BetColor) last);
    }
}

void end_round(Game* game) {
    game->round++;
    reset_dice(game);

    // remove spec cards
    for (int i = 0; i < N_PLAYERS; i++) {
        game->players[i].used_spec = false;
    }

    for (int i = 0; i < BOARD_SIZE; i++) {
        game->board[i].has_spec = false;
    }
    reset_tickets(game);
}

//////////////////////////////////// Turn //////////////////////////////////////

// -1 if ticket is not available, else the amount
int assign_ticket(Game* game, BetColor color, int player_id) {

    TicketStack* tickets = &game->tickets[(int) color];

    int available = -1;

    for (int i = 0; i < N_TICKETS; i++) {
        if (tickets->items[i].player_id == -1) {
            available                   = tickets->items[i].amount;
            tickets->items[i].player_id = player_id;
            return available;
        }
    }
    return available;
}

void get_available_tickets(Game* game, TicketStack* tickets) {
    for (int i = 0; i < N_BETS_COLORS; i++) {
        for (int j = 0; j < N_TICKETS; j++) {
            if (game->tickets[i].items[j].player_id == -1) {
                stack_push(tickets, game->tickets[i].items[i]);
            }
        }
    }
}

void get_possible_spec_location(Game* game, Locations* buff) {
    for (int i = 0; i < BOARD_SIZE - 1; i++) { // cant put peice on last spot
        if (!(stack_count((&game->board[i].camel_stack)) > 0 || game->board[i].has_spec ||
              (i != BOARD_SIZE && game->board[i + 1].has_spec) || (i != 0 && game->board[i - 1].has_spec))) {
            stack_push(buff, i);
        }
    }
}

bool place_spec_tile(Game* game, int player_id, int space, Spectator spec) {

    if (game->players[player_id].used_spec) {
        return false;
    }

    // check if spec next to tile or on tile
    if (stack_count((&game->board[space].camel_stack)) > 0 || game->board[space].has_spec ||
        (space != BOARD_SIZE && game->board[space + 1].has_spec) || (space != 0 && game->board[space - 1].has_spec)) {
        return false;
    }
    game->board[space].has_spec = true;
    game->board[space].spec     = spec;
    return true;
}

void roll_dice(Game* game) {
    Roll die = {0};

    int random_int = rand() % (N_DICE + 1);

    // Cast the random integer to the enum type
    DiceColor random_color = (DiceColor) random_int;
    if (random_color == DGREY) {
        // if Grey randomly select 0 or 1 for black or white
        die.color = (CamelColor) ((rand() % 2) + N_BETS_COLORS);
        die.value = rand_range(1, 3) * -1;

    } else {
        die.color = (CamelColor) random_color;
        die.value = rand_range(1, 3);
    }

    stack_push(&game->dice, die);
}

Camel* get_camel(Game* game, CamelColor color) {

    for (int i = 0; i < BOARD_SIZE; i++) {
        CamelStack* stack = &game->board[i].camel_stack;
        for (size_t j = 0; j < stack_count(stack); j++) {
            if (stack->items[j].color == color) {
                return &stack->items[j];
            }
        }
    }
    return NULL;
}

void move_camel(Game* game, CamelColor color, int spaces) {
    int dest;
    Camel* camel = get_camel(game, color);
    assert(camel != NULL && "Could not find your camel");
    int curr_space = camel->space;

    Orientation move_orientation = camel->orientation;

    if (game->board[curr_space + spaces].has_spec) {
        Spectator spec = game->board[curr_space + spaces].spec;

        move_orientation = spec.orientation;

        game->players[spec.player].points++;          // Give point to player who placed spec
        if (spec.orientation == camel->orientation) { // forward and +1 or reverse and -1
            spaces++;
        } else {
            spaces--;
        }
    }
    dest = curr_space + spaces;

    if (dest < 0) {
        dest = 0;
    }

    if (dest >= BOARD_SIZE - 1) {
        game->winner = true;
        dest         = BOARD_SIZE - 1;
    }

    // move all camels in stack to dest
    CamelStack tmp   = {0};
    tmp.capacity     = N_CAMELS;
    CamelStack tmp2  = {0}; // only used if going backwards
    tmp2.capacity    = N_CAMELS;
    Camel curr_camel = {0};

    CamelStack* stack      = &game->board[curr_space].camel_stack;
    CamelStack* dest_stack = &game->board[dest].camel_stack;

    bool match_found = false;
    while (!match_found) { // pop from start stack until we reach desired camel
        stack_pop(stack, &curr_camel);
        match_found = curr_camel.color == color ? true : false;
        stack_push(&tmp, curr_camel);
    }

    // if camels are going in reverse, pop the dest stack into a tmp stack, then push start stack into dest and then
    // push dest stack
    if (move_orientation == REVERSE) {
        while (!stack_empty(dest_stack)) {
            stack_pop(dest_stack, &curr_camel);
            stack_push(&tmp2, curr_camel);
        }
    }

    while (!stack_empty(&tmp)) {
        stack_pop(&tmp, &curr_camel);
        // printf("pushed: %s ", enum2char(curr_camel.color));
        curr_camel.space = dest;
        stack_push(dest_stack, curr_camel);
    }

    if (move_orientation == REVERSE) {
        while (!stack_empty(&tmp2)) {
            stack_pop(&tmp2, &curr_camel);
            curr_camel.space = dest;
            stack_push(dest_stack, curr_camel);
        }
    }
}

//////////////////////////////////// I/O //////////////////////////////////////
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

void wait_for_enter(void) { getchar(); }

char read_char(void) {
    char input;
    scanf(" %c", &input);
    clear_input_buffer();
    return input;
}

int read_int(void) {
    int input;
    scanf(" %d", &input);
    clear_input_buffer();
    return input;
}
// TODO break into smaller functions
void get_user_input(Game* game, int player_id, Turn* turn) {
    printf("Player %d turn: [R]oll, [W]ager, Take a [T]icket or Place [S]pectator\n", player_id);
    char input_char = read_char();
    switch (input_char) {
        case 'R': {
            turn->turn_type = ROLL;
            break;
        };
        case 'W': {
            turn->turn_type = WAGER;
            if (game->players[player_id].hand_size < 0) {
                printf("No cards to wager\n");
                get_user_input(game, player_id, turn); // TODO RESET
            }
            printf("[W]inner or [L]oser\n");
            input_char = read_char();
            switch (input_char) {
                case 'W': {
                    turn->orientation = FORWARD;
                    break;
                }
                case 'L': {
                    turn->orientation = REVERSE;
                    break;
                }
                default: {
                    get_user_input(game, player_id, turn);
                    break;
                }
            }

            printf("Select a Camel to wager on: [R]ED, [B]LUE, [Y]ELLOW, [G]REEN, [P]URPLE\n");
            input_char = read_char();
            switch (input_char) {
                case 'R': {
                    turn->color = BRED;
                    break;
                };
                case 'B': {
                    turn->color = BBLUE;
                    break;
                };
                case 'Y': {
                    turn->color = BYELLOW;
                    break;
                };
                case 'G': {
                    turn->color = BGREEN;
                    break;
                };
                case 'P': {
                    turn->color = BPURPLE;
                    break;
                };
                default:
                    get_user_input(game, player_id, turn); // TODO RESET

                    break;
            }
            break;
        };

        case 'T': {
            turn->turn_type = TICKET;
            printf("Select ticket [R]ED, [B]LUE, [Y]ELLOW, [G]REEN, [P]URPLE\n");
            input_char = read_char();
            switch (input_char) {
                case 'R': {
                    turn->color = BRED;
                    break;
                };
                case 'B': {
                    turn->color = BBLUE;
                    break;
                };
                case 'Y': {
                    turn->color = BYELLOW;
                    break;
                };
                case 'G': {
                    turn->color = BGREEN;
                    break;
                };
                case 'P': {
                    turn->color = BPURPLE;
                    break;
                };
                default:
                    get_user_input(game, player_id, turn); // TODO RESET
                    break;
            }
            break;
        }
        case 'S': {
            turn->turn_type = SPECTATOR;
            // int* buff = NULL;
            // get_possible_spec_location(game, buff); // TODO check that this is a valid spot
            printf("Pick a location to place the Spectator tile:\n");
            int pos        = read_int();
            turn->position = pos;
            printf("[+]1 or [-]1\n");
            input_char = read_char();
            switch (input_char) {
                case '+': {
                    turn->orientation = FORWARD;
                    break;
                };
                case '-': {
                    turn->orientation = REVERSE;
                    break;
                };
                default: {
                    get_user_input(game, player_id, turn); // TODO RESET
                    break;
                }
            }
            turn->orientation = input_char == '+' ? FORWARD : REVERSE;
            break;
        };
        default: {
            get_user_input(game, player_id, turn); // TODO RESET

            break;
        }
    }
}

void clear_screen(void) {
    // ANSI escape code to clear the entire screen and move the cursor to the top-left (1,1)
    printf("\033[2J\033[1;1H");
}

void render_horizontal(Game* game) {
    clear_screen();
    printf("Round: %d | Turn %d\n", game->round, game->turn);
    // render wagers
    printf("Wagers\n");
    printf("W: [%zu] ", game->winner_bets.count);
    printf("L: [%zu] ", game->loser_bets.count);

    // render tickets
    printf("\nTickets\n");

    for (int i = 0; i < N_BETS_COLORS; i++) {
        BetColor color = (BetColor) i;
        printf(" %s | ", enum2char((CamelColor) color)); // todo fix cast?
        for (int j = 0; j < N_TICKETS; j++) {
            if (game->tickets[i].items[j].player_id == -1) {
                printf(" [%d] ", game->tickets[i].items[j].amount);
            } else {

                printf("     ");
            }
        }
        printf("\n");
    }

    printf("\nPlayers\n");

    for (int i = 0; i < N_PLAYERS; i++) {
        printf(" %d (%2d)| ", game->players[i].id, game->players[i].points);
        for (int j = 0; j < N_BETS_COLORS; j++) {
            for (size_t k = 0; k < stack_count(&game->tickets[j]); k++) {
                if (game->tickets[j].items[k].player_id == i) {
                    printf(" [%s:%d] ", enum2char((CamelColor) game->tickets[j].items[k].color),
                           game->tickets[j].items[k].amount);
                }
            }
        }

        printf("\n");
    }

    printf("Dice\n");
    for (size_t i = 0; i < game->dice.count; i++) {
        printf(" | %s:%d | ", enum2char(game->dice.items[i].color), game->dice.items[i].value);
    }

    printf("\nBoard\n");

    // render board
    for (size_t i = N_CAMELS; i > 0; i--) {
        printf(" %zu | ", i);
        for (size_t j = 0; j < BOARD_SIZE; j++) {

            if (game->board[j].has_spec && i == 1) {
                printf(" %s1 ", orient2char(game->board[j].spec.orientation));
                continue;
            } else if (stack_count((&game->board[j].camel_stack)) >= i) {
                printf(" %2s ", enum2char(game->board[j].camel_stack.items[i - 1].color));
            } else {
                printf("    ");
            }
        }
        printf("\n");
    }
    printf("     ");
    for (size_t j = 0; j < BOARD_SIZE; j++) {
        printf(" %2zu ", j);
    }
    printf("\n");
    if (game->winner) {
        printf("\nGame Over!:\n");
        printf("Scores\n");
        for (size_t i = N_PLAYERS; i > 0; i--) {
            printf("%zu: %d %d \n", i, game->players[i].id, game->players[i].points);
        }
    }
}

bool next_turn(Game* game, Turn* turn, int curr_player_id) {

    // get next players input
    switch (turn->turn_type) {
        case WAGER: {
            LOG("Turn %d: %d made wagered %s to %s \n", game->turn, curr_player_id, enum2char((CamelColor) turn->color),
                orient2char(turn->orientation));

            bool in_hand = false;
            for (int i = 0; i < game->players[curr_player_id].hand_size; i++) {
                if (game->players[curr_player_id].hand[i].color == turn->color &&
                    game->players[curr_player_id].hand[i].used == false) {
                    in_hand                                    = true;
                    game->players[curr_player_id].hand[i].used = true;
                }
            }

            if (!in_hand) {
                return false;
            }

            Wager w = {.color = turn->color, .player = curr_player_id};
            if (turn->orientation == FORWARD) {
                stack_push(&game->winner_bets, w);
            } else {
                stack_push(&game->loser_bets, w);
            }

            break;
        }
        case ROLL: {
            roll_dice(game);
            Roll die = stack_peak(&game->dice);
            LOG("Turn %d: %d rolled %s%d\n", game->turn, curr_player_id, enum2char((CamelColor) die.color), die.value);
            move_camel(game, die.color, die.value);
            game->players[curr_player_id].points++;
            break;
        }
        case TICKET: {
            int amount = assign_ticket(game, turn->color, curr_player_id);
            if (amount == -1) {
                LOG("Turn %d: Could not assign ticket to %d", game->turn, curr_player_id);
                return false;
            }
            LOG("Turn %d: %d picked a %s:%d Ticket", game->turn, curr_player_id, enum2char((CamelColor) turn->color),
                amount);
            break;
        }

        case SPECTATOR: {
            Spectator spec = {.orientation = turn->orientation, .player = curr_player_id};
            bool allowed   = place_spec_tile(game, curr_player_id, turn->position, spec);
            if (!allowed) {
                LOG("Turn %d: %d COULD NOT place Spectator card (%s1) on %d", game->turn, curr_player_id,
                    orient2char(turn->orientation), turn->position);
                return false;
            }
            LOG("Turn %d: %d placed Spectator card (%s1) on %d", game->turn, curr_player_id,
                orient2char(turn->orientation), turn->position);
            break;
        }
        default:
            break;
    }
    return true;
}

#ifndef TEST_BUILD
int main(int argc, char** argv) {
    // srand((unsigned int) time(NULL));
    srand((unsigned int) 4);

    Game game = {0};
    init_game(&game);
    render_horizontal(&game);
    printf("Enter any key to start game\n");
    wait_for_enter();

    int curr_player_id = 0;
    int first, second;
    Turn turn = {0};
    // Roll die  = {0};
    while (!game.winner) {
        while (game.dice.count != N_DICE && !game.winner) {
            bool valid_turn = false;

            while (!valid_turn) {
                get_user_input(&game, curr_player_id, &turn);
                valid_turn = next_turn(&game, &turn, curr_player_id);
            }

            // render game state
            render_horizontal(&game);
            curr_player_id = (curr_player_id + 1) % N_PLAYERS;
            game.turn++;
        }
        // give out points for round
        score_round(&game, &first, &second);
        printf("First place: %s\tSecond Place: %s\n", enum2char((CamelColor) first), enum2char((CamelColor) second));
        render_horizontal(&game);

        if (!game.winner) {
            printf("Enter any key for next round\n");
            wait_for_enter();
        }
        end_round(&game);
        render_horizontal(&game);
    }

    qsort(game.players, N_PLAYERS, sizeof(Player), compare);
    render_horizontal(&game);
    return 0;
}

#endif // TEST_BUILD
