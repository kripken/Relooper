echo "test"
./test &> test.out
diff -U 5 test.out test.txt

echo "test 2"
./test2 &> test2.out
diff -U 5 test2.out test2.txt

echo "test 3"
./test3 &> test3.out
diff -U 5 test3.out test3.txt

echo "test debug"
./test_debug &> test_debug.out
diff -U 5 test_debug.out test_debug.txt

echo "test dead"
./test_dead &> test_dead.out
diff -U 5 test_dead.out test_dead.txt

