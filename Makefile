all:
	c++ -o libtest.so -shared -fPIC main.cpp -llibmem
	javac target/main/Main.java
