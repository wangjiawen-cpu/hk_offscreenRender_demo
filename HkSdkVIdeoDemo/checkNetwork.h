#ifndef CHECKNETWORK_H
#define CHECKNETWORK_H

bool checkVideoConnect(unsigned char * ip,int port,int ms = 200);

int caleVideoDelay(char * ip,int port);

#endif // CHECKNETWORK_H
