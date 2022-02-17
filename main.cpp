#include <unistd.h>
#include "Webserver.h"

int main(){
    Webserver server(1316,3,60000,false,4);
    server.Start();
}