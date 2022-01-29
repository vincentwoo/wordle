#include <algorithm>
#include <atomic>
#include <boost/container_hash/hash.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <math.h>
#include <mutex>
#include <numeric>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include "util.h"

using namespace std;

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
shared_mutex cache_1_ply_mutex;

GuessScore guess_next_word(const solutions_container &remaining_solutions)
{
  if (remaining_solutions.size() <= 2)
    return { remaining_solutions[0], (int)remaining_solutions.size() - 1 };

  // cache_accesses++;
  cache_1_ply_mutex.lock_shared();
  auto it = cache_1_ply.find(remaining_solutions);
  if (it != cache_1_ply.end()) {
    auto ret = it->second;
    cache_1_ply_mutex.unlock_shared();
    return ret;
  }
  cache_1_ply_mutex.unlock_shared();

  GuessScore best = { 0, 9999 };
  for (int guess_idx = 0; guess_idx < possibles.size(); guess_idx++) {
    short tally[243] = {};
    int bonus = 0;
    //const mask_t* lookup_offset = mask_lookup + guess_idx * solutions.size();
    for (auto const &soln : remaining_solutions) {
      tally[mask_lookup[guess_idx * solutions.size() + soln]]++;
      if (soln == guess_idx) bonus = -1;
    }

    int max = bonus;
    for (auto const &count : tally) {
      max += count * count;
    }

    if (max < best.score) {
      best.score = max;
      best.guess = guess_idx;
    }
  }
  cache_1_ply_mutex.lock();
  cache_1_ply[remaining_solutions] = best;
  cache_1_ply_mutex.unlock();
  return best;
}

unordered_map<
  solutions_container,
  GuessScore,
  container_hash<solutions_container>
> cache_2_ply;
shared_mutex cache_2_ply_mutex;
GuessScore guess_next_word_2_ply(const solutions_container &remaining_solutions)
{
  if (remaining_solutions.size() < 20) return guess_next_word(remaining_solutions);
  cache_2_ply_mutex.lock_shared();
  auto it = cache_2_ply.find(remaining_solutions);
  if (it != cache_2_ply.end()) {
    auto ret = it->second;
    cache_2_ply_mutex.unlock_shared();
    return ret;
  }
  cache_2_ply_mutex.unlock_shared();

  GuessScore best = { 0, 9999 };
  for (int guess_idx = 0; guess_idx < possibles.size(); guess_idx++) {
    solutions_container groups[243];
    for (auto const &soln : remaining_solutions) {
      if (soln != guess_idx)
        groups[mask_lookup[guess_idx * solutions.size() + soln]].push_back(soln);
    }

    GuessScore subScore = { guess_idx, 0 };
    sort(groups, groups+243, [](const solutions_container& a, const solutions_container& b) -> bool {
      return a.size() > b.size();
    });

    if (groups[0].size() > 5 + remaining_solutions.size() / 4) continue;

    for (auto const &group : groups) {
      if (group.empty() || subScore.score >= best.score) break;
      GuessScore leafScore = guess_next_word(group);
      subScore.score += group.size() * leafScore.score;
    }

    if (subScore.score < best.score) best = subScore;
  }

  cache_2_ply_mutex.lock();
  cache_2_ply[remaining_solutions] = best;
  cache_2_ply_mutex.unlock();
  return best;
}

solutions_container filter_solutions(int guess_idx, mask_t mask, const solutions_container& current_solutions) {
  solutions_container ret;
  for (const auto& soln : current_solutions) {
    if (mask == mask_lookup[guess_idx * solutions.size() + soln])
      ret.push_back(soln);
  }
  ret.shrink_to_fit();
  return ret;
}

