# set up player
cd ~/cvs/player
. ./setup

cd ~/cvs/random_deduct/roulette/analysis
export TMP=/t/tmp
time ./roulette_deduct_table | fsort 22000000000 >/t/roulette.txt

cd ~/cvs/random_deduct/roulette/web
gcc -c bsearch.c
gcc -o gamebreaker bsearch.c gamebreaker.c rtr.c util.c
 export f=$PWD
 cd /t
 $f/gamebreaker
 
