#define DYNAMIC_ARRAY_IMPLEMENTATION
#include "da.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
TODO:
- make tickets a stack as well (its literly a stack of cards)
*/

#define BOARD_SIZE 16
#define N_PLAYERS  5
#define N_DICE     5

#define N_BETS_COLORS 5 // 1 per regular camel color
#define N_TICKETS     4 // 5,3,2,2
#define N_CAMELS      7 // reg + 2 crazy

typedef enum { BRED, BBLUE, BYELLOW, BGREEN, BPURPLE } BetColor;
typedef enum { CRED, CBLUE, CYELLOW, CGREEN, CPURPLE, CWHITE, CBLACK } CamelColor;
typedef enum { DRED, DBLUE, DYELLOW, DGREEN, DPURPLE, DGREY } DiceColor;
typedef enum { FORWARD, REVERSE } Orientation;

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
    }
}

typedef struct {
    CamelColor color;
    int value; // 1, 2, 3
} Roll;

typedef struct {
    int player;
    BetColor color;
} Bet;

typedef struct {
    BetColor color;
    int amount;    // 5, 3, 2, 2
    int player_id; // -1 if available >=0 refers to player holding the ticket
} Ticket;

typedef struct {
    CamelColor color;
    Orientation orientation;
    int space; // space the camel is on - NOT NEEDED
} Camel;

typedef struct {
    int player;              // who placed spec
    Orientation orientation; // +1 or -1
} Spectator;

typedef struct {
    Spectator spec; // spec card on spot
    bool has_spec;  // spec card on spot?
    Camel* camel_stack;
} Tile;

typedef struct {
    int id;
    int points;
    bool used_spec;
    BetColor hand[N_BETS_COLORS]; // the color cards you have left to bet for dinner or loser
} Player;

typedef struct {
    int turn;
    Tile board[BOARD_SIZE]; // array of tiles each tile can either be a stack of camels or a spec
    Player players[N_PLAYERS];
    Bet winner_bets[N_PLAYERS * N_BETS_COLORS];
    Bet loser_bets[N_PLAYERS * N_BETS_COLORS];
    Ticket tickets[N_BETS_COLORS][N_TICKETS];
} Game;

//////////////////////////////////// Game State //////////////////////////////////////

int rand_range(int low, int high) { return (rand() % (high - low + 1)) + low; }

void init_game(Game* game) {

    ///// set up board /////////
    for (int i = 0; i < BOARD_SIZE; i++) {
        game->board[i].camel_stack = NULL;
        game->board[i].has_spec    = false;
    }

    ///// Place camels on board /////////
    for (int c = 0; c < N_CAMELS; c++) {
        int starting_tile = rand_range(0, 2);
        Camel camel       = {0};
        camel.color       = (CamelColor) c;
        camel.orientation = FORWARD;

        if ((CamelColor) c == CBLACK || (CamelColor) c == CWHITE) {
            starting_tile     = BOARD_SIZE - starting_tile - 1;
            camel.orientation = REVERSE;
        }
        camel.space = starting_tile;
        stack_push(game->board[starting_tile].camel_stack, camel);
    }

    ///// Players /////////
    for (int i = 0; i < N_PLAYERS; i++) {
        Player p         = {.id = i, .points = 0, .used_spec = false, .hand = {BRED, BBLUE, BYELLOW, BGREEN, BPURPLE}};
        game->players[i] = p;
    }

    ///// Wagers /////////
}
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
            Ticket t            = {.amount = amount, .color = color, .player_id = -1};
            game->tickets[i][j] = t;
        }
    }
}

void init_round(Game* game) {
    // reset dice count

    // reset tickets
    reset_tickets(game);

    // remove spec cards
    for (int i = 0; i < N_PLAYERS; i++) {
        game->players[i].used_spec = false;
    }

    for (int i = 0; i < BOARD_SIZE; i++) {
        game->board[i].has_spec = false;
    }
}

// void end_round(Game* game) {};

//////////////////////////////////// Turn //////////////////////////////////////

bool assign_ticket(Game* game, BetColor color, int player_id) {

    Ticket* tickets = game->tickets[(int) color];

    bool available = false;

    for (int i = 0; i < N_TICKETS; i++) {
        if (tickets[i].player_id == -1) {
            available            = true;
            tickets[i].player_id = player_id;
        }
    }
    return available;
}

bool place_spec_tile(Game* game, int space, Spectator spec) {
    // check if spec next to tile or on tile
    if (stack_count(game->board[space].camel_stack) > 0 || game->board[space].has_spec ||
        (space != BOARD_SIZE && game->board[space + 1].has_spec) || (space != 0 && game->board[space - 1].has_spec)) {
        return false;
    }
    game->board[space].has_spec = true;
    game->board[space].spec     = spec;
    return true;
}

