# -I/usr/include is needed because http://stackoverflow.com/questions/1107940/size-t-can-not-be-found-by-g-4-1-or-others-on-ubuntu-8-1
g++ -o am main.cpp automata_mista.cpp -I/usr/include
