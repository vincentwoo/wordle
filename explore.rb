require 'yaml'
$stdout.sync = true

# require 'stackprof'
# StackProf.run(mode: :cpu, raw: true, out: 'perf3.dump') do

solutions = File.read('word_lists.txt').split("\n\n").first.split("\n").map(&:freeze)
possible = eval(File.read('wordle_first_guess.txt')).map(&:first).map(&:freeze)

solution_to_index = solutions.map.with_index.to_h
possible_to_index = possible.map.with_index.to_h

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

all_masks = possible.map { |guess| solutions.map { |soln| check_guess guess, soln }}

best_height = solutions.length

results = (0..9).map { |guess1|
  puts "trying to rate guess1: #{possible[guess1]}"
  _solutions = (0...solutions.length)
  skip = false
  mask_groups = _solutions
    .group_by { |soln| all_masks[guess1][soln] }
    .sort_by { |_, group| -group.length }
    .map { |mask1, solutions2|
      mask1 = mask1.to_s(4).rjust(5, '0')
      guess2 = if solutions2.length == 1
        [mask1, solutions[solutions2.first], 0]
      else
        (0...possible.length).map { |guess2|
          [mask1, guess2, solutions2.map { |soln2| all_masks[guess2][soln2] }.tally.values.max]
        }.min_by(&:last)
      end
      if guess2.last > best_height
        skip = true
        break
      end
      guess2
    }
  if skip
    puts "Skipping #{guess1}"
    next
  end

  mask1, guess2, height = mask_groups.max_by(&:last)

  puts "worst case for #{guess1} is \"#{mask1}\" + #{guess2}: #{height}"
  # File.write "results/#{guess1}.yaml", YAML.dump(mask_groups)
  if height < best_height
    best_height = height
    puts "(this was a new best)"
  end
  [guess1, mask1, guess2, height]
}

puts "Best all around first guess: #{results.compact.min_by(&:last)}"
# File.write 'results/top.yaml', YAML.dump(results)

# end
