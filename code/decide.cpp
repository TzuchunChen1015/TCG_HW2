// These are not basic function for board.
// write them in separate file makes code cleaner.
// I recommend write all your complicated algorithms in this file.
// Or sort them into even more files to make code even cleaner!

#include "board/board.hpp"
#include "math_lib/maths.hpp"
#include <stdio.h>
#include <random>
#define SIMULATION_BATCH_NUM 100
#define MAX_SIMULATION_COUNT 3500000

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
constexpr int MAX_SIZE_OPPONENT_MINUS_MINE = 15;
float best_UCB_array[MAX_SIZE_OPPONENT_MINUS_MINE];

int PruningWithOpponentMinusMine(int current_id);
int PruningUCBWithBestLCB(int current_id);
int ProgressivePruning(int current_id);

int MCTS() {
  while(boards[0].metadata.number < MAX_SIMULATION_COUNT) {
    /* Selection */
    int current_id = 0;
    while(boards[current_id].metadata.first_child_id != -1) {
      /* Pruning */
      current_id = PruningWithOpponentMinusMine(current_id);
      // current_id = PruningUCBWithBestLCB(current_id);
      // current_id = ProgressivePruning(current_id);
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
    boards[child_id].moving_color ^= 1;
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

/* Pruning Algorithm */
int PruningWithOpponentMinusMine(int current_id) {
  for(int i = 0; i < MAX_SIZE_OPPONENT_MINUS_MINE; i++) {
    best_UCB_array[i] = 0;
  }
  
  int first_child_id = boards[current_id].metadata.first_child_id;
  long long int bit = 1;

  for(int i = 0; i < boards[current_id].move_count; i++) {
    if((boards[current_id].metadata.child_bit & bit)) {
      int child_id = first_child_id + i;
      int opponent = helper::GetNumberOfBit(boards[child_id].piece_bits[(1 ^ boards[current_id].moving_color)]);
      int mine = helper::GetNumberOfBit(boards[child_id].piece_bits[boards[current_id].moving_color]);
      float child_UCB = fast_UCB(boards[child_id].metadata.loss, boards[child_id].metadata.number, boards[current_id].metadata.number);
      if(best_UCB_array[opponent - mine + 6] < child_UCB) {
        best_UCB_array[opponent - mine + 6] = child_UCB;
      }
    }
    bit <<= 1;
  }

  for(int i = MAX_SIZE_OPPONENT_MINUS_MINE - 2; i >= 0; i--) {
    if(best_UCB_array[i] < best_UCB_array[i + 1]) {
      best_UCB_array[i] = best_UCB_array[i + 1];
    }
  }

  bit = 1;
  int best_child_id = -1;
  float best_UCB = 0;
  for(int i = 0; i < boards[current_id].move_count; i++) {
    if((boards[current_id].metadata.child_bit & bit)) {
      int child_id = first_child_id + i;
      int opponent = helper::GetNumberOfBit(boards[child_id].piece_bits[(1 ^ boards[current_id].moving_color)]);
      int mine = helper::GetNumberOfBit(boards[child_id].piece_bits[boards[current_id].moving_color]);
      float child_UCB = fast_UCB(boards[child_id].metadata.loss, boards[child_id].metadata.number, boards[current_id].metadata.number);
      if(best_UCB_array[opponent - mine + 6 + 1] > child_UCB) { // if others' opponent_minus_mine and UCB are both
        boards[current_id].metadata.child_bit &= (~bit);        // greater than yours, than you should be pruned.
      } else {
        if(best_UCB < child_UCB) {
          best_UCB = child_UCB;
          best_child_id = child_id;
        }
      }
    }
    bit <<= 1;
  }
  return best_child_id;
}
int PruningUCBWithBestLCB(int current_id) {
  long long int bit = 1;
  float best_LCB = 0;
  for(int i = 0, child_id = boards[current_id].metadata.first_child_id; i < boards[current_id].move_count; i++, child_id++) {
    if((boards[current_id].metadata.child_bit & bit)) {
      float child_LCB = fast_LCB(boards[child_id].metadata.loss, boards[child_id].metadata.number, boards[current_id].metadata.number);
      if(best_LCB < child_LCB) {
        best_LCB = child_LCB;
      }
    }
    bit <<= 1;
  }

  bit = 1;
  int best_child_id = -1;
  float best_UCB = 0;
  for(int i = 0, child_id = boards[current_id].metadata.first_child_id; i < boards[current_id].move_count; i++, child_id++) {
    if((boards[current_id].metadata.child_bit & bit)) {
      float child_UCB = fast_UCB(boards[child_id].metadata.loss, boards[child_id].metadata.number, boards[current_id].metadata.number);
      if(child_UCB < best_LCB) {
        boards[current_id].metadata.child_bit &= (~bit);
      } else if(best_UCB < child_UCB) {
        best_UCB = child_UCB;
        best_child_id = child_id;
      }
    }
    bit <<= 1;
  }
  return best_child_id;
}

int ProgressivePruning(int current_id) {
  static constexpr float rd = 3;

  long long int bit = 1;
  float best_mu_l = 0;
  for(int i = 0, child_id = boards[current_id].metadata.first_child_id; i < boards[current_id].move_count; i++, child_id++) {
    if((boards[current_id].metadata.child_bit & bit)) {
      float mean = (float) boards[child_id].metadata.loss / (float) boards[child_id].metadata.number;
      float variance = (float) boards[child_id].metadata.loss * (1 - mean) * (1 - mean) + (float) boards[child_id].metadata.win * mean * mean;
      float child_mu_l = mean - rd * std::sqrt(variance);
      if(best_mu_l < child_mu_l) {
        best_mu_l = child_mu_l;
      }
    }
    bit <<= 1;
  }

  bit = 1;
  int best_child_id = -1;
  float best_UCB = 0;
  for(int i = 0, child_id = boards[current_id].metadata.first_child_id; i < boards[current_id].move_count; i++, child_id++) {
    if((boards[current_id].metadata.child_bit & bit)) {
      float mean = (float) boards[child_id].metadata.loss / (float) boards[child_id].metadata.number;
      float variance = (float) boards[child_id].metadata.loss * (1 - mean) * (1 - mean) + (float) boards[child_id].metadata.win * mean * mean;
      float child_mu_r = mean + rd * std::sqrt(variance);
      if(best_mu_l > child_mu_r) {
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
  return best_child_id;
}