void roll_dice(Roll* die) {

    int random_int = rand() % N_DICE + 1;

    // Cast the random integer to the enum type
    DiceColor random_color = (DiceColor) random_int;
    if (random_color == DGREY) {
        // if Grey randomly select 0 or 1 for black or white
        die->color = (CamelColor) ((rand() % 2) + N_BETS_COLORS);
    } else {
        die->color = (CamelColor) random_color;
    }

    die->value = rand_range(1, 3);
}
Camel* get_camel(Game* game, CamelColor color) {

    for (int i = 0; i < BOARD_SIZE; i++) {
        Camel* stack = game->board[i].camel_stack;
        for (size_t j = 0; j < stack_count(stack); j++) {
            if (stack[j].color == color) {
                return &stack[j];
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

    Orientation move_orientation = FORWARD;

    if (game->board[curr_space + spaces].has_spec) {
        Spectator spec = game->board[curr_space + spaces].spec;

        if (spec.orientation == REVERSE || camel->orientation == REVERSE) {
            move_orientation = REVERSE;
        }

        // game->players[spec.player].points++;          // Give point to player who placed spec
        if (spec.orientation == camel->orientation) { // forward and +1 or reverse and -1
            spaces++;
        } else {
            spaces--;
        }
    }
    dest = curr_space + spaces;

    if (dest > BOARD_SIZE) {
        // TODO implement
        printf("Game over\n");
    }

    // move all camels in stack to dest
    Camel* tmp       = NULL;
    Camel* tmp2      = NULL; // only used if going backwards
    Camel curr_camel = {0};

    Camel** stack      = &game->board[curr_space].camel_stack;
    Camel** dest_stack = &game->board[dest].camel_stack;

    // what if dest == stack

    bool match_found = false;
    while (!match_found) { // pop from start stack until we reach desired camel
        curr_camel  = stack_pop(*stack);
        match_found = curr_camel.color == color ? true : false;
        stack_push(tmp, curr_camel);
    }

    // if camels are going in reverse, pop the dest stack into a tmp stack, then push start stack into dest and then
    // push dest stack
    if (move_orientation == REVERSE) {
        while (!stack_empty(*dest_stack)) {
            curr_camel = stack_pop(*dest_stack);
            stack_push(tmp2, curr_camel);
        }
    }

    while (!stack_empty(tmp)) {
        curr_camel = stack_pop(tmp);
        // printf("pushed: %s ", enum2char(curr_camel.color));
        curr_camel.space = dest;
        stack_push(*dest_stack, curr_camel);
    }

    if (move_orientation == REVERSE) {
        while (!stack_empty(tmp2)) {
            curr_camel       = stack_pop(tmp2);
            curr_camel.space = dest;
            stack_push(*dest_stack, curr_camel);
        }
    }
}

//////////////////////////////////// I/O //////////////////////////////////////

// void get_user_input(Game* game, Player* player) {
//     // pick a ticket
//
//     // place a Spectator tile
//
//     // wager on winner/loser
//
//     // roll dice
// }

void render_game(Game* game) {
    // printf("\e[1;1H\e[2J");
    for (int i = 0; i < BOARD_SIZE; i++) {

        Tile tile = game->board[i];

        printf("%d\t", i);

        if (tile.has_spec) {
            char* val = tile.spec.orientation == FORWARD ? "+1" : "-1";
            printf("[%s | p%d] ", val, tile.spec.player);
        } else {
            for (size_t j = 0; j < stack_count(tile.camel_stack); j++) {
                printf("%s ", enum2char(tile.camel_stack[j].color));
            }
        }
        printf("\n");
    }
}

void render_horizontal(Game* game) {

    for (size_t i = N_CAMELS; i > 0; i--) {
        printf(" %zu | ", i);
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (stack_count(game->board[j].camel_stack) >= i) {
                printf(" %2s ", enum2char(game->board[j].camel_stack[i - 1].color));
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
}

#ifndef TEST_BUILD
int main(int argc, char** argv) {
    // srand((unsigned int) time(NULL));
    srand((unsigned int) 4);

    // printf("Hello World!\n");
    Game game = {0};
    init_game(&game);
    render_horizontal(&game);
    int curr_player_id = 0;
    bool end_game      = false;
    while (!end_game) {
        init_round(&game); // reset wager and spec cards
        int rolled_dice = N_DICE - 1;
        while (rolled_dice > 0) {
            Player curr_player = game.players[curr_player_id];

            // get next players input

            // update game state

            // render game state

            curr_player_id = (curr_player.id + 1) % N_PLAYERS;
            rolled_dice--;
        }

        // give out points
    }

    return 0;
}

#endif // TEST_BUILD
