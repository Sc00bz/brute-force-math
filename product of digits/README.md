# Recursive product of digits
Take a nonnegative integer in base 10 then multiply all its digits together. Then do it again and again until the product is a single digit. Now find the smallest number where it takes N steps to get down to a single digit.

## Why
Numberphile video: https://youtu.be/Wim9WJeDTHQ

## Previously Known
```
Steps | Smallest number
------+---------------------
0     | 0
1     | 10
2     | 25
3     | 39
4     | 77
5     | 679
6     | 6788
7     | 68889
8     | 2677889
9     | 26888999
10    | 377888899
11    | 277777788888899
12    | Should be imposible
```

Largest 11 step should be (ignoring padding it with 1s): 77777733332222222222222222222

Note that's 277777788888899 but expandened and reordered

## What has been checked (according to source)
Upto 233 digit numbers.

## What I Ran
I ran this with suffixes of 0 to 1024 digits (ie `MIN_DIGITS = 0`, `MAX_DIGITS = 1024`):
```
Prefix | Suffix regex
-------+--------------
2      | 7*8*9*
26     | 7*8*9*
3      | 7*8*9*
4      | 7*8*9*
6      | 7*8*9*
None   | 7*8*9*
3      | 5+7*9*
None   | 5+7*9*
```
Note regex repeaters: * means 0 or more, + means 1 or more

I had an earlier version of this that ran the prefixes with 0 to 512 digits of 7, 8 (or 5), and 9. I forget what I had it output but it was at least any 11+ step number. Likely also outputed 10 steps too.

## "Code Output" (I made it look nice and added info)
The output summary for `MIN_DIGITS = 0` and `MAX_DIGITS = 1024`:
```
Steps | Count
------+------------
0     |          9
1     |          0
2     | 1438995148
3     |      11994   Largest is MAX_DIGITS = 155
4     |        387   Largest is MAX_DIGITS =  47
5     |        142   Largest is MAX_DIGITS =  24
6     |         41   Largest is MAX_DIGITS =  18
7     |         12   Largest is MAX_DIGITS =  17
8     |          8   Largest is MAX_DIGITS =  17
9     |          5   Largest is MAX_DIGITS =  13
10    |          2   Largest is MAX_DIGITS =  12
11    |          2   Largest is MAX_DIGITS =  16
Took: 5 hr, 17 min (i5-6500 3.2 GHz)
```
Note this avoids all numbers that will end in 1 step. The 9 zero steps are "" (ie 0), 2, 3, 4, 5, 6, 7, 8, 9 from:
```
Prefix | Suffix regex | Zero steps
-------+--------------+-------------
2      | 7*8*9*       | 2
3      | 7*8*9*       | 3
4      | 7*8*9*       | 4
6      | 7*8*9*       | 6
None   | 7*8*9*       | "", 7, 8, 9
None   | 5+7*9*       | 5
```

## Why it is Stupid to Run Farther
There's a `1-.1^digits` chance you get a 0 digit in the product, then next step is the last.

There's a `1-.1^digits/.5^digits` chance you get a 5 digit in the product, then the next even digit in any product means next step is the last.

Basically it's like `1-.8^digits` chance of not ending next step. You know there could magically be another but the largest minimized >2 step I found is 155 digits and none others appeared in any 1024 digit number.