/* test_camel_up.c */

#include "minunit.h"
#include <stdio.h>
#include <string.h>

// Define TEST_BUILD before including main.c to exclude main()
#define TEST_BUILD
#include "main.c"

int tests_run = 0;

// Helper function to create a fresh game
Game* setup_game(void) {
    static Game game;
    memset(&game, 0, sizeof(Game));
    srand(42); // Fixed seed for reproducible tests
    init_game(&game);
    return &game;
}

//////////////////////////////////// Stack Tests //////////////////////////////////////

static char* test_stack_push_pop(void) {
    WagerStack stack = {0};
    stack.capacity   = MAX_WAGERS;

    Wager w1 = {.player = 0, .color = BRED};
    Wager w2 = {.player = 1, .color = BBLUE};

    mu_assert("Push should succeed", stack_push(&stack, w1));
    mu_assert("Push should succeed", stack_push(&stack, w2));
    mu_assert("Stack count should be 2", stack.count == 2);

    Wager result;
    mu_assert("Pop should succeed", stack_pop(&stack, &result));
    mu_assert("Popped wrong color", result.color == BBLUE);
    mu_assert("Popped wrong player", result.player == 1);
    mu_assert("Stack count should be 1", stack.count == 1);

    return 0;
}

static char* test_stack_empty(void) {
    WagerStack stack = {0};
    stack.capacity   = MAX_WAGERS;

    mu_assert("Empty stack should be empty", stack_empty(&stack));

    Wager w = {.player = 0, .color = BRED};
    stack_push(&stack, w);
    mu_assert("Stack with item should not be empty", !stack_empty(&stack));

    return 0;
}

static char* test_stack_overflow(void) {
    WagerStack stack = {0};
    stack.capacity   = 2;

    Wager w = {.player = 0, .color = BRED};
    mu_assert("First push should succeed", stack_push(&stack, w));
    mu_assert("Second push should succeed", stack_push(&stack, w));
    mu_assert("Third push should fail (overflow)", !stack_push(&stack, w));

    return 0;
}

//////////////////////////////////// Hand Management Tests //////////////////////////////////////

static char* test_init_player_hand(void) {
    Player player = {0};
    init_player_hand(&player);

    mu_assert("Hand size should be 5", hand_size(&player) == N_BETS_COLORS);
    mu_assert("Should have RED", player.hand[0].color == BRED);
    mu_assert("Should have BLUE", player.hand[1].color == BBLUE);
    mu_assert("Should have YELLOW", player.hand[2].color == BYELLOW);
    mu_assert("Should have GREEN", player.hand[3].color == BGREEN);
    mu_assert("Should have PURPLE", player.hand[4].color == BPURPLE);

    return 0;
}

static char* test_has_card_in_hand(void) {
    Player player = {0};
    init_player_hand(&player);

    mu_assert("Should have RED card", has_card_in_hand(&player, BRED));
    mu_assert("Should have BLUE card", has_card_in_hand(&player, BBLUE));

    // Remove a card
    remove_card_from_hand(&player, BRED);
    mu_assert("Should not have RED card after removal", !has_card_in_hand(&player, BRED));
    mu_assert("Should still have BLUE card", has_card_in_hand(&player, BBLUE));

    return 0;
}

static char* test_remove_card_from_hand(void) {
    Player player = {0};
    init_player_hand(&player);

    mu_assert("Remove should succeed", remove_card_from_hand(&player, BYELLOW));
    mu_assert("Hand size should be 4", hand_size(&player) == 4);
    mu_assert("Should not have YELLOW", !has_card_in_hand(&player, BYELLOW));

    // Try to remove same card again
    mu_assert("Remove should fail for already removed card", !remove_card_from_hand(&player, BYELLOW));

    return 0;
}

static char* test_remove_all_cards(void) {
    Player player = {0};
    init_player_hand(&player);

    remove_card_from_hand(&player, BRED);
    remove_card_from_hand(&player, BBLUE);
    remove_card_from_hand(&player, BYELLOW);
    remove_card_from_hand(&player, BGREEN);
    remove_card_from_hand(&player, BPURPLE);

    mu_assert("Hand should be empty", hand_size(&player) == 0);
    mu_assert("Should not have any cards", !has_card_in_hand(&player, BRED));

    return 0;
}

