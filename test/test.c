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
    mu_assert("Should have RED",     player.hand[0].color == BRED);
    mu_assert("Should have BLUE",    player.hand[1].color == BBLUE);
    mu_assert("Should have YELLOW" , player.hand[2].color == BYELLOW);
    mu_assert("Should have GREEN",   player.hand[3].color == BGREEN);
    mu_assert("Should have PURPLE",  player.hand[4].color == BPURPLE);

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
