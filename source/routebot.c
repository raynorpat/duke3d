/* Optional input author. Only DOS playback supplies reference results. */
#include <limits.h>
#include "duke3d.h"
#include "routebot.h"

static struct waypoint { int x,y,action,tics; } points[4096];
static int count,current,elapsed,waiting;

int routebot_load(const char *path)
{
    FILE *f=fopen(path,"r");
    char line[256],action[16],argument[16],extra,*end;
    int n,line_number=0,tics;
    count=current=elapsed=waiting=0;
    if(!f) { perror(path); return 0; }
    while(fgets(line,sizeof(line),f))
    {
        char *p=strchr(line,'#');
        line_number++;
        if(p) *p=0;
        p=line; while(isspace((unsigned char)*p)) p++;
        if(!*p) continue;
        tics=0; action[0]=0;
        if(count==4096) goto bad;
        n=sscanf(p,"%d %d %15s %15s %c",&points[count].x,&points[count].y,action,argument,&extra);
        if(n==2) points[count].action=0;
        else if(n==3 && !strcmp(action,"use")) points[count].action=1;
        else if(n==3 && !strcmp(action,"fire")) points[count].action=2;
        else if(n==3 && !strcmp(action,"jump")) points[count].action=3;
        else if(n==3 && !strcmp(action,"crouch")) points[count].action=5;
        else if(n==4 && (!strcmp(action,"wait") || !strcmp(action,"fire") || !strcmp(action,"weapon"))) {
            long value=strtol(argument,&end,10);
            if(*end || value<=0 || value>INT_MAX) goto bad;
            if(!strcmp(action,"weapon") && value>10) goto bad;
            tics=value;
            points[count].action=!strcmp(action,"wait") ? 4 : !strcmp(action,"fire") ? 2 : 6;
        }
        else goto bad;
        points[count++].tics=tics;
    }
    fclose(f);
    if(count) return 1;
    fprintf(stderr,"%s: empty route\n",path); return 0;
bad:
    fprintf(stderr,"%s:%d: expected x y [use|jump|crouch|fire [tics]|wait tics|weapon 1-10]\n",path,line_number);
    fclose(f); count=0; return 0;
}

int routebot_next(int32_t x,int32_t y,short angle,input *out)
{
    memset(out,0,sizeof(*out));
    while(current<count)
    {
        struct waypoint *p=&points[current];
        int dx=p->x-x,dy=p->y-y,dist=FindDistance2D(dx,dy),turn;
        if(elapsed++>=26*60 && !((p->action==4 || (p->action==2 && p->tics)) && dist<=128)) {
            fprintf(stderr,"route waypoint %d timed out at %d %d (target %d %d)\n",current+1,x,y,p->x,p->y);
            return -1;
        }
        if(p->action==1 && (elapsed&1)) out->bits|=1u<<29;
        if(p->action==2) out->bits|=1u<<2;
        if(p->action==3 && (elapsed%26)==1) out->bits|=1;
        if(p->action==5) out->bits|=2;
        if(p->action==6) out->bits|=(unsigned)p->tics<<8;
        if(dist<=128)
        {
            if(p->action==1) out->bits|=1u<<29;
            if(p->action==3) out->bits|=1;
            if((p->action==4 || (p->action==2 && p->tics)) && ++waiting<p->tics) return 1;
            printf("ROUTE waypoint %d reached at %d %d\n",current+1,x,y);
            current++; elapsed=waiting=0;
            if(p->action) return 1;
            continue;
        }
        out->fvel=(int64_t)dx*min(dist*4,2047)/dist;
        out->svel=(int64_t)dy*min(dist*4,2047)/dist;
        turn=((getangle(dx,dy)+1024-angle)&2047)-1024;
        out->avel=min(max(turn/8,-64),64);
        return 1;
    }
    return 0;
}