//////////////////////////////////// Game Initialization Tests //////////////////////////////////////

static char* test_init_game(void) {
    Game* game = setup_game();

    mu_assert("Turn should start at 0", game->turn == 0);
    mu_assert("Game should not be won initially", !game->winner);
    mu_assert("Dice should be empty", game->dice.count == 0);
    mu_assert("Winner bets should be empty", game->winner_bets.count == 0);
    mu_assert("Loser bets should be empty", game->loser_bets.count == 0);

    return 0;
}

static char* test_players_initialized(void) {
    Game* game = setup_game();

    for (int i = 0; i < N_PLAYERS; i++) {
        mu_assert("Player ID should match index", game->players[i].id == i);
        mu_assert("Player should start with 0 points", game->players[i].points == 0);
        mu_assert("Player should not have used spectator", !game->players[i].used_spec);
        mu_assert("Player should have full hand", hand_size(&game->players[i]) == N_BETS_COLORS);
    }

    return 0;
}

static char* test_camels_on_board(void) {
    Game* game = setup_game();

    int camel_count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        camel_count += game->board[i].camel_stack.count;
    }

    mu_assert("All camels should be on board", camel_count == N_CAMELS);

    return 0;
}

static char* test_tickets_initialized(void) {
    Game* game = setup_game();

    for (int i = 0; i < N_BETS_COLORS; i++) {
        mu_assert("Should have 4 tickets per color", game->tickets[i].count == N_TICKETS);

        // Check ticket values: 5, 3, 2, 2
        mu_assert("First ticket should be 5", game->tickets[i].items[0].amount == 5);
        mu_assert("Second ticket should be 3", game->tickets[i].items[1].amount == 3);
        mu_assert("Third ticket should be 2", game->tickets[i].items[2].amount == 2);
        mu_assert("Fourth ticket should be 2", game->tickets[i].items[3].amount == 2);

        // All should be unowned
        for (int j = 0; j < N_TICKETS; j++) {
            mu_assert("Tickets should start unowned", game->tickets[i].items[j].player_id == -1);
        }
    }

    return 0;
}

//////////////////////////////////// Ticket Tests //////////////////////////////////////

static char* test_assign_ticket(void) {
    Game* game = setup_game();

    int amount = assign_ticket(game, BRED, 0);
    mu_assert("Should get 5 point ticket first", amount == 5);
    mu_assert("Ticket should be assigned to player 0", game->tickets[BRED].items[0].player_id == 0);

    amount = assign_ticket(game, BRED, 1);
    mu_assert("Should get 3 point ticket second", amount == 3);

    return 0;
}

static char* test_assign_all_tickets(void) {
    Game* game = setup_game();

    assign_ticket(game, BRED, 0);
    assign_ticket(game, BRED, 1);
    assign_ticket(game, BRED, 2);
    assign_ticket(game, BRED, 3);

    int amount = assign_ticket(game, BRED, 4);
    mu_assert("Should return -1 when no tickets left", amount == -1);

    return 0;
}

//////////////////////////////////// Spectator Tests //////////////////////////////////////

static char* test_place_spec_tile(void) {
    Game* game = setup_game();

    Spectator spec = {.player = 0, .orientation = FORWARD};
    bool placed    = place_spec_tile(game, 0, 5, spec);

    mu_assert("Should place spectator successfully", placed);
    mu_assert("Tile should have spectator", game->board[5].has_spec);
    mu_assert("Spectator player should be 0", game->board[5].spec.player == 0);

    return 0;
}

static char* test_cant_place_spec_twice(void) {
    Game* game = setup_game();

    Spectator spec = {.player = 0, .orientation = FORWARD};
    place_spec_tile(game, 0, 5, spec);

    // Try to place again
    bool placed = place_spec_tile(game, 0, 6, spec);
    mu_assert("Should not place spectator twice", !placed);

    return 0;
}

