#include "socket.h"


int main() {
    NewServer server("127.0.0.1", 4444);
    server.StartServer();
    return 0;
}