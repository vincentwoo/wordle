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

int main() {
  ifstream wordlist("word_lists.txt");
  vector<string> possibles;
  vector<string> solutions;
  auto cur = &solutions;
  for(string line; getline( wordlist, line ); )
  {
    if (line == "") cur = &possibles;
    cur->push_back(line);
  }

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
    cout << sizeof(short) << " " << sizeof(masks) << endl;
    fwrite(masks, sizeof(short), possibles.size() * solutions.size(), f);
    fclose(f);
  }
}