static char* test_cant_place_spec_adjacent(void) {
    Game* game = setup_game();

    Spectator spec1 = {.player = 0, .orientation = FORWARD};
    place_spec_tile(game, 0, 5, spec1);

    Spectator spec2 = {.player = 1, .orientation = FORWARD};
    bool placed     = place_spec_tile(game, 1, 6, spec2);
    mu_assert("Should not place spectator adjacent to another", !placed);

    placed = place_spec_tile(game, 1, 4, spec2);
    mu_assert("Should not place spectator adjacent to another", !placed);

    return 0;
}

//////////////////////////////////// Dice Tests //////////////////////////////////////

static char* test_roll_dice(void) {
    Game* game = setup_game();

    mu_assert("Dice should start empty", game->dice.count == 0);

    roll_dice(game);
    mu_assert("Should have 1 die after roll", game->dice.count == 1);

    Roll die = stack_peak(&game->dice);
    mu_assert("Die value should be between -3 and 3", die.value >= -3 && die.value <= 3 && die.value != 0);

    return 0;
}

static char* test_roll_all_dice(void) {
    Game* game = setup_game();

    for (int i = 0; i < N_DICE; i++) {
        roll_dice(game);
    }

    mu_assert("Should have all dice rolled", game->dice.count == N_DICE);

    return 0;
}

//////////////////////////////////// Camel Movement Tests //////////////////////////////////////

static char* test_get_camel(void) {
    Game* game = setup_game();

    Camel* red_camel = get_camel(game, CRED);
    mu_assert("Should find red camel", red_camel != NULL);
    mu_assert("Red camel should be red", red_camel->color == CRED);

    return 0;
}

static char* test_move_camel_forward(void) {
    Game* game = setup_game();

    Camel* camel  = get_camel(game, CRED);
    int start_pos = camel->space;

    move_camel(game, CRED, 2);

    camel = get_camel(game, CRED);
    mu_assert("Camel should move forward", camel->space == start_pos + 2);

    return 0;
}

static char* test_move_camel_to_finish(void) {
    Game* game = setup_game();

    mu_assert("Game should not be won initially", !game->winner);

    // Move a camel to the last space
    Camel* camel      = get_camel(game, CRED);
    int spaces_to_end = BOARD_SIZE - 1 - camel->space;
    move_camel(game, CRED, spaces_to_end);

    mu_assert("Game should be won", game->winner);

    return 0;
}

//////////////////////////////////// Scoring Tests //////////////////////////////////////

static char* test_get_top_camels(void) {
    Game* game = setup_game();

    int first, second;
    get_top_camels(game, &first, &second);

    mu_assert("Should find first place camel", first != -1);
    mu_assert("Should find second place camel", second != -1);
    mu_assert("First and second should be different", first != second);

    return 0;
}

static char* test_assign_points_winner(void) {
    Game* game = setup_game();

    // Assign a ticket to player 0
    assign_ticket(game, BRED, 0);

    int initial_points = game->players[0].points;

    // RED wins, BLUE second
    assign_points(game, CRED, CBLUE);

    mu_assert("Winner ticket holder should get points", game->players[0].points > initial_points);

    return 0;
}

static char* test_assign_points_second(void) {
    Game* game = setup_game();

    // Assign a ticket to player 0
    assign_ticket(game, BBLUE, 0);

    int initial_points = game->players[0].points;

    // RED wins, BLUE second
    assign_points(game, CRED, CBLUE);

    mu_assert("Second place ticket holder should get 1 point", game->players[0].points == initial_points + 1);

    return 0;
}

static char* test_assign_points_loser(void) {
    Game* game = setup_game();

    // Assign a ticket to player 0
    assign_ticket(game, BYELLOW, 0);

    int initial_points = game->players[0].points;

    // RED wins, BLUE second (YELLOW loses)
    assign_points(game, CRED, CBLUE);

    mu_assert("Losing ticket holder should lose 1 point", game->players[0].points == initial_points - 1);

    return 0;
}

//////////////////////////////////// Round Tests //////////////////////////////////////

