#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>
using namespace std;

typedef unsigned char byte;

vector<string> possibles;
vector<string> solutions;
vector<int> starting_solutions;
byte* masks;

byte create_mask(const string &guess, const string &soln) {
  int mask[] = { 0, 0, 0, 0, 0 };
  unordered_map<char, int> letterCounts;

  for (char c : soln) letterCounts[c]++;

  for (int i = 0; i < 5; i++) {
    char c = guess[i];

    if (c == soln[i]) {
      mask[i] = 2;
      letterCounts[c]--;
    }
  }

  for (int i = 0; i < 5; i++) {
    char c = guess[i];
    if (mask[i] == 0 && letterCounts[c] > 0) {
      mask[i] = 1;
      letterCounts[c]--;
    }
  }

  byte ret = 0;
  for (int i = 0; i < 5; i++) {
    ret += mask[i] * pow(3, (4 - i));
  }
  return ret;
}

void print_mask(byte mask) {
  char ret[5];
  for (int i = 4; i >= 0; i--) {
    ret[i] = '0' + (mask % 3);
    mask /= 3;
  }
  cout << ret << endl;
}

int guess_next_word(const vector<int> &remaining_solutions) {
  if (remaining_solutions.size() <= 2) return remaining_solutions[0];
  int min_score = 9999;
  int min_guess = 0;
  for (int guess_idx = 0; guess_idx < possibles.size(); guess_idx++) {
    byte tally[243] = {0};
    int bonus = 0;
    for (auto soln : remaining_solutions) {
      tally[masks[guess_idx * solutions.size() + soln]]++;
      if (soln == guess_idx) bonus = -1;
    }

    int max = *max_element(tally, tally+243) * 2 + bonus;

    if (max < min_score) {
      min_score = max;
      min_guess = guess_idx;
    }
  }
  return min_guess;
}

vector<int> filter_solutions(int guess_idx, byte mask, vector<int>& current_solutions) {
  vector<int> ret;
  for (int soln : current_solutions) {
    if (mask == masks[guess_idx * solutions.size() + soln])
      ret.push_back(soln);
  }
  return ret;
}

void play_game() {
  string guess = "roate";
  vector<int> remaining_solutions;
  while (true) {
    cout << "I think you should guess " << guess << endl;
    cout << "Enter results ('.' for no match, 'X' for yellow, 'O' for green):" << endl;
    string mask_buf;
    cin >> mask_buf;
    byte mask = 0;
    for (int i = 0; i < 5; i++) {
      int m = 0;
      if (mask_buf[i] == 'x')
        m = 1;
      else if (mask_buf[i] == 'o')
        m = 2;
      mask += m * pow(3, (4 - i));
    }

    auto it = find(possibles.begin(), possibles.end(), guess);
    int guess_idx = distance(possibles.begin(), it);

    remaining_solutions = filter_solutions(guess_idx, mask,
        remaining_solutions.empty() ? starting_solutions : remaining_solutions);

    cout << remaining_solutions.size() << " remaining solutions: ";
    vector<string> remaining_solutions_display;
    for (auto guess_idx: remaining_solutions) {
      remaining_solutions_display.push_back(possibles[guess_idx]);
    }
    sort(remaining_solutions_display.begin(), remaining_solutions_display.end());
    for (auto possible: remaining_solutions_display) {
      cout << possible << ", ";
    }
    cout << endl;

    if (remaining_solutions.size() == 1) break;

    guess = possibles[guess_next_word(remaining_solutions)];
  }
}

double benchmark(string starting_word = "roate") {
  auto it = find(possibles.begin(), possibles.end(), starting_word);
  int starting_guess_idx = distance(possibles.begin(), it);
  int turns = 0;

  for (int answer = 0; answer < solutions.size(); answer++) {
    // cout << "Starting run for " << solutions[answer] << endl;
    vector<int> remaining_solutions;
    int guess_idx = starting_guess_idx;
    // string debug = "";

    while (true) {
      // debug += possibles[guess_idx] + " ";
      turns++;
      if (guess_idx == answer) break;

      byte mask = masks[guess_idx * solutions.size() + answer];
      // cout << "Starting size: " << remaining_solutions.size() << endl;
      remaining_solutions = filter_solutions(guess_idx, mask,
        remaining_solutions.empty() ? starting_solutions : remaining_solutions);
      // cout << "Guessed " << possibles[guess_idx] << ", Ending size: " << remaining_solutions.size() << ": ";
      // for (auto i : remaining_solutions) { cout << possibles[i] << " "; }
      // cout << endl;


      guess_idx = guess_next_word(remaining_solutions);
    }
    // cout << debug << endl;
  }
  return (double)turns / solutions.size();
}

int main() {
  ifstream wordlist("word_lists.txt");
  bool hit_break = false;
  for(string line; getline( wordlist, line ); )
  {
    if (line == "") {
      hit_break = true;
      continue;
    }
    possibles.push_back(line);
    if (!hit_break) solutions.push_back(line);
  }
  wordlist.close();

  starting_solutions.resize(solutions.size());
  iota(starting_solutions.begin(), starting_solutions.end(), 0);

  int f = open("masks_array.bin", O_RDONLY);
  if (f != -1) {
    masks = static_cast<byte*>(mmap(
      NULL,
      sizeof(byte) * possibles.size() * solutions.size(),
      PROT_READ, MAP_PRIVATE, f, 0u));
  } else {
    int i = 0;
    masks = new byte[possibles.size() * solutions.size()];
    for (auto guess : possibles) {
      for (auto soln : solutions) {
        masks[i++] = create_mask(guess, soln);
      }
    }

    FILE* f = fopen("masks_array.bin", "wb");
    fwrite(masks, sizeof(byte), possibles.size() * solutions.size(), f);
    fclose(f);
  }

  // play_game();
  cout << benchmark() << endl;
}
