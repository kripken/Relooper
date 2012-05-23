echo "relooper"
g++ Relooper.cpp -c -g

echo "test"
g++ test.cpp -c -o test.o -g
g++ Relooper.o test.o -o test

echo "test 2"
gcc test2.c -c -o test2.o -g
g++ Relooper.o test2.o -o test2

echo "test 3"
gcc test3.c -c -o test3.o -g
g++ Relooper.o test3.o -o test3

echo
echo "============================="
echo