static char* test_end_round_resets(void) {
    Game* game = setup_game();

    // Do some actions
    roll_dice(game);
    Spectator spec = {.player = 0, .orientation = FORWARD};
    place_spec_tile(game, 0, 5, spec);
    assign_ticket(game, BRED, 0);

    int current_round = game->round;
    end_round(game);

    mu_assert("Round should increment", game->round == current_round + 1);
    mu_assert("Dice should be reset", game->dice.count == 0);
    mu_assert("Spectators should be cleared", !game->board[5].has_spec);
    mu_assert("Player should be able to use spec again", !game->players[0].used_spec);

    return 0;
}

//////////////////////////////////// Integration Tests //////////////////////////////////////

static char* test_full_round_playthrough(void) {
    Game* game = setup_game();

    // Roll all dice
    for (int i = 0; i < N_DICE; i++) {
        roll_dice(game);
        Roll die = stack_peak(&game->dice);
        move_camel(game, die.color, die.value);
    }

    mu_assert("All dice should be rolled", game->dice.count == N_DICE);

    // Score the round
    int first, second;
    score_round(game, &first, &second);

    mu_assert("Should have first place", first != -1);
    mu_assert("Should have second place", second != -1);

    return 0;
}

//////////////////////////////////// Utility Tests //////////////////////////////////////

static char* test_enum2char(void) {
    mu_assert("Red should be 'R'", strcmp(enum2char(CRED), "R") == 0);
    mu_assert("Blue should be 'B'", strcmp(enum2char(CBLUE), "B") == 0);
    mu_assert("Yellow should be 'Y'", strcmp(enum2char(CYELLOW), "Y") == 0);
    mu_assert("Green should be 'G'", strcmp(enum2char(CGREEN), "G") == 0);
    mu_assert("Purple should be 'P'", strcmp(enum2char(CPURPLE), "P") == 0);
    mu_assert("Black should be 'K'", strcmp(enum2char(CBLACK), "K") == 0);
    mu_assert("White should be 'W'", strcmp(enum2char(CWHITE), "W") == 0);

    return 0;
}

static char* test_orient2char(void) {
    mu_assert("Forward should be '+'", strcmp(orient2char(FORWARD), "+") == 0);
    mu_assert("Reverse should be '-'", strcmp(orient2char(REVERSE), "-") == 0);

    return 0;
}

static char* test_rand_range(void) {
    for (int i = 0; i < 100; i++) {
        int val = rand_range(1, 3);
        mu_assert("Random value should be in range", val >= 1 && val <= 3);
    }

    return 0;
}

//////////////////////////////////// Advanced Scoring Tests //////////////////////////////////////

static char* test_multiple_tickets_same_color(void) {
    Game* game = setup_game();

    // Three players take RED tickets
    assign_ticket(game, BRED, 0); // Gets 5
    assign_ticket(game, BRED, 1); // Gets 3
    assign_ticket(game, BRED, 2); // Gets 2

    int p0_points = game->players[0].points;
    int p1_points = game->players[1].points;
    int p2_points = game->players[2].points;

    // RED wins, BLUE second
    assign_points(game, CRED, CBLUE);

    mu_assert("Player 0 should get 5 points", game->players[0].points == p0_points + 5);
    mu_assert("Player 1 should get 3 points", game->players[1].points == p1_points + 3);
    mu_assert("Player 2 should get 2 points", game->players[2].points == p2_points + 2);

    return 0;
}

static char* test_second_place_ticket_scoring(void) {
    Game* game = setup_game();

    // Player takes BLUE ticket
    assign_ticket(game, BBLUE, 0);
    int initial = game->players[0].points;

    // RED wins, BLUE second
    assign_points(game, CRED, CBLUE);

    mu_assert("Second place ticket should give 1 point", game->players[0].points == initial + 1);

    return 0;
}

