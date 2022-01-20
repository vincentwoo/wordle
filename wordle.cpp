#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

short create_mask(const string &guess, const string &soln) {
  short mask[] = { 0, 0, 0, 0, 0 };
  unordered_map<char, short> letterCounts;

  for (char c : soln) letterCounts[c]++;

  for (unsigned short i = 0; i < guess.length(); i++) {
    char c = guess[i];

    if (c == soln[i]) {
      mask[i] = 2;
      letterCounts[c]--;
    }
  }

  for (unsigned short i = 0; i < guess.length(); i++) {
    char c = guess[i];
    if (mask[i] == 0 && letterCounts[c] > 0) {
      mask[i] = 1;
      letterCounts[c]--;
    }
  }

  short ret = 0;
  for (unsigned short i = 0; i < guess.length(); i++) {
    ret += (mask[i] << 2 * (4 - i));
  }
  return ret;
}

void print_mask(short mask) {
  for (short i = 4; i >= 0; i--) {
    cout << ((mask & (3 << (2 * i))) >> (2 * i));
  }
  cout << endl;
}

void play_game(
  const vector<string> &solutions,
  const vector<string> &possibles,
  const short* masks) {

  string guess = "tolar";
  vector<int> *remaining_solutions = new vector<int>();
  while (true) {
    cout << "I think you should guess " << guess << endl;
    cout << "Enter results ('.' for no match, 'X' for yellow, 'O' for green):" << endl;
    string mask_buf;
    cin >> mask_buf;
    short mask = 0;
    for (int i = 0; i < 5; i++) {
      short m = 0;
      if (mask_buf[i] == 'x')
        m = 1;
      else if (mask_buf[i] == 'o')
        m = 2;
      mask += (m << 2 * (4 - i));
    }

    auto it = find(possibles.begin(), possibles.end(), guess);
    int guess_idx = distance(possibles.begin(), it);

    if (remaining_solutions->empty()) {
      for (int i = 0; i < solutions.size(); i++) {
        if (mask == masks[guess_idx * solutions.size() + i])
          remaining_solutions->push_back(i);
      }
    } else {
      vector<int> *next_remaining_solutions = new vector<int>();
      for (int soln : *remaining_solutions) {
        if (mask == masks[guess_idx * solutions.size() + soln])
          next_remaining_solutions->push_back(soln);
      }
      delete remaining_solutions;
      remaining_solutions = next_remaining_solutions;
    }

    cout << remaining_solutions->size() << " remaining solutions: ";
    vector<string> remaining_solutions_display;
    for (auto guess_idx: *remaining_solutions) {
      remaining_solutions_display.push_back(possibles[guess_idx]);
    }
    sort(remaining_solutions_display.begin(), remaining_solutions_display.end());
    for (auto possible: remaining_solutions_display) {
      cout << possible << ", ";
    }
    cout << endl;

    if (remaining_solutions->size() == 1) break;

    int min_score = 9999;
    int min_guess = 0;
    for (int guess_idx = 0; guess_idx < possibles.size(); guess_idx++) {
      unordered_map<short, int> tally;
      for (auto soln : *remaining_solutions) {
        tally[masks[guess_idx * solutions.size() + soln]]++;
      }
      auto max = max_element(
        tally.begin(),
        tally.end(),
        [] (const auto& a, const auto& b) -> bool { return a.second < b.second; }
      );
      int bonus =
        std::find(remaining_solutions->begin(), remaining_solutions->end(), guess_idx) != remaining_solutions->end() ?
          -1 : 0;
      if (max->second + bonus < min_score) {
        min_score = max->second;
        min_guess = guess_idx;
      }
    }

    guess = possibles[min_guess];
  }
}

int main() {
  ifstream wordlist("word_lists.txt");
  vector<string> possibles;
  vector<string> solutions;
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

  short* masks = new short[possibles.size() * solutions.size()];

  if (FILE *f = fopen("masks_array.bin", "rb")) {
    fread(masks, sizeof(short), possibles.size() * solutions.size(), f);
    fclose(f);
  } else {
    int i = 0;
    for (auto guess : possibles) {
      for (auto soln : solutions) {
        masks[i++] = create_mask(guess, soln);
      }
    }

    f = fopen("masks_array.bin", "wb");
    fwrite(masks, sizeof(short), possibles.size() * solutions.size(), f);
    fclose(f);
  }

  play_game(solutions, possibles, masks);
}
