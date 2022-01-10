require 'set'

unless File.exist? 'word_lists.txt'
  require 'uri'
  require 'net/http'
  js = Net::HTTP.get(URI.parse('https://www.powerlanguage.co.uk/wordle/main.db1931a8.js'))
  lists = js.scan(/(\[(?:"[a-z]{5}",?)+\])/).map { |s| eval(s.first) }
  File.write('word_lists.txt', lists.map { |l| l.join("\n") }.join("\n\n"))
end

lists = File.read('word_lists.txt').split("\n\n")
solutions, possible = lists.map { |s| s.split("\n").map(&:chars) }
possible += solutions

def remaining_solutions solutions, rejected, present, found
  solutions.select { |soln|
    (soln & rejected).length == 0 &&
    found.all? { |c, i| soln[i] == c } &&
    present.all? { |c, i| soln[i] != c && soln.include?(c) }
  }
end

guess = 'roate'
while true
  puts "I think you should guess #{guess.upcase}"
  puts "Enter results ('.' for no match, 'X' for yellow, 'O' for green):"
  mask = gets.chomp.upcase

  rejected = []
  present = []
  found = []
  mask.chars.each.with_index do |m, i|
    case m
    when '.'
      rejected.push guess[i]
    when 'X'
      present.push [guess[i], i]
    when 'O'
      found.push [guess[i], i]
    else
      raise 'Whoops!'
    end
  end

  solutions = remaining_solutions solutions, rejected.uniq, present, found

  if solutions.length == 1
    puts "its gotta be #{solutions.first.join}"
    break
  else
    puts "#{solutions.length} solutions remain: #{solutions.map(&:join).join(', ')}"
  end

  memo = {}
  guess = possible.min_by { |guess|
    score = solutions.sum do |solution|
      rejected = []
      present = []
      found = []
      guess.each.with_index { |c, i|
        if solution[i] == c
          found.push [c, i]
        elsif solution.include?(c)
          present.push [c, i]
        else
          rejected.push c
        end
      }
      rejected = rejected.uniq.sort

      memo[[rejected, present, found]] ||=
        remaining_solutions(solutions, rejected, present, found).count
    end
    score -= 1 if solutions.include?(guess)
    score
  }.join
end