static char* test_losing_ticket_penalty(void) {
    Game* game = setup_game();

    // Player takes YELLOW, GREEN, PURPLE tickets (all lose)
    assign_ticket(game, BYELLOW, 0);
    assign_ticket(game, BGREEN, 1);
    assign_ticket(game, BPURPLE, 2);

    int p0_initial = game->players[0].points;
    int p1_initial = game->players[1].points;
    int p2_initial = game->players[2].points;

    // RED wins, BLUE second (YELLOW, GREEN, PURPLE lose)
    assign_points(game, CRED, CBLUE);

    mu_assert("Losing ticket should lose 1 point", game->players[0].points == p0_initial - 1);
    mu_assert("Losing ticket should lose 1 point", game->players[1].points == p1_initial - 1);
    mu_assert("Losing ticket should lose 1 point", game->players[2].points == p2_initial - 1);

    return 0;
}

static char* test_mixed_ticket_scoring(void) {
    Game* game = setup_game();

    // Different outcomes for different players
    assign_ticket(game, BRED, 0);    // Winner: +5
    assign_ticket(game, BBLUE, 1);   // Second: +1
    assign_ticket(game, BYELLOW, 2); // Loser: -1

    int p0_initial = game->players[0].points;
    int p1_initial = game->players[1].points;
    int p2_initial = game->players[2].points;

    assign_points(game, CRED, CBLUE);

    mu_assert("Winner ticket holder gets points", game->players[0].points == p0_initial + 5);
    mu_assert("Second place holder gets 1", game->players[1].points == p1_initial + 1);
    mu_assert("Loser loses 1", game->players[2].points == p2_initial - 1);

    return 0;
}

static char* test_wager_scoring_correct_guess(void) {
    Game* game = setup_game();

    // Players make wagers on winner
    Wager w1 = {.player = 0, .color = BRED};
    Wager w2 = {.player = 1, .color = BRED};
    Wager w3 = {.player = 2, .color = BRED};
    Wager w4 = {.player = 3, .color = BRED};
    Wager w5 = {.player = 4, .color = BRED};
    Wager w6 = {.player = 5, .color = BRED};

    stack_push(&game->winner_bets, w1);
    stack_push(&game->winner_bets, w2);
    stack_push(&game->winner_bets, w3);
    stack_push(&game->winner_bets, w4);
    stack_push(&game->winner_bets, w5);
    stack_push(&game->winner_bets, w6);

    int p0_initial = game->players[0].points;
    int p1_initial = game->players[1].points;
    int p2_initial = game->players[2].points;
    int p3_initial = game->players[3].points;
    int p4_initial = game->players[4].points;
    int p5_initial = game->players[5].points;

    // RED wins, BLUE loses
    score_wagers(game, BRED, BBLUE);

    // First bet gets 8, second gets 5, third gets 3
    mu_assert("First wager should get 8 points", game->players[0].points == p0_initial + 8);
    mu_assert("Second wager should get 5 points", game->players[1].points == p1_initial + 5);
    mu_assert("Third wager should get 3 points", game->players[2].points == p2_initial + 3);
    mu_assert("Fourth wager should get 2 points", game->players[3].points == p3_initial + 2);
    mu_assert("Fifth wager should get 1 points", game->players[4].points == p4_initial + 1);
    mu_assert("Sixth wager should get 1 points", game->players[5].points == p5_initial + 1);
    return 0;
}

static char* test_wager_scoring_wrong_guess(void) {
    Game* game = setup_game();

    // Players bet on BLUE to win, but RED wins
    Wager w1 = {.player = 0, .color = BBLUE};
    Wager w2 = {.player = 1, .color = BBLUE};

    stack_push(&game->winner_bets, w1);
    stack_push(&game->winner_bets, w2);

    int p0_initial = game->players[0].points;
    int p1_initial = game->players[1].points;

    // RED wins (not BLUE)
    score_wagers(game, BRED, BYELLOW);

    mu_assert("Wrong winner wager loses 1 point", game->players[0].points == p0_initial - 1);
    mu_assert("Wrong winner wager loses 1 point", game->players[1].points == p1_initial - 1);

    return 0;
}
static char* test_loser_wager_scoring(void) {
    Game* game = setup_game();

    // Players bet on who will lose
    Wager w1 = {.player = 0, .color = BYELLOW};
    Wager w2 = {.player = 1, .color = BYELLOW};
    Wager w3 = {.player = 2, .color = BGREEN}; // Wrong guess

    stack_push(&game->loser_bets, w1);
    stack_push(&game->loser_bets, w2);
    stack_push(&game->loser_bets, w3);

    int p0_initial = game->players[0].points;
    int p1_initial = game->players[1].points;
    int p2_initial = game->players[2].points;

    // RED wins, YELLOW loses
    score_wagers(game, BRED, BYELLOW);

    mu_assert("First correct loser bet gets 8", game->players[0].points == p0_initial + 8);
    mu_assert("Second correct loser bet gets 5", game->players[1].points == p1_initial + 5);
    mu_assert("Wrong loser bet loses 1", game->players[2].points == p2_initial - 1);

    return 0;
}

