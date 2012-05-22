echo "relooper"
g++ Relooper.cpp -c

echo "test"
g++ test.cpp -c -o test.o
g++ Relooper.o test.o -o test

echo "test 2"
gcc test2.c -c -o test2.o
g++ Relooper.o test2.o -o test2

echo
echo "============================="
echo

