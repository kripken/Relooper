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

echo "test debug"
gcc test_debug.cpp -c -o test_debug.o -g
g++ Relooper.o test_debug.o -o test_debug

echo "test dead"
gcc test_dead.cpp -c -o test_dead.o -g
g++ Relooper.o test_dead.o -o test_dead

echo "test 4"
g++ test4.cpp -c -o test4.o -g
g++ Relooper.o test4.o -o test4

echo
echo "============================="
echo