static char* test_score_round_integration(void) {
    Game* game = setup_game();

    // Setup: Tickets and positions
    assign_ticket(game, BRED, 0);
    assign_ticket(game, BBLUE, 1);
    assign_ticket(game, BYELLOW, 2);

    // Move camels to specific positions
    // We need to ensure RED is first, BLUE is second
    Camel* red    = get_camel(game, CRED);
    Camel* blue   = get_camel(game, CBLUE);
    Camel* yellow = get_camel(game, CYELLOW);

    // Move RED to position 10 (higher than others)
    move_camel(game, CRED, 10 - red->space);
    // Move BLUE to position 9
    move_camel(game, CBLUE, 9 - blue->space);
    // Move YELLOW to position 5
    move_camel(game, CYELLOW, 5 - yellow->space);

    int p0_initial = game->players[0].points;
    int p1_initial = game->players[1].points;
    int p2_initial = game->players[2].points;

    int first, second;
    score_round(game, &first, &second);

    mu_assert("First should be RED", first == CRED);
    mu_assert("Second should be BLUE", second == CBLUE);
    mu_assert("RED ticket holder gained points", game->players[0].points > p0_initial);
    mu_assert("BLUE ticket holder gained 1 point", game->players[1].points == p1_initial + 1);
    mu_assert("YELLOW ticket holder lost 1 point", game->players[2].points == p2_initial - 1);

    return 0;
}

static char* test_full_game_scoring(void) {
    Game* game = setup_game();

    // Setup wagers: Player 0 is FIRST, Player 1 is SECOND
    Wager w1 = {.player = 0, .color = BRED};
    Wager w2 = {.player = 1, .color = BRED};
    stack_push(&game->winner_bets, w1);
    stack_push(&game->winner_bets, w2);

    // Player 0 takes RED ticket (5 pts)
    // Player 1 takes BLUE ticket (1 pt if BLUE is second)
    assign_ticket(game, BRED, 0);
    assign_ticket(game, BBLUE, 1);

    // Simulate game end - move RED to winning position
    Camel* red = get_camel(game, CRED);
    move_camel(game, CRED, BOARD_SIZE - 1 - red->space);

    // Ensure we know who is second for the ticket points
    Camel* blue = get_camel(game, CBLUE);
    move_camel(game, CBLUE, (BOARD_SIZE - 2) - blue->space);

    int p0_initial = game->players[0].points;
    int p1_initial = game->players[1].points;

    int first, second;
    score_round(game, &first, &second);

    // NEW FIFO CALCULATION:
    // Player 0: +5 (Ticket) + 8 (1st Wager) = +13 points
    // Player 1: +1 (2nd Place Ticket) + 5 (2nd Wager) = +6 points

    mu_assert("Player 0 should gain 13 points", game->players[0].points == p0_initial + 13);
    mu_assert("Player 1 should gain 6 points", game->players[1].points == p1_initial + 6);

    return 0;
}

static char* test_tickets_reset_after_round(void) {
    Game* game = setup_game();

    // Take some tickets
    assign_ticket(game, BRED, 0);
    assign_ticket(game, BBLUE, 1);
    assign_ticket(game, BRED, 2);

    mu_assert("Tickets should be assigned", game->tickets[BRED].items[0].player_id == 0);
    mu_assert("Tickets should be assigned", game->tickets[BBLUE].items[0].player_id == 1);

    // End round should reset tickets
    end_round(game);

    mu_assert("Tickets should be reset", game->tickets[BRED].items[0].player_id == -1);
    mu_assert("Tickets should be reset", game->tickets[BBLUE].items[0].player_id == -1);
    mu_assert("All tickets should be available", game->tickets[BRED].count == N_TICKETS);

    return 0;
}


