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

// Directly assign id 0 to the board received
int MCTS() {
  while(boards[0].metadata.number < MAX_SIMULATION_COUNT) {
    int current_id = 0;
    while(!boards[current_id].metadata.child_id.empty()) {
      int best_child_id = -1;
      float best_UCB = 0;

      for(int child : boards[current_id].metadata.child_id) {
        float child_UCB = fast_UCB(boards[child].metadata.loss, boards[child].metadata.number, boards[current_id].metadata.number);
        if(best_UCB < child_UCB) {
          best_UCB = child_UCB;
          best_child_id = child;
        }
      }
      current_id = best_child_id;
    }
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

  int best_move = 0;
  float best_win_rate = 0;
  for(int i = 0; i < static_cast<int>(boards[0].metadata.child_id.size()); i++) {
    int child = boards[0].metadata.child_id[i];
    float win_rate = (float) boards[child].metadata.loss / (float) boards[child].metadata.number;
    if(best_win_rate < win_rate) {
      best_win_rate = win_rate;
      best_move = i;
    }
  }
  return best_move;
}

static int g_board_id = 0;

void Simulation(int current_id) {
  boards[current_id].generate_moves();

  int win = 0;
  int loss = 0;
  int number = 0;

  for(int i = 0; i < boards[current_id].move_count; i++) {
    int idx = g_board_id++;
    boards[current_id].metadata.child_id.push_back(idx);
    boards[idx] = boards[current_id];
    boards[idx].metadata.Clear();
    boards[idx].metadata.parent_id = current_id;
    boards[idx].move(i);

    for(int j = 0; j < SIMULATION_BATCH_NUM; j++) {
      if(boards[idx].simulate() == false) {
        ++boards[idx].metadata.win;
      } else {
        ++boards[idx].metadata.loss;
      }
      ++boards[idx].metadata.number;
    }

    win += boards[idx].metadata.win;
    loss += boards[idx].metadata.loss;
    number += boards[idx].metadata.number;
  }

  // backtrack
  do {
    std::swap(win, loss);
    boards[current_id].metadata.win += win;
    boards[current_id].metadata.loss += loss;
    boards[current_id].metadata.number += number;
    current_id = boards[current_id].metadata.parent_id;
  } while(current_id != -1);
}

// Except first move, call decide to decide which move to perform
int Board::decide() {
  generate_moves();

  boards[0] = *this;
  boards[0].metadata.Clear();
  g_board_id = 1;

  return MCTS();
}

// Only used in first move
int Board::first_move_decide_dice() {
  boards[0] = *this;
  boards[0].metadata.Clear();
  g_board_id = 1;
  
  for(int i = 0; i < PIECE_NUM; i++) {
    int idx = g_board_id++;
    boards[0].metadata.child_id.push_back(idx);
    boards[idx] = boards[0];
    boards[idx].dice = i;
    boards[idx].metadata.Clear();
    boards[idx].metadata.parent_id = 0;
  
    for(int j = 0; j < SIMULATION_BATCH_NUM; j++) {
      if(boards[idx].simulate() == false) {
        ++boards[idx].metadata.win;
      } else {
        ++boards[idx].metadata.loss;
      }
      ++boards[idx].metadata.number;
    }
  
    boards[0].metadata.win += boards[idx].metadata.loss;
    boards[0].metadata.loss += boards[idx].metadata.win;
    boards[0].metadata.number += boards[idx].metadata.number;
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
