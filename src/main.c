#define DYNAMIC_ARRAY_IMPLEMENTATION
#include "da.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_SIZE 15
#define N_PLAYERS  5
#define N_TICKETS  5
#define N_CAMELS   7
#define N_DICE     5

typedef enum { BRED, BBLUE, BYELLOW, BGREEN, BPURPLE } BetColor;
typedef enum { CRED, CBLUE, CYELLOW, CGREEN, CPURPLE, CWHITE, CBLACK } CamelColor;
typedef enum { DRED, DBLUE, DYELLOW, DGREEN, DPURPLE, DGREY } DiceColor;
typedef enum { FOWARD, REVERSE } Orientation;


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
    int space; // space the camel is on
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
    Bet hand[N_TICKETS];
} Player;

typedef struct {
    int turn;
    Player players[N_PLAYERS];
    Tile board[BOARD_SIZE]; // array of tiles each tile can either be a stack of camels or a spec
    Bet winner_bets[N_PLAYERS * N_TICKETS];
    Bet loser_bets[N_PLAYERS * N_TICKETS];
    Ticket* red_tickets;
    Ticket* blue_tickets;
    Ticket* yellow_tickets;
    Ticket* green_tickets;
    Ticket* purple_tickets;
} Game;

//////////////////////////////////// Tile //////////////////////////////////////

bool camels_empty(CamelStack* stack) { return stack->count == 0 ? true : false; }
bool camels_full(CamelStack* stack) { return stack->count == N_CAMELS ? true : false; }
void camels_push(CamelStack* stack, Camel camel) { stack->camels[stack->count++] = camel; }
Camel camels_pop(CamelStack* stack) {
    Camel camel = stack->camels[--stack->count];
    return camel;
}
void place_camel(Game* game, int space, Camel camel) { camels_push(&game->board[space].camel_stack, camel); }

//////////////////////////////////// Game State //////////////////////////////////////

void init_game(void) {
    // alloc/set up all the game elements above
}

// void init_round(Game* game) {};
//
// void end_round(Game* game) {};

//////////////////////////////////// Turn //////////////////////////////////////

// assign ticket on top of stack to player
bool assign_ticket(TicketStack* ts, Player* player) {
    if (ts->count < 0) {
        return false;
    }
    ts->tickets[ts->count].player_id = player->id;
    ts->count--;
    return true;
}

// void list_available_tickets(Game* game, Ticket* tickets) {};
// void make_winner_wager() {};
// void make_loser_wafer() {};
bool place_spec_tile(Game* game, int space, Spectator spec) {
    // check if spec next to tile or on tile
    if (game->board[space].camel_stack.count > 0 || game->board[space].has_spec ||
        (space != BOARD_SIZE && game->board[space + 1].has_spec) || (space != 0 && game->board[space - 1].has_spec)) {
        return false;
    }
    game->board[space].has_spec = true;
    game->board[space].spec     = spec;
    return true;
}

void roll_dice(Roll* die) {
    // randomly select color from DiceColor
    srand((unsigned int) time(NULL));

    int random_int = rand() % N_DICE + 1;

    // Cast the random integer to the enum type
    DiceColor random_color = (DiceColor) random_int;
    if (random_color == DGREY) {
        // if Grey randomly select 0 or 1 for black or white
        die->color = (CamelColor) ((rand() % 2) + N_TICKETS);
    } else {
        die->color = (CamelColor) random_color;
    }

    die->value = (rand() % 3) + 1;
}

void move_camel(Game* game, Camel* camel, int spaces) {
    printf("COLOR: %s", enum2char(camel->color));
    int dest;
    int curr_space = camel->space;

    if (game->board[curr_space + spaces].has_spec) {
        Spectator spec = game->board[curr_space + spaces].spec;
        game->players[spec.player].points++;          // Give point to player who placed spec
        if (spec.orientation == camel->orientation) { // foward and +1 or reverse and -1
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
    CamelStack tmp    = {0};
    CamelStack* stack = &game->board[curr_space].camel_stack;
    bool match        = false;
    Camel curr_camel  = {0};
    while (!match || !camels_empty(stack)) {
        curr_camel = camels_pop(stack);
        camels_push(&tmp, curr_camel);
        printf("%s", enum2char(curr_camel.color));
        printf("%s", enum2char(camel->color));
        match = curr_camel.color == camel->color ? true : false;
    }

    while (!camels_empty(&tmp)) {
        curr_camel       = camels_pop(&tmp);
        curr_camel.space = dest;
        camels_push(&game->board[dest].camel_stack, curr_camel);
    }
    // if (camel->orientation == REVERSE) {
    //
    //     // move all camels in stack to dest
    //     CamelStack tmp2    = {0};
    //     CamelStack* stack2 = &game->board[dest].camel_stack;
    //     match              = false;
    //
    //     while (!match) {
    //         curr_camel = camels_pop(stack2);
    //         camels_push(&tmp2, curr_camel);
    //         match = (curr_camel.color == camel->color) ? true : false;
    //     }
    //
    //     while (!camels_empty(&tmp2)) {
    //         curr_camel       = camels_pop(&tmp2);
    //         curr_camel.space = dest;
    //         camels_push(&game->board[dest].camel_stack, curr_camel);
    //     }
    // }
}

//////////////////////////////////// I/O //////////////////////////////////////

void get_user_input(Game* game, Player* player) {
    // pick a ticket

    // place a Spectator tile

    // wager on winner/loser

    // roll dice
}

void render_game(Game* game) {
    printf("\n\n");
    for (int i = 0; i <= BOARD_SIZE; i++) {

        Tile tile = game->board[i];

        printf("%d\t", i + 1);

        if (tile.has_spec) {
            char* val = tile.spec.orientation == FOWARD ? "+1" : "-1";
            printf("[%s | p%d] ", val, tile.spec.player);
        } else {
            int count = tile.camel_stack.count;
            for (int j = count; j > 0; --j) {
                printf("%s", enum2char(tile.camel_stack.camels[j].color));
            }
        }
        printf("\n");
    }
}

#ifndef TEST_BUILD
int main(int argc, char** argv) {

    printf("Hello World!\n");
    Game game = {0};
    Camel c   = {.color = CGREEN, .orientation = FOWARD, .space = 2};
    place_camel(&game, 0, c);
    Spectator s = {.orientation = FOWARD, .player = 2};
    place_spec_tile(&game, 2, s);
    render_game(&game);

    move_camel(&game, &c, 3);
    render_game(&game);

    // int curr_player_id = 0;
    //
    // bool end_game = false;
    // while (!end_game) {
    //     int rolled_dice = N_DICE - 1;
    //     while (rolled_dice > 0) {
    //         Player curr_player = game.players[curr_player_id];
    //
    //         // get next players input
    //         // update game state
    //         // render game state
    //
    //         curr_player_id = (curr_player.id + 1) % N_PLAYERS;
    //         rolled_dice--;
    //     }
    //
    //     // give out points
    //     // reset wagers and specs
    // }

    return 0;
}

#endif // TEST_BUILD
