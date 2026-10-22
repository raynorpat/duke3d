#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "routebot.h"
static int failures;
#define CHECK(c) do { if(!(c)) { printf("FAIL line %d: %s\n",__LINE__,#c); failures++; } } while(0)
int main(void)
{
    FILE *f=fopen("routebot-test.route","w"); input in; int i;
    fputs("# test\n0 0 wait 2\n1024 0 use\n1024 1024 fire\n2048 1024 jump\n",f); fclose(f);
    initengine(); CHECK(routebot_load("routebot-test.route"));
    CHECK(routebot_next(0,0,0,&in)==1 && in.fvel==0 && in.bits==0);
    CHECK(routebot_next(0,0,0,&in)==1 && in.fvel==0);
    CHECK(routebot_next(0,0,0,&in)==1 && in.fvel>0 && in.svel==0 && (in.bits&(1u<<29)));
    CHECK(routebot_next(1024,0,0,&in)==1 && (in.bits&(1u<<29)));
    CHECK(routebot_next(1024,0,0,&in)==1 && in.svel>0 && in.avel>0 && (in.bits&4));
    CHECK(routebot_next(1024,1024,0,&in)==1 && (in.bits&4));
    CHECK(routebot_next(1024,1024,0,&in)==1 && in.fvel>0 && (in.bits&1));
    CHECK(routebot_next(2048,1024,0,&in)==1 && (in.bits&1));
    CHECK(routebot_next(2048,1024,0,&in)==0);
    f=fopen("routebot-test.route","w"); fputs("10 20 wait 0\n",f); fclose(f);
    CHECK(!routebot_load("routebot-test.route"));
    f=fopen("routebot-test.route","w"); fputs("1024 0\n",f); fclose(f);
    CHECK(routebot_load("routebot-test.route"));
    for(i=0;i<26*60;i++) CHECK(routebot_next(0,0,0,&in)==1);
    CHECK(routebot_next(0,0,0,&in)==-1);
    f=fopen("routebot-test.route","w");
    fputs("0 0 crouch\n0 0 fire 3\n0 0 weapon 3\n0 0 use crouch\n0 0 face 512\n0 0 face 0\n",f); fclose(f);
    CHECK(routebot_load("routebot-test.route"));
    CHECK(routebot_next(0,0,0,&in)==1 && (in.bits&2));
    CHECK(routebot_next(0,0,0,&in)==1 && (in.bits&4));
    CHECK(routebot_next(0,0,0,&in)==1 && (in.bits&4));
    CHECK(routebot_next(0,0,0,&in)==1 && (in.bits&4));
    CHECK(routebot_next(0,0,0,&in)==1 && ((in.bits>>8)&15)==3);
    CHECK(routebot_next(0,0,0,&in)==1 && (in.bits&2) && (in.bits&(1u<<29)));
    CHECK(routebot_next(0,0,0,&in)==1 && in.avel>0 && in.fvel==0);
    CHECK(routebot_next(0,0,512,&in)==1 && in.avel==0);
    CHECK(routebot_next(0,0,512,&in)==1 && in.avel<0);
    CHECK(routebot_next(0,0,0,&in)==1 && in.avel==0);
    CHECK(routebot_next(0,0,0,&in)==0);
    remove("routebot-test.route"); return failures!=0;
}
