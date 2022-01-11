unless File.exist? 'word_lists.txt'
  require 'uri'
  require 'net/http'
  js = Net::HTTP.get(URI.parse('https://www.powerlanguage.co.uk/wordle/main.db1931a8.js'))
  lists = js.scan(/(\[(?:"[a-z]{5}",?)+\])/).map { |s| eval(s.first) }
  File.write('word_lists.txt', lists.map { |l| l.join("\n") }.join("\n\n"))
end

lists = File.read('word_lists.txt').split("\n\n")
solutions, possible = lists.map { |s| s.split("\n").map(&:freeze) }
possible += solutions

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

guess = 'tolar'
while true
  puts "I think you should guess #{guess.upcase}"
  puts "Enter results ('.' for no match, 'X' for yellow, 'O' for green):"
  mask = gets.chomp.upcase.gsub(/./, {'.' => 0, 'X' => 1, 'O' => 2}).to_i(4)

  solutions.select! { |soln| check_guess(guess.chars, soln) == mask }
  if solutions.length == 1
    puts "its gotta be #{solutions.first}"
    break
  else
    puts "#{solutions.length} solutions remain: #{solutions.join(', ')}"
  end

  guess = possible.min_by { |guess|
    solutions.map { |soln| check_guess guess, soln }.tally.values.max - (solutions.include?(guess) ? 1 : 0)
  }
end
