mkdir -p ./bin/;
gcc -o ./bin/main main.c;
# echo -e "HEAD / HTTP/1.0\n" | nc 127.0.0.1 8888;