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
- convert Orientation to bool
*/

#define BOARD_SIZE 17 // 16 for real game 17th is for winning camels
#define N_PLAYERS  5
#define N_DICE     5

#define N_BETS_COLORS 5 // 1 per regular camel color
#define N_TICKETS     4 // 5,3,2,2
#define N_CAMELS      7 // reg + 2 crazy
#define MAX_WAGERS    N_PLAYERS* N_BETS_COLORS

typedef enum { BRED, BBLUE, BYELLOW, BGREEN, BPURPLE } BetColor;
typedef enum { CRED, CBLUE, CYELLOW, CGREEN, CPURPLE, CWHITE, CBLACK } CamelColor;
typedef enum { DRED, DBLUE, DYELLOW, DGREEN, DPURPLE, DGREY } DiceColor;
typedef enum { FORWARD, REVERSE } Orientation;
typedef enum { WAGER, ROLL, TICKET, SPECTATOR } TurnType;

const char* oreint2char(Orientation oreint) {
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
    int hand_size;
    BetColor hand[N_BETS_COLORS]; // the color cards you have left to bet for dinner or loser
} Player;

typedef struct {
    bool winner;
    int turn;
    int round_rolled_dice;
    Roll die;
    Tile board[BOARD_SIZE]; // array of tiles each tile can either be a stack of camels or a spec - add one for winnner
                            // spot
    Player players[N_PLAYERS];
    Bet winner_bets[MAX_WAGERS];
    Bet loser_bets[MAX_WAGERS];
    Ticket tickets[N_BETS_COLORS][N_TICKETS];
} Game;

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

            game->tickets[i][j] = t;
        }
    }
}

void init_game(Game* game) {

    Roll die                = {0};
    game->die               = die;
    game->round_rolled_dice = N_DICE - 1;
    game->winner            = false;

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
            starting_tile     = BOARD_SIZE - starting_tile - 2;
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
    reset_tickets(game);

    ///// Wagers /////////
}

