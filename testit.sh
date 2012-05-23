echo "test"
./test &> test.out
diff test.out test.txt

echo "test 2"
./test2 &> test2.out
diff test2.out test2.txt

echo "test 3"
./test3 &> test3.out
diff test3.out test3.txt

