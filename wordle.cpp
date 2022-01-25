#include <boost/container_hash/hash.hpp>
#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
  #include <Windows.h>
#else
  #include <sys/mman.h>
#endif // _WIN32

using namespace std;

typedef unsigned char byte;
typedef vector<int> solutions_container;

vector<string> possibles;
vector<string> solutions;
solutions_container starting_solutions;
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

string mask_to_str(byte mask) {
  string ret(5, '0');
  for (int i = 4; i >= 0; i--) {
    ret[i] = '0' + (mask % 3);
    mask /= 3;
  }
  return ret;
}

template <typename Container>
struct container_hash {
  size_t operator() (Container const& c) const {
    return boost::hash_range(c.begin(), c.end());
  }
};

struct GuessScore {
  int guess;
  int score;
};

unordered_map<
  solutions_container,
  GuessScore,
  container_hash<solutions_container>
> cache_1_ply;
// int cache_accesses = 0, cache_hits = 0;

GuessScore guess_next_word(const solutions_container &remaining_solutions) {
  if (remaining_solutions.size() <= 2)
    return { remaining_solutions[0], (int)remaining_solutions.size() - 1 };

  // cache_accesses++;
  auto it = cache_1_ply.find(remaining_solutions);
  if (it != cache_1_ply.end()) {
    // cache_hits++;
    return it->second;
  }

  GuessScore best = { 0, 9999 };
  for (int guess_idx = 0; guess_idx < possibles.size(); guess_idx++) {
    short tally[243] = {};
    int bonus = 0;
    for (auto const &soln : remaining_solutions) {
      tally[masks[guess_idx * solutions.size() + soln]]++;
      if (soln == guess_idx) bonus = -1;
    }

    // int max = 0;
    // for (auto const &count : tally) {
    //   if (count > max) max = count;
    // }
    // max = max * 2 + bonus;

    int max = bonus;
    for (auto const &count : tally) {
      max += count * count;
    }
    // max = max * 2 + bonus;

    if (max < best.score) {
      best.score = max;
      best.guess = guess_idx;
    }
  }
  cache_1_ply[remaining_solutions] = best;
  return best;
}

GuessScore guess_next_word_2_ply(const solutions_container &remaining_solutions) {
  if (remaining_solutions.size() < 20) return guess_next_word(remaining_solutions);

  GuessScore best = { 0, 9999 };
  for (int guess_idx = 0; guess_idx < possibles.size(); guess_idx++) {
    solutions_container groups[243];
    for (auto const &soln : remaining_solutions) {
      if (soln != guess_idx)
        groups[masks[guess_idx * solutions.size() + soln]].push_back(soln);
    }

    GuessScore subScore = { guess_idx, 0 };
    // sort(groups, groups+243, [](const solutions_container& a, const solutions_container& b) -> bool {
    //   return a.size() > b.size();
    // });

    for (auto const &group : groups) {
    // for (int i = 0; i < 5; i++) {
      // const auto& group = groups[i];
      if (group.empty()) continue;
      GuessScore leafScore = guess_next_word(group);
      // if (leafScore.score > subScore.score) subScore.score = leafScore.score;
      subScore.score += group.size() * leafScore.score;
      if (subScore.score >= best.score) break;
    }

    if (subScore.score < best.score) best = subScore;
  }
  return best;
}

solutions_container filter_solutions(int guess_idx, byte mask, solutions_container& current_solutions) {
  solutions_container ret;
  for (int soln : current_solutions) {
    if (mask == masks[guess_idx * solutions.size() + soln])
      ret.push_back(soln);
  }
  return ret;
}

void play_game() {
  string guess = "trace";
  solutions_container remaining_solutions;
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

    guess = possibles[guess_next_word_2_ply(remaining_solutions).guess];
  }
}

// unordered_map<int, int> guess_counts;

vector<int> benchmark(string starting_word = "trace") {
  auto it = find(possibles.begin(), possibles.end(), starting_word);
  int starting_guess_idx = distance(possibles.begin(), it);
  vector<int> distribution(7, 0);

  short second_guess_cache[243];
  fill(second_guess_cache, second_guess_cache+243, -1);

  for (int answer = 0; answer < 10; answer++) {
    cout << "Starting run for " << solutions[answer] << endl;
    solutions_container remaining_solutions;
    int guess_idx = starting_guess_idx;
    int depth = 1;

    while (true) {
      // guess_counts[guess_idx]++;
      if (guess_idx == answer) break;

      byte mask = masks[guess_idx * solutions.size() + answer];
      // cout << "Starting size: " << remaining_solutions.size() << endl;
      remaining_solutions = filter_solutions(guess_idx, mask,
        depth == 1 ? starting_solutions : remaining_solutions);
      cout << "Guessed " << possibles[guess_idx] << " got " << mask_to_str(mask) << ", Ending size: " << remaining_solutions.size() << ": " << endl;
      // for (auto i : remaining_solutions) { cout << solutions[i] << " "; }
      // cout << endl;

      if (depth == 1 && second_guess_cache[mask] != -1) {
        guess_idx = second_guess_cache[mask];
      } else {
        guess_idx = guess_next_word_2_ply(remaining_solutions).guess;
        if (depth == 1) second_guess_cache[mask] = guess_idx;
      }

      depth++;
    }
    distribution[depth]++;
    // cout << "Guessed " << solutions[answer] << " in " << depth << endl;
  }
  return distribution;
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

#ifdef _WIN32
  HANDLE f = CreateFileA("masks_array.bin", GENERIC_READ, 0, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f != INVALID_HANDLE_VALUE) {
    auto fileMap = CreateFileMappingA(f, NULL, PAGE_READONLY, 0, 0, NULL);

    masks = (byte *) MapViewOfFile(fileMap, FILE_MAP_READ, 0, 0,
      sizeof(byte) * possibles.size() * solutions.size());
#else
  int f = open("masks_array.bin", O_RDONLY);
  if (f != -1) {
    masks = static_cast<byte*>(mmap(
      NULL,
      sizeof(byte) * possibles.size() * solutions.size(),
      PROT_READ, MAP_PRIVATE, f, 0u));
#endif // _WIN32
  } else {
    int i = 0;
    masks = new byte[possibles.size() * solutions.size()];
    for (const auto& guess : possibles) {
      for (const auto& soln : solutions) {
        masks[i++] = create_mask(guess, soln);
      }
    }

    auto f_out = fopen("masks_array.bin", "wb");
    fwrite(masks, sizeof(byte), possibles.size() * solutions.size(), f_out);
    fclose(f_out);
  }

  // ifstream guessfile("ranked_first_guesses.txt");
  // int i = 0;
  // for(string guess; getline( guessfile, guess ) && i < 20; i++)
  // {
  //   auto distribution = benchmark(guess);
  //   int total = 0;
  //   for (int turn = 1; turn <= 6; turn++) {
  //     total += distribution[turn] * turn;
  //   }
  //   cout << (double) total / solutions.size() << ": " << guess << endl;
  //   // cout << cache_1_ply.size() << " cache entries " << double(cache_hits) / cache_accesses << " hit rate" << endl;
  // }
  // guessfile.close();

  // play_game();

  auto distribution = benchmark();
  int total = 0;
  for (int turn = 1; turn <= 6; turn++) {
    total += distribution[turn] * turn;
    cout << "Solved in " << turn << " turn: " << distribution[turn] << endl;
  }
  cout << "Total average: " << (double) total / solutions.size() << endl;


  // cout << guess_counts.size() << " unique words guessed:" << endl;
  // for (auto entry : guess_counts) {
  //   cout << possibles[entry.first] << ": " << entry.second << endl;
  // }
}
