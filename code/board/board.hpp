#ifndef __BOARD__
#define __BOARD__ 1

#define RED 0
#define BLUE 1

#define PIECE_NUM 6

struct MetaData {
  int parent_id = -1;
  int child_from, child_to;

  int win;    // number of wins for the board
  int loss;   // number of losses for the board
  int number; // number of game

  void operator= (const MetaData& rhs) {
    child_from = child_to = -1;
    win = loss = number = 0;
  }
};

typedef struct _board
{
    MetaData metadata;

    // all captured: piece_bits becomes 0
    unsigned char piece_bits[2];
    int piece_position[2][PIECE_NUM];
    // blank is -1
    int board[25];
    char moves[PIECE_NUM][2];
    int move_count;
    char moving_color;
    char dice;
    void init_with_piecepos(int input_piecepos[2][6], char input_color);
    void move(int id_with_dice);
    void generate_moves();
    bool check_winner();
    void print_board();

    // not basic functions, written in decide.cpp
    bool simulate();
    int decide();
    int first_move_decide_dice();
} Board;
#endif
