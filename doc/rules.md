# Rules of the game DDC
## Map
```text
# initial
A B C D E F G H I J K L M N O
. . . . . . . . . . . . . . . 1
. . . . . . . . . . . . . . . 2
. . . . . . . . . . . . . . . 3
. . . . . . . . . . . . . . . 4
. . . . . . . . . . . O . . . 5
. . . . . . . . . . . . . . . 6
. . . . . . . . . . . . . . . 7
. . . . . . . . . . . . . . . 8
. . . . . . . . . . . . . . . 9
. . . . . . . . . . . . . . . 10
. . . . . . . . . . . . . . . 11
. . . . . . . . . . . . . . . 12
. . . . . . . . . . . . . . . 13
. . . . . . . . . . . . . . . 14
. . . X . . . . . . . . . . . 15
. . . . . . . . . . . . . . . 16
. . . . . . . . . . . . . . . 17
. . . . . . . . . . . . . . . 18
. . . . . . . . . . . . . . . 19
```

## Win
- ```O```side: remove 15D.
- ```X```side: remove 5L.
- draw: no one can make a legal move.

## Examples
- ```O```: stones of one player.
- ```X```: stones of another player.
- ```o```: possible moves of ```O``` side.
- ```x```: possible moves of ```X``` side.
- ```b```: possible moves of both sides.
```text
# example 1
       O|X
. o .   |   o O o
o O o   |   o O o
. o .   |   . o .
  before|after

# exmple 2
       X|O
. x .   |   x X x
x X x   |   x X x
. x .   |   . x .
  before|after

# example 3
           O|X
o o o . x   |   o o o o .
O O O o X   |   O O O O .
o o o . x   |   o o o o .
      before|after

# example 4
               X|O
o O O O O o .   |   o O O O . . .
. o o o O o .   |   . o o . x . .
. . . . b . .   |   . . . x X x .
. . . x X x .   |   . . . x X x .
. . . x X x .   |   . . . x X x .
. . . . x . .   |   . . . . x . .
          before|after

# example 5
         O|X
o O o .   |   o O O o
O O . x   |   O O o x
o . X X   |   o . X X
. x X x   |   . x X x
    before|after

# example 6
         X|O
O O b X   |   O O X X
    before|after
```