$stdout.sync = true

lists = File.read('word_lists.txt').split("\n\n")
solutions, possible = lists.map { |s| s.split("\n").map(&:chars) }
possible += solutions

puzzles = File.read('benchmark.txt').split("\n")
puzzles = puzzles.first(ARGV[0].to_i) if ARGV[0]

def remaining_solutions solutions, rejected, present, found
  solutions.select { |soln|
    (soln & rejected).length == 0 &&
    found.all? { |c, i| soln[i] == c } &&
    present.all? { |c, i| soln[i] != c && soln.include?(c) }
  }
end

def check_guess guess, soln
  rejected = []
  present = []
  found = []
  guess.each.with_index do |c, i|
    if soln[i] == c
      found.push [c, i]
    elsif soln.include?(c)
      present.push [c, i]
    else
      rejected.push c
    end
  end
  [rejected.uniq.sort, present, found]
end

total_cost = puzzles.sum do |puzzle|
  _solutions = solutions
  guesses = ['roate']
  while true
    break if guesses.last == puzzle

    _solutions = remaining_solutions _solutions, *check_guess(guesses.last.chars, puzzle)

    memo = {}
    guess = possible.min_by { |guess|
      score = _solutions.sum do |solution|
        mask = check_guess(guess, solution)
        memo[mask] ||= remaining_solutions(_solutions, *mask).count
      end
      score -= 1 if _solutions.include?(guess)
      score
    }.join
    guesses.push guess
  end
  puts "Solved #{puzzle.upcase} in #{guesses.length}: #{guesses.map(&:upcase).join(', ')}"
  guesses.length
end

puts "Solved all #{puzzles.length} puzzles in #{total_cost} guesses, avg #{total_cost.to_f / puzzles.length} per"
