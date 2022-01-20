$stdout.sync = true

# require 'stackprof'
# StackProf.run(mode: :cpu, raw: true, out: 'perf2.dump') do
  $solutions = File.read('word_lists.txt').split("\n\n").first.split("\n").map(&:freeze)
  $possible = eval(File.read('wordle_first_guess.txt')).map(&:first).map(&:freeze)

  puzzles = File.read('benchmark.txt').split("\n")
  puzzles = puzzles.first(ARGV[0].to_i) if ARGV[0]

  File.open('masks.bin', 'rb') { |f| $all_masks = Marshal.load f }

  def groups_for_guess solutions, guess
    solutions.group_by { |soln| $all_masks[guess][soln] }
  end

  $memo = {}
  def best_guess solutions = (0...$solutions.length), depth = 1
    return [0, solutions.first] if solutions.size == 1

    if depth == 0
      return $memo[solutions] ||= (0...$possible.length).map { |guess|
        [entropy(solutions, guess), guess]
      }.min
    end

    (0...$possible.length).map { |guess|
      p guess if guess % 100 == 0
      groups_for_guess(solutions, guess)
        .sort_by { |mask, group| [-group.length, mask] }
        .first(1)
        .map { |mask, group| best_guess group, depth - 1 }
        .max
    }.min
  end


  def entropy solutions, guess
    scores = $all_masks[guess].values_at(*solutions).tally.values
    scores.max + (scores.sum.to_f / scores.length) / 10
    # scores.sum { |count|
    #   p = count.to_f / scores.length
    #   p * Math.log2(p)
    # }
    # scores.sum.to_f / scores.length
  end

  def create_mask guess, soln
    letterCounts = soln.chars.tally
    letterCounts.default = 0

    mask = [0, 0, 0, 0, 0]
    guess.chars.each.with_index do |c, i|
      if c == soln[i]
        mask[i] = 2
        letterCounts[c] -= 1
      end
    end
    guess.chars.each.with_index do |c, i|
      if mask[i] == 0 && letterCounts[c] >= 1
        mask[i] = 1
        letterCounts[c] -= 1
      end
    end

    (2.downto(0).map { |i| mask.count(i) } + mask).join.to_i
  end

  total_cost = puzzles.sum do |puzzle|
    solutions = (0...$solutions.length)
    guesses = ['roate']
    while true
      p guesses
      break if guesses.last == puzzle
      mask = create_mask guesses.last, puzzle

      guess = $possible.index guesses.last
      solutions = solutions.select { |soln| $all_masks[guess][soln] == mask }
      raise
      p solutions.length

      guess = $possible[best_guess(solutions).last]
      guesses.push guess
    end
    puts "Solved #{puzzle.upcase} in #{guesses.length}: #{guesses.map(&:upcase).join(', ')}"
    guesses.length
  end

  puts "Solved all #{puzzles.length} puzzles in #{total_cost} guesses, avg #{total_cost.to_f / puzzles.length} per"
# end