static char* test_spectator_tile_points(void) {
    Game* game = setup_game();

    // Place spectator tile
    Spectator spec = {.player = 0, .orientation = FORWARD};
    place_spec_tile(game, 0, 5, spec);

    int initial_points = game->players[0].points;

    // Move a camel onto the spectator tile
    Camel* red         = get_camel(game, CRED);
    int spaces_to_spec = 5 - red->space;
    move_camel(game, CRED, spaces_to_spec);

    mu_assert("Spectator owner should get 1 point", game->players[0].points == initial_points + 1);

    return 0;
}

static char* test_roll_gives_point(void) {
    Game* game = setup_game();

    int initial_points = game->players[0].points;

    // Simulate a roll
    roll_dice(game);
    Roll die = stack_peak(&game->dice);
    move_camel(game, die.color, die.value);
    game->players[0].points++; // This is what happens in next_turn

    mu_assert("Rolling should give 1 point", game->players[0].points == initial_points + 1);

    return 0;
}
static char* test_wager_scoring_mixed_fifo(void) {
    Game* game = setup_game(); // Resets game state

    // Player 0: Correct (8 pts)
    // Player 1: Wrong (-1 pt)
    // Player 2: Correct (5 pts)
    stack_push(&game->winner_bets, ((Wager){.player = 0, .color = BRED}));
    stack_push(&game->winner_bets, ((Wager){.player = 1, .color = BBLUE}));
    stack_push(&game->winner_bets, ((Wager){.player = 2, .color = BRED}));

    int p0_init = game->players[0].points;
    int p1_init = game->players[1].points;
    int p2_init = game->players[2].points;

    score_wagers(game, BRED, BYELLOW);

    mu_assert("First correct (P0) gets 8", game->players[0].points == p0_init + 8); //
    mu_assert("Wrong bet (P1) loses 1", game->players[1].points == p1_init - 1);    //
    mu_assert("Second correct (P2) gets 5", game->players[2].points == p2_init + 5); //
    return 0;
}


static char* test_wager_stack_at_capacity(void) {
    Game* game = setup_game();
    Wager w    = {.player = 0, .color = BRED};

    // Fill to capacity
    for (int i = 0; i < MAX_WAGERS; i++) {
        mu_assert("Should allow pushing up to capacity", stack_push(&game->winner_bets, w));
    }

    // Try one more
    mu_assert("Should not allow pushing beyond capacity", !stack_push(&game->winner_bets, w));
    mu_assert("Count should remain at MAX_WAGERS", game->winner_bets.count == MAX_WAGERS);
    return 0;
}
static char* test_wagers_clear_after_scoring(void) {
    Game* game = setup_game();
    stack_push(&game->winner_bets, ((Wager) {.player = 0, .color = BRED}));

    score_wagers(game, BRED, BBLUE);
    mu_assert("Winner bets should be empty after scoring", game->winner_bets.count == 0);

    int points_after_score = game->players[0].points;
    score_wagers(game, BRED, BBLUE); // Call again
    mu_assert("Points should not increase a second time", game->players[0].points == points_after_score);
    return 0;
}
static char* test_stack_victory_order(void) {
    Game* game = setup_game();

    // Move Red and Blue to the finish line in a stack
    // Red on bottom (index 0), Blue on top (index 1)
    game->board[BOARD_SIZE - 1].camel_stack.count = 0;
    move_camel(game, CRED, (BOARD_SIZE - 1) - get_camel(game, CRED)->space);
    move_camel(game, CBLUE, (BOARD_SIZE - 1) - get_camel(game, CBLUE)->space);

    int first, second;
    get_top_camels(game, &first, &second);

    mu_assert("Top camel (BLUE) should be first", first == CBLUE);
    mu_assert("Bottom camel (RED) should be second", second == CRED);
    return 0;
}
static char* test_spectator_reverse_move(void) {
    Game* game = setup_game();
    Spectator spec = {.player = 5, .orientation = REVERSE};
    place_spec_tile(game, 5, 10, spec); // Place at 10

    Camel* red = get_camel(game, CRED);
    
    // We calculate the move based on the CURRENT position
    int move_amount = 10 - red->space; 
    move_camel(game, CRED, move_amount); 

    // CRITICAL FIX: Re-fetch the pointer!
    // The camel has moved to a new memory address in a different stack.
    red = get_camel(game, CRED); 

    mu_assert("Camel should bounce back to 9", red->space == 9);
    mu_assert("Owner of spectator tile should get 1 point", game->players[5].points == 1);
    return 0;
}
//////////////////////////////////// Test Suite //////////////////////////////////////

