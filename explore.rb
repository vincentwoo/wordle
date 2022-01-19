require 'progress_bar'

$solutions = File.read('word_lists.txt').split("\n\n").first.split("\n").map(&:freeze)
$possible = eval(File.read('wordle_first_guess.txt')).map(&:first).map(&:freeze)

# solution_to_index = $solutions.map.with_index.to_h
# possible_to_index = $possible.map.with_index.to_h

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

unless File.exists? 'masks.bin'
  File.write 'masks.bin', Marshal.dump(
    $possible.map { |guess| $solutions.map { |soln| create_mask guess, soln }}
  )
end

File.open('masks.bin', 'rb') { |f| $all_masks = Marshal.load f }

def dfs solutions = (0...$solutions.length), depth = 3, path = [], bar = nil
  if solutions.size == 1
    bar.puts path + [[$solutions[solutions.first]]]
    return
  end
  return if depth == 0

  (0...$possible.length).each do |guess|
    next if guess == 0 && depth == 3
    bar.increment! if depth == 2
    mask, remaining_solutions = solutions
      .group_by { |soln| $all_masks[guess][soln] }
      .max_by { |mask, group| [group.length, -mask] }

    # if depth > 0 && mask != 500000
    #   puts "Skipping first guess: #{$possible[guess]}" if depth == 3
    #   next
    # end
    if depth == 3
      puts "Analyzing first guess: #{$possible[guess]}"
      bar = ProgressBar.new $possible.length
    end
    dfs remaining_solutions, depth - 1, path + [[$possible[guess], mask]], bar
  end
end


def entropy solutions, guess
  scores = $all_masks[guess].values_at(*solutions).tally.values
  [scores.max, scores.sum.to_f / scores.length]
  # scores.sum { |count|
  #   p = count.to_f / scores.length
  #   p * Math.log2(p)
  # }
  # scores.sum.to_f / scores.length
end

def two_turn_segmentation
  possible = (0...$possible.length)
  solutions = (0...$solutions.length)
  bar = ProgressBar.new possible.size
  possible.map do |guess1|
    bar.increment!
    solutions
      .group_by { |soln| $all_masks[guess1][soln] }
      .sort_by { |mask, group| [-group.length, mask] }
      .first(15)
      .map { |mask, _solutions|
        guess2 = (0...$possible.length).min_by { |guess2|
          entropy _solutions, guess2
        }
        [$possible[guess1], mask, $possible[guess2], entropy(_solutions, guess2)]
      }
  end
end

# ret = two_turn_segmentation
# File.write 'segmentation.bin', Marshal.dump(ret)
# pp ret
# raise
# sorted = ret.map { |entries| entries.max_by(&:last) }.sort_by(&:last)
# puts "best:"
# pp sorted.first(5)
# puts "worst:"
# pp sorted.last(5)

$memo = {}
def full_dfs solutions, depth = 2, bar = ProgressBar.new($possible.length)
  # p solutions.length, depth
  return true if solutions.size == 1
  return true if solutions.size == 2 && depth > 0
  # raise if solutions.size > 150 && depth == 1
  return false if depth == 0

  return $memo[solutions] if depth == 1 && $memo[solutions]


  ret = (0...$possible.length).any? do |guess|
    bar&.increment!
    (solutions - [guess])
      .group_by { |soln| $all_masks[guess][soln] }
      .all? { |_, group|
        if solutions.size == group.size
          false
        else
          full_dfs group, depth - 1, nil
        end
      }
  end

  $memo[solutions] = ret if depth == 1
  ret
end

f = File.open('segmentation.bin', 'rb')
data = Marshal.load f
data = data.sort_by {|entries| entries.max_by(&:last).last }.first(50)
data.map! { |entries| entries.sort_by(&:last).reverse.first(5) }
data = data.flatten(1)

data.each do |guess1, mask1, guess2, _|
  puts "trying to solve from #{guess1}, #{mask1}"
  _guess1 = $possible.index guess1
  # guess2 = $possible.index guess2
  solutions = (0...$solutions.length).select { |soln| $all_masks[_guess1][soln] == mask1 }
  if full_dfs(solutions, 2)
    puts "there WAS a total solution for #{guess1}/#{mask1}"
  else
    puts "there was NOT a total solution for #{guess1}/#{mask1}"
  end
  # groups = solutions.group_by { |soln| $all_masks[guess2][soln] }
  # if groups.all? { |mask, solutions| full_dfs solutions, 1 }
  #   p guess1, guess2
  #   p 'wow'
  # end
end

