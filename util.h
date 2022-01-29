#ifndef WORDLE_UTIL_H
#define WORDLE_UTIL_H

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
#else
  #include <sys/mman.h>
#endif // _WIN32

using namespace std;

typedef unsigned char mask_t;
typedef vector<int> solutions_container;

vector<string> possibles;
vector<string> solutions;
solutions_container starting_solutions;
mask_t* mask_lookup;

mask_t create_mask(const string& guess, const string& soln) {
  int responses[] = { 0, 0, 0, 0, 0 };
  unordered_map<char, int> letterCounts;

  for (char c : soln) letterCounts[c]++;

  for (int i = 0; i < 5; i++) {
    char c = guess[i];

    if (c == soln[i]) {
      responses[i] = 2;
      letterCounts[c]--;
    }
  }

  for (int i = 0; i < 5; i++) {
    char c = guess[i];
    if (responses[i] == 0 && letterCounts[c] > 0) {
      responses[i] = 1;
      letterCounts[c]--;
    }
  }

  mask_t mask = 0;
  for (int i = 0; i < 5; i++) {
    mask += responses[i] * (mask_t)pow(3, (4 - i));
  }
  return mask;
}

void init_words() {
  ifstream wordlist("ranked_wordlist.txt");
  bool hit_break = false;
  for (string line; getline(wordlist, line); )
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

#ifdef _WIN32
  HANDLE f = CreateFileA("mask_lookup_array.bin", GENERIC_READ, 0, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f != INVALID_HANDLE_VALUE) {
    auto fileMap = CreateFileMappingA(f, NULL, PAGE_READONLY, 0, 0, NULL);
    mask_lookup = (mask_t*)MapViewOfFile(fileMap, FILE_MAP_READ, 0, 0,
      sizeof(mask_t) * possibles.size() * solutions.size());
#else
  int f = open("mask_lookup_array.bin", O_RDONLY);
  if (f != -1) {
    mask_lookup = static_cast<mask_t*>(mmap(
      NULL,
      sizeof(mask_t) * possibles.size() * solutions.size(),
      PROT_READ, MAP_PRIVATE, f, 0u));
#endif // _WIN32
  }
  else {
    cout << "Regenerating lookup tables" << endl;
    int i = 0;
    mask_lookup = new mask_t[possibles.size() * solutions.size()];
    for (const auto& guess : possibles) {
      for (const auto& soln : solutions) {
        mask_lookup[i++] = create_mask(guess, soln);
      }
    }

    auto f_out = fopen("mask_lookup_array.bin", "wb");
    fwrite(mask_lookup, sizeof(mask_t), possibles.size() * solutions.size(), f_out);
    fclose(f_out);
  }
}

string mask_to_str(mask_t mask) {
  string ret(5, '0');
  for (int i = 4; i >= 0; i--) {
    ret[i] = '0' + (mask % 3);
    mask /= 3;
  }
  return ret;
}

#endif