void assign_points(Game* game, CamelColor top, CamelColor second) {

    // assign points to anyone holding a winning color ticket
    for (int i = 0; i < N_BETS_COLORS; i++) {
        for (int j = 0; j < N_TICKETS; j++) {
            Ticket t = game->tickets[i][j];
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

void get_top_camels(Game* game, int* first, int* second) {

    // get top_camel and second place dont count black and white camel
    *first  = -1;
    *second = -1;

    for (int b = BOARD_SIZE - 1; b > 0; b--) {
        Camel** stack = &game->board[b].camel_stack;
        for (size_t j = stack_count(*stack); j > 0; j--) {
            CamelColor top = (*stack)[j - 1].color;
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

void end_round(Game* game, int* first, int* second) {
    get_top_camels(game, first, second);
    assign_points(game, (CamelColor) *first, (CamelColor) *second);

    // reset dice count
    game->round_rolled_dice = N_DICE - 1;

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

    Ticket* tickets = game->tickets[(int) color];

    int available = -1;

    for (int i = 0; i < N_TICKETS; i++) {
        if (tickets[i].player_id == -1) {
            available            = tickets[i].amount;
            tickets[i].player_id = player_id;
            return available;
        }
    }
    return available;
}

void get_available_tickets(Game* game, Ticket* tickets) {
    for (int i = 0; i < N_BETS_COLORS; i++) {
        for (int j = 0; j < N_TICKETS; j++) {
            if (game->tickets[i][j].player_id == -1) {
                da_append(tickets, game->tickets[i][j]);
            }
        }
    }
}

void get_possible_spec_location(Game* game, int* buff) {
    for (int i = 0; i < BOARD_SIZE - 1; i++) { // cant put peice on last spot
        if (!(stack_count(game->board[i].camel_stack) > 0 || game->board[i].has_spec ||
              (i != BOARD_SIZE && game->board[i + 1].has_spec) || (i != 0 && game->board[i - 1].has_spec))) {
            da_append(buff, i);
        }
    }
}

bool place_spec_tile(Game* game, int player_id, int space, Spectator spec) {

    if (game->players[player_id].used_spec) {
        return false;
    }

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

    if (dest >= BOARD_SIZE - 1) {
        game->winner = true;
        dest         = BOARD_SIZE - 1;
    }

    // move all camels in stack to dest
    Camel* tmp       = NULL;
    Camel* tmp2      = NULL; // only used if going backwards
    Camel curr_camel = {0};

    Camel** stack      = &game->board[curr_space].camel_stack;
    Camel** dest_stack = &game->board[dest].camel_stack;

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

Turn get_user_input(Game* game, int player_id) {
    printf("[R]oll, [W]ager, Take a [T]icket or Place [S]pectator");
    Turn turn = {0};
    char input_char;
    scanf("%c", &input_char);
    switch (input_char) {
    case 'R': {
        turn.turn_type = ROLL;
        break;
    };
    case 'W': {
        if (game->players[player_id].hand_size < 0) {
            printf("No cards to wager");
            return get_user_input(game, player_id);
        }
        printf("[W]inner or [L]oser");
        break;
    };

    case 'T': {
        printf("Select ticket [R]ED, [B]LUE, [Y]ELLOW, [G]REEN, [P]URPLE");
        scanf("%c", &input_char);
        switch (input_char) {
        case 'R': {
            turn.color = BRED;
            break;
        };
        case 'B': {
            turn.color = BBLUE;
            break;
        };
        case 'Y': {
            turn.color = BYELLOW;
            break;
        };
        case 'G': {
            turn.color = BGREEN;
            break;
        };
        case 'P': {
            turn.color = BPURPLE;
            break;
        };
        default:
            return get_user_input(game, player_id);
            break;
        }
    }
    case 'S': {
        int* buff = NULL;
        get_possible_spec_location(game, buff);
        printf("Pick a location to place the Spectator tile:");
        for (size_t i = 0; i < da_count(buff); i++) {
            printf(" [%d] ", buff[i]);
        }
        int pos = 0;
        scanf("%d", &pos);
        turn.position = pos;
        printf("[+]1 or [-]1");
        scanf("%c", &input_char);
        turn.orientation = input_char == '+' ? FORWARD : REVERSE;
        break;
    };
    default: {
        return get_user_input(game, player_id);
        break;
    }
    }

    return turn;
}

void render_horizontal(Game* game) {

    // render wagers

    // printf("Wagers\n");
    // printf("Winners: \n");
    //
    // for (int i = 0; i < MAX_WAGERS; i++) {
    //
    // }

    // render tickets

    printf("\nTickets\n");

    for (int i = 0; i < N_BETS_COLORS; i++) {
        BetColor color = (BetColor) i;
        printf(" %s | ", enum2char((CamelColor) color)); // todo fix cast?
        for (int j = 0; j < N_TICKETS; j++) {
            if (game->tickets[i][j].player_id == -1) {
                printf(" [%d] ", game->tickets[i][j].amount);
            } else {

                printf("     ");
            }
        }
        printf("\n");
    }

    printf("\nPlayers\n");

    for (int i = 0; i < N_PLAYERS; i++) {
        printf(" %d (%d)| ", game->players[i].id, game->players[i].points);
        for (int j = 0; j < N_BETS_COLORS; j++) {
            for (int k = 0; k < N_TICKETS; k++) {
                if (game->tickets[j][k].player_id == i) {
                    printf(" [%s:%d] ", enum2char((CamelColor) game->tickets[j][k].color), game->tickets[j][k].amount);
                }
            }
        }

        printf("\n");
    }

    printf("\nBoard\n");

    // render board
    for (size_t i = N_CAMELS; i > 0; i--) {
        printf(" %zu | ", i);
        for (size_t j = 0; j < BOARD_SIZE; j++) {

            if (game->board[j].has_spec && i == 1) {
                printf(" %s1 ", oreint2char(game->board[j].spec.orientation));
                continue;
            } else if (stack_count(game->board[j].camel_stack) >= i) {
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
    int first, second;

    while (!game.winner) {
        while (game.round_rolled_dice > 0 && !game.winner) {
            Player curr_player = game.players[curr_player_id];

            Turn turn      = {0};
            turn.turn_type = ROLL;
            // turn.color       = BPURPLE;
            // turn.orientation = FORWARD;
            // turn.position    = 5;

            // get next players input
            switch (turn.turn_type) {
            case WAGER: {
                printf("MAKE A WAGER");
                break;
            }
            case ROLL: {
                roll_dice(&game.die);
                printf("%d rolled %s%d\n", curr_player_id, enum2char((CamelColor) game.die.color), game.die.value);
                move_camel(&game, game.die.color, game.die.value);
                game.round_rolled_dice--;
                break;
            }
            case TICKET: {
                int amount = assign_ticket(&game, turn.color, curr_player_id);
                if (amount == -1) {
                    printf("Could not assign ticket\n");
                    // TODO: restart turn?
                }
                printf("%d Picked a %s:%d Ticket\n", curr_player_id, enum2char((CamelColor) turn.color), amount);
                break;
            }

            case SPECTATOR: {
                printf("Place Spectator card");
                Spectator spec = {.orientation = turn.orientation, .player = curr_player_id};
                bool allowed   = place_spec_tile(&game, curr_player_id, turn.position, spec);
                if (!allowed) {
                    printf("Could not place tile");
                    // TODO: restart turn?
                }
                break;
            }
            default:
                break;
            }

            // render game state

            printf("\e[1;1H\e[2J"); // clear screen
            render_horizontal(&game);

            curr_player_id = (curr_player.id + 1) % N_PLAYERS;
            printf("Press Any Key to Continue\n");
            getchar();
            // game.winner    = true;
        }

        // give out points for round
        end_round(&game, &first, &second);
        render_horizontal(&game);
        printf("First place: %s\tSecond Place: %s\n", enum2char((CamelColor) first), enum2char((CamelColor) second));
    }
    // winning_player = end_game(); // give out wagers and select winner
    // render again showing winner

    return 0;
}

#endif // TEST_BUILD
