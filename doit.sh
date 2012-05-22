echo "1"
g++ Relooper.cpp -c
echo "2"
g++ test.cpp -c -o test_cpp.o
echo "3"
gcc test.c -c -o test_c.o
echo "4"
g++ Relooper.o test_cpp.o -o test_cpp
echo "5"
g++ Relooper.o test_c.o -o test_c