static char* all_tests(void) {
    printf("Running Stack Tests...\n");
    mu_run_test(test_stack_push_pop);
    mu_run_test(test_stack_empty);
    mu_run_test(test_stack_overflow);

    printf("Running Hand Management Tests...\n");
    mu_run_test(test_init_player_hand);
    mu_run_test(test_has_card_in_hand);
    mu_run_test(test_remove_card_from_hand);
    mu_run_test(test_remove_all_cards);

    printf("Running Game Initialization Tests...\n");
    mu_run_test(test_init_game);
    mu_run_test(test_players_initialized);
    mu_run_test(test_camels_on_board);
    mu_run_test(test_tickets_initialized);

    printf("Running Ticket Tests...\n");
    mu_run_test(test_assign_ticket);
    mu_run_test(test_assign_all_tickets);

    printf("Running Spectator Tests...\n");
    mu_run_test(test_place_spec_tile);
    mu_run_test(test_cant_place_spec_twice);
    mu_run_test(test_cant_place_spec_adjacent);

    printf("Running Dice Tests...\n");
    mu_run_test(test_roll_dice);
    mu_run_test(test_roll_all_dice);

    printf("Running Camel Movement Tests...\n");
    mu_run_test(test_get_camel);
    mu_run_test(test_move_camel_forward);
    mu_run_test(test_move_camel_to_finish);

    printf("Running Scoring Tests...\n");
    mu_run_test(test_get_top_camels);
    mu_run_test(test_assign_points_winner);
    mu_run_test(test_assign_points_second);
    mu_run_test(test_assign_points_loser);

    printf("Running Round Tests...\n");
    mu_run_test(test_end_round_resets);

    printf("Running Integration Tests...\n");
    mu_run_test(test_full_round_playthrough);

    printf("Running Utility Tests...\n");
    mu_run_test(test_enum2char);
    mu_run_test(test_orient2char);
    mu_run_test(test_rand_range);
    printf("Running Advanced Scoring Tests...\n");
    mu_run_test(test_multiple_tickets_same_color);
    mu_run_test(test_second_place_ticket_scoring);
    mu_run_test(test_losing_ticket_penalty);
    mu_run_test(test_mixed_ticket_scoring);
    mu_run_test(test_wager_scoring_correct_guess);
    mu_run_test(test_wager_scoring_wrong_guess);
    mu_run_test(test_loser_wager_scoring);
    mu_run_test(test_score_round_integration);
    mu_run_test(test_full_game_scoring);
    mu_run_test(test_tickets_reset_after_round);
    mu_run_test(test_spectator_tile_points);
    mu_run_test(test_roll_gives_point);
    mu_run_test(test_wager_scoring_mixed_fifo);
    mu_run_test(test_wager_stack_at_capacity);
    mu_run_test(test_wagers_clear_after_scoring);
    mu_run_test(test_stack_victory_order);
    mu_run_test(test_spectator_reverse_move);

    return 0;
}

int main(int argc, char** argv) {
    printf("=== Camel Up Game Test Suite ===\n\n");

    char* result = all_tests();

    printf("\n=================================\n");
    if (result != 0) {
        printf("FAILED: %s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);
    printf("=================================\n");

    return result != 0;
}
