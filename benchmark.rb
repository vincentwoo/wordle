$stdout.sync = true

require 'stackprof'
StackProf.run(mode: :cpu, raw: true, out: 'perf2.dump') do
  lists = File.read('word_lists.txt').split("\n\n")
  solutions, possible = lists.map { |s| s.split("\n").map(&:chars) }
  possible += solutions

  puzzles = File.read('benchmark.txt').split("\n")
  puzzles = puzzles.first(ARGV[0].to_i) if ARGV[0]

  def remaining_solutions solutions, guess, mask
    i = 4
    while i >= 0
      m = mask & 3
      solutions = case m
      when 0
        solutions.reject { |soln| soln.include? guess[i] }
      when 1
        solutions.select { |soln| soln.include?(guess[i]) && soln[i] != guess[i] }
      when 2
        solutions.select { |soln| soln[i] == guess[i] }
      end
      mask >>= 2
      i -= 1
    end
    solutions
  end

  def check_guess guess, soln
    i = 0
    ret = 0
    while i < 5
      ret <<= 2
      if soln[i] == guess[i]
        ret += 2
      elsif soln.include?(guess[i])
        ret += 1
      end
      i += 1
    end
    ret
  end

  total_cost = puzzles.sum do |puzzle|
    _solutions = solutions
    guesses = ['roate']
    while true
      break if guesses.last == puzzle
      guess = guesses.last.chars

      mask = check_guess(guess, puzzle)
      _solutions = remaining_solutions _solutions, guess, mask

      guess = possible.min_by { |guess|
        memo = {}
        score = _solutions.sum do |solution|
          mask = check_guess(guess, solution)
          memo[mask] ||= remaining_solutions(_solutions, guess, mask).count
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
end
