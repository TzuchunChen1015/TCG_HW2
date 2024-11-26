// These are not basic function for board.
// write them in separate file makes code cleaner.
// I recommend write all your complicated algorithms in this file.
// Or sort them into even more files to make code even cleaner!

#include "board/board.hpp"
#include "math_lib/maths.hpp"
#include <stdio.h>
#include <random>
#define SIMULATION_BATCH_NUM 100
#define MAX_SIMULATION_COUNT 2000000

// Pre-accounce enough boards memory
constexpr int MAX_SIZE = 5e5;
static Board boards[MAX_SIZE + 1];

void Simulation(int current_id);

namespace helper {

int GetNumberOfBit(int bits) {
  int count = 0;
  while(bits) {
    if(bits & 1) {
      ++count;
    }
    bits >>= 1;
  }
  return count;
}

}

// Directly assign id 0 to the board received
int MCTS() {
  while(boards[0].metadata.number < MAX_SIMULATION_COUNT) {
    /* Selection */
    int current_id = 0;
    while(boards[current_id].metadata.first_child_id != -1) {
      /* Pruning */
      int first_child_id = boards[current_id].metadata.first_child_id;
      long long int bit = 1;

      int max_opponent_minus_mine = -6;
      for(int i = 0; i < boards[current_id].move_count; i++) {
        if((boards[current_id].metadata.child_bit & bit)) {
          int child_id = first_child_id + i;
          int opponent = helper::GetNumberOfBit(boards[child_id].piece_bits[(1 ^ boards[current_id].moving_color)]);
          int mine = helper::GetNumberOfBit(boards[child_id].piece_bits[boards[current_id].moving_color]);
          if(max_opponent_minus_mine < opponent - mine) {
            max_opponent_minus_mine = opponent - mine;
          }
        }
        bit <<= 1;
      }

      bit = 1;
      int best_child_id = -1;
      float best_UCB = 0;
      for(int i = 0; i < boards[current_id].move_count; i++) {
        if((boards[current_id].metadata.child_bit & bit)) {
          int child_id = first_child_id + i;
          int opponent = helper::GetNumberOfBit(boards[child_id].piece_bits[(1 ^ boards[current_id].moving_color)]);
          int mine = helper::GetNumberOfBit(boards[child_id].piece_bits[boards[current_id].moving_color]);
          if(max_opponent_minus_mine > opponent - mine) {
            boards[current_id].metadata.child_bit &= (~bit);
          } else {
            float child_UCB = fast_UCB(boards[child_id].metadata.loss, boards[child_id].metadata.number, boards[current_id].metadata.number);
            if(best_UCB < child_UCB) {
              best_UCB = child_UCB;
              best_child_id = child_id;
            }
          }
        }
        bit <<= 1;
      }
      current_id = best_child_id;
      /* Pruning */
    }
    /* Selection */
    if(boards[current_id].check_winner()) {
      int win = 0;
      int loss = SIMULATION_BATCH_NUM;
      int number = SIMULATION_BATCH_NUM;
      do {
        boards[current_id].metadata.win += win;
        boards[current_id].metadata.loss += loss;
        boards[current_id].metadata.number += number;
        std::swap(win, loss);
        current_id = boards[current_id].metadata.parent_id;
      } while(current_id != -1);
    } else {
      Simulation(current_id);
    }
  }

  long long int bit = 1;
  int best_move = 0;
  float best_win_rate = 0;
  for(int i = 0, child_id = boards[0].metadata.first_child_id; i < boards[0].move_count; i++, child_id++) {
    if((boards[0].metadata.child_bit & bit)) {
      float win_rate = (float) boards[child_id].metadata.loss / (float) boards[child_id].metadata.number;
      if(best_win_rate < win_rate) {
        best_win_rate = win_rate;
        best_move = i;
      }
    }
    bit <<= 1;
  }
  return best_move;
}

static int g_board_id = 0;

void Simulation(int current_id) {
  /* Expansion */
  boards[current_id].generate_moves();
  boards[current_id].metadata.first_child_id = g_board_id;
  boards[current_id].metadata.child_bit = (1ll << (boards[current_id].move_count)) - 1;
  g_board_id += boards[current_id].move_count;
  /* Expansion */

  int win = 0;
  int loss = 0;
  int number = 0;

  /* Simulation */
  for(int i = 0, child_id = boards[current_id].metadata.first_child_id; i < boards[current_id].move_count; i++, child_id++) {
    boards[child_id] = boards[current_id];
    boards[child_id].metadata.parent_id = current_id;
    boards[child_id].move(i);

    for(int j = 0; j < SIMULATION_BATCH_NUM; j++) {
      if(boards[child_id].simulate() == false) {
        ++boards[child_id].metadata.win;
      } else {
        ++boards[child_id].metadata.loss;
      }
    }
    boards[child_id].metadata.number = SIMULATION_BATCH_NUM;

    win += boards[child_id].metadata.win;
    loss += boards[child_id].metadata.loss;
    number += boards[child_id].metadata.number;
  }
  /* Simulation */

  /* Back Propagation */
  do {
    std::swap(win, loss);
    boards[current_id].metadata.win += win;
    boards[current_id].metadata.loss += loss;
    boards[current_id].metadata.number += number;
    current_id = boards[current_id].metadata.parent_id;
  } while(current_id != -1);
  /* Back Propagation */
}

// Except first move, call decide to decide which move to perform
int Board::decide() {
  generate_moves();

  boards[0] = *this;
  g_board_id = 1;

  return MCTS();
}

// Only used in first move
int Board::first_move_decide_dice() {
  boards[0] = *this;
  boards[0].metadata.first_child_id = 1;
  boards[0].move_count = PIECE_NUM;
  boards[0].metadata.child_bit = (1 << (PIECE_NUM)) - 1;

  g_board_id = PIECE_NUM + 1;
  
  for(int child_id = 1; child_id <= PIECE_NUM; child_id++) {
    boards[child_id] = boards[0];
    boards[child_id].dice = child_id - 1;
    boards[child_id].metadata.parent_id = 0;
  
    for(int i = 0; i < SIMULATION_BATCH_NUM; i++) {
      if(boards[child_id].simulate() == false) {
        ++boards[child_id].metadata.win;
      } else {
        ++boards[child_id].metadata.loss;
      }
    }
    boards[child_id].metadata.number = SIMULATION_BATCH_NUM;
  
    boards[0].metadata.win += boards[child_id].metadata.loss;
    boards[0].metadata.loss += boards[child_id].metadata.win;
    boards[0].metadata.number += boards[child_id].metadata.number;
  }

  return MCTS();
}
// You should use mersenne twister and a random_device seed for the simulation
// But no worries, I've done it for you. Hope it can save you some time!
// Call random_num()%num to randomly pick number from 0 to num-1
std::mt19937 random_num(std::random_device{}());

// Very fast and clean simulation!
bool Board::simulate()
{
    Board board_copy = *(this);
    // run until game ends.
    while (!board_copy.check_winner())
    {
        board_copy.generate_moves();
        board_copy.move(random_num() % board_copy.move_count);
    }
    // game ends! find winner.
    // Win!
    if (board_copy.moving_color == moving_color)
        return true;
    // Lose!
    else
        return false;
}