# after guessing tyler: [3, 48, 49, 50, 93, 96, 100, 110, 152, 175, 177, 180, 182, 210, 221, 253, 269, 285, 296, 301, 303, 318, 348, 373, 401, 413, 418, 437, 451, 458, 462, 494, 502, 537, 549, 553, 559, 561, 563, 575, 583, 586, 593, 595, 600, 603, 626, 630, 650, 660, 663, 682, 696, 704, 708, 720, 734, 762, 766, 767, 788, 792, 793, 801, 811, 814, 815, 818, 824, 827, 840, 853, 858, 863, 879, 913, 947, 968, 970, 979, 980, 1007, 1012, 1023, 1039, 1042, 1043, 1054, 1056, 1069, 1076, 1080, 1081, 1086, 1088, 1103, 1115, 1118, 1122, 1133, 1159, 1165, 1168, 1177, 1182, 1199, 1229, 1237, 1238, 1254, 1260, 1264, 1304, 1310, 1327, 1329, 1364, 1383, 1387, 1415, 1446, 1447, 1454, 1456, 1491, 1505, 1517, 1524, 1529, 1547, 1555, 1557, 1558, 1568, 1574, 1589, 1606, 1612, 1614, 1615, 1618, 1653, 1657, 1659, 1660, 1667, 1678, 1701, 1710, 1717, 1721, 1724, 1751, 1759, 1768, 1793, 1804, 1862, 1882, 1883, 1907, 1925, 1933, 1970, 1975, 1982, 1998, 2005, 2023, 2025, 2031, 2039, 2059, 2066, 2096, 2109, 2110, 2113, 2119, 2122, 2124, 2137, 2147, 2169, 2178, 2179, 2181, 2183, 2194, 2202, 2203, 2209, 2225, 2248, 2255, 2258, 2267, 2276, 2283, 2285]
# after guessing tyler, cains: [3, 462, 494, 968, 1007, 1229, 1721]

# p dfs [3, 462, 494, 968, 1007, 1229, 1721], 1
# p dfs [3, 48, 49, 50, 93, 96, 100, 110, 152, 175, 177, 180, 182, 210, 221, 253, 269, 285, 296, 301, 303, 318, 348, 373, 401, 413, 418, 437, 451, 458, 462, 494, 502, 537, 549, 553, 559, 561, 563, 575, 583, 586, 593, 595, 600, 603, 626, 630, 650, 660, 663, 682, 696, 704, 708, 720, 734, 762, 766, 767, 788, 792, 793, 801, 811, 814, 815, 818, 824, 827, 840, 853, 858, 863, 879, 913, 947, 968, 970, 979, 980, 1007, 1012, 1023, 1039, 1042, 1043, 1054, 1056, 1069, 1076, 1080, 1081, 1086, 1088, 1103, 1115, 1118, 1122, 1133, 1159, 1165, 1168, 1177, 1182, 1199, 1229, 1237, 1238, 1254, 1260, 1264, 1304, 1310, 1327, 1329, 1364, 1383, 1387, 1415, 1446, 1447, 1454, 1456, 1491, 1505, 1517, 1524, 1529, 1547, 1555, 1557, 1558, 1568, 1574, 1589, 1606, 1612, 1614, 1615, 1618, 1653, 1657, 1659, 1660, 1667, 1678, 1701, 1710, 1717, 1721, 1724, 1751, 1759, 1768, 1793, 1804, 1862, 1882, 1883, 1907, 1925, 1933, 1970, 1975, 1982, 1998, 2005, 2023, 2025, 2031, 2039, 2059, 2066, 2096, 2109, 2110, 2113, 2119, 2122, 2124, 2137, 2147, 2169, 2178, 2179, 2181, 2183, 2194, 2202, 2203, 2209, 2225, 2248, 2255, 2258, 2267, 2276, 2283, 2285], 2
# p dfs

# guess = 'tyler'
# depth = 1
# solutions = (0...$solutions.length)
# while true
#   puts "I think you should guess #{guess.upcase}"
#   puts "Enter results ('.' for no match, 'X' for yellow, 'O' for green):"
#   mask = gets.chomp.upcase.gsub(/./, {'.' => 0, 'X' => 1, 'O' => 2}).to_i

#   solutions = solutions.select { |soln| $all_masks[possible_to_index[guess]][soln] % 100000 == mask }
#   if solutions.length == 1
#     puts "its gotta be #{$solutions[solutions.first]}"
#     break
#   else
#     puts "#{solutions.length} solutions remain: #{$solutions.values_at(*solutions).join(', ')}"
#   end
#   raise

#   path = dfs solutions, depth
#   guess = path.first
#   depth -= 1
# end
