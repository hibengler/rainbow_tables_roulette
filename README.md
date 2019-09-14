# rainbow_tables_roulette
An example text based roulette game that can be predicted due to psuedorandom numbers and a rainbow table.

Sorta-Requires:
The bar database for the fast sorter - fsort. On 2019 hardware, fsort takes about 7 hours to run.  Regular linux sort would take weeks.
https://github.com/hibengler/bar_database

License:
LGPL 2.0 - 

Platforms:
example program - win32 or linux

Rainbow table generator - linux

web server - linux

I wrote this years ago as a surprise for my nephew, who was getting into computers.  I had the roulette game run- and produce psuedo random numbers.   Then a mysterious web site would be able to guess the subsequent roulette numbers and thereby win money in the game.

The win32 srand() function sets to a more truly random starting seed.  Then the rand() function is called to get random numbers for the roulette number choices.   The program uses no network, and can run standalone on winxp and above platforms.

The website runs on a linux server and records the roulettes numbers one by one.  And after 5 or 6, knows what the srand seed is, so it can continue predicting the winning numbers before the actual program can.



Rainbow table generation:
There are 4 billion (actually 2 billion) possible seed numbers to start with.  So I go through all 4 billion possibilities and print out the first 6 or so numbers and the starting seed.   These 4 billion rows are then sorted by fsort (merge sort/final merge) and written to roulette.txt - this is the rainbow table with 128Gigabytes!  It could be cut in half because there are two starting seeds that create the same values.  This generation takes hours, because of the sorting.



Web site:
I borrowed the rtr server from the https://github.com/hibengler/fake_identity_generator_for_whole_countries app, which is single thread poll driven.  Then a simple binary search on the roulette.txt file which is memory mapped.  I also put an artificial limit of 7, with a special back door so I could show my nephew how it worked.





Possible uses:
If one was inclined (and I am not inclined), one could make an online texas holdem poker game, that calls srand for every sparate game.  Basically by the turn, a computer with a big enough rainbow table would know the river, and the contents of eveyones hand.   At the time I thought of this, I estimated the investment for hardware would be $30,000.00 for the cheater- and that includes letting a player cut the deck.   But that would be bad.








