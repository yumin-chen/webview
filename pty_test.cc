#include <pty.h>
#include <unistd.h>
int main() {
    int master;
    forkpty(&master, NULL, NULL, NULL);
    return 0;
}
