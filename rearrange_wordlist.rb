solutions, possible = File.read('word_lists.txt').split("\n\n").map { |s| s.split("\n").map(&:freeze) }
ordered = File.read('ranked_first_guesses.txt').split("\n").map(&:freeze)

require 'set'
solutions = Set.new solutions

ordered_solutions = []
ordered_possibles = []
ordered.each do |word|
	if solutions.include? word
		ordered_solutions.push word
	else
		ordered_possibles.push word
	end
end

ordered_solutions.each { |w| puts w }
puts ''
ordered_possibles.each { |w| puts w }