void play_game() {
  string guess = "crate";
  solutions_container remaining_solutions;
  while (true) {
    cout << "I think you should guess " << guess << endl;
    cout << "Enter results ('.' for no match, 'X' for yellow, 'O' for green):" << endl;
    string mask_buf;
    cin >> mask_buf;
    mask_t mask = 0;
    for (int i = 0; i < 5; i++) {
      int m = 0;
      if (mask_buf[i] == 'x') {
        m = 1;
      } else if (mask_buf[i] == 'o') {
        m = 2;
      }
      mask += m * pow(3, (4 - i));
    }

    auto it = find(possibles.begin(), possibles.end(), guess);
    auto guess_idx = distance(possibles.begin(), it);

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

vector<int> benchmark(string starting_word = "crate") {
  auto it = find(possibles.begin(), possibles.end(), starting_word);
  int starting_guess_idx = distance(possibles.begin(), it);
  vector<int> distribution(7, 0);

  for (int answer = 0; answer < solutions.size(); answer++) {
    //cout << "Starting " << starting_word << ", " << solutions[answer] << endl;
    solutions_container remaining_solutions;
    int guess_idx = starting_guess_idx;
    int depth = 1;

    while (true) {
      if (guess_idx == answer) break;

      if (depth > 5) {
        cout << "Problem with starting word: " << starting_word << endl;
        exit(0);
      }

      const mask_t& mask = mask_lookup[guess_idx * solutions.size() + answer];
      //cout << "Starting size: " << remaining_solutions.size() << endl;
      remaining_solutions = filter_solutions(guess_idx, mask,
        depth == 1 ? starting_solutions : remaining_solutions);
      //cout << "Guessed " << possibles[guess_idx] << " got " << mask_to_str(mask) << ", Ending size: " << remaining_solutions.size() << ": " << endl;
      //for (const auto& i : remaining_solutions) { cout << solutions[i] << " "; }
      //cout << endl;

      guess_idx = guess_next_word_2_ply(remaining_solutions).guess;
      depth++;
    }
    distribution[depth]++;
    //cout << "Guessed " << solutions[answer] << " in " << depth << endl;
  }
  return distribution;
}

mutex queueLock;
queue<int> workQueue;
vector<pair<int, double>> results;
void jobFunc() {
  while (true) {
    int work;
    queueLock.lock();
    if (workQueue.empty()) {
      work = -1;
    } else {
      work = workQueue.front();
      workQueue.pop();
    }
    queueLock.unlock();

    if (work == -1) break;
    auto distribution = benchmark(possibles[work]);
    int total = 0;
    for (int turn = 1; turn <= 6; turn++) {
      total += distribution[turn] * turn;
    }
    double average = (double)total / solutions.size();
    cout << average << ": " << possibles[work] << endl;
    results[work] = {work, average};
  }
}

void runParallelizedBenchmark() {
  int n_guesses = 1000;//possibles.size();
  int n_threads = 2;
  results.resize(n_guesses);
  for (int i = 0; i < n_guesses; i++) workQueue.push(i);
  vector<thread*> threads;
  for (int i = 0; i < n_threads; i++) {
    threads.push_back(new thread(jobFunc));
  }
  for (auto _thread : threads) _thread->join();
  sort(results.begin(), results.end(),
    [](const auto& a, const auto &b) -> bool { return a.second < b.second; });
  cout << "Best starting words:" << endl;
  for (int i = 0; i < 5; i++) {
    cout << "#" << i+1 << " " << possibles[results[i].first] << " with an average of "
      << results[i].second << " guesses" << endl;
  }
  cout << "Worst starting words:" << endl;
  for (int i = 1; i <= 5; i++) {
    cout << "#" << i << " " << possibles[results[results.size() - i].first] << " with an average of "
      << results[results.size() - i].second << " guesses" << endl;
  }
}

int main() {
  init_words();

  auto start = chrono::system_clock::now();
  cout << "Starting run" << endl;

  // runParallelizedBenchmark();
  // play_game();

  auto distribution = benchmark("crate");
  int total = 0;
  for (int turn = 1; turn <= 6; turn++) {
   total += distribution[turn] * turn;
   cout << "Solved in " << turn << " turn: " << distribution[turn] << endl;
  }
  cout << "Total average: " << (double) total / solutions.size() << endl;

  auto end = chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  cout << elapsed_seconds.count() << " elapsed" << endl;

  size_t total_key_length = 0, n = 0, max = 0;
  for (const auto& entry : cache_1_ply) {
    auto size = entry.first.size();
    total_key_length += size;
    n++;
    if (size > max) max = size;
  }
  cout << "1-ply cache stats: " << double(total_key_length) / n << " avg size, " << n << " entries, " << max << " max size." << endl;

  total_key_length = 0, n = 0, max = 0;
  for (const auto& entry : cache_2_ply) {
    auto size = entry.first.size();
    total_key_length += size;
    n++;
    if (size > max) max = size;
  }
  cout << "2-ply cache stats: " << double(total_key_length) / n << " avg size, " << n << " entries, " << max << " max size." << endl;

  // cout << guess_counts.size() << " unique words guessed:" << endl;
  // for (auto entry : guess_counts) {
  //   cout << possibles[entry.first] << ": " << entry.second << endl;
  // }
}
