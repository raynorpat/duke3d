/* Flush transport readiness before the other test peer starts. */
#include <stdio.h>
extern int duke3d_main(int argc, char **argv);
int main(int argc, char **argv)
{
    setvbuf(stdout,NULL,_IONBF,0);
    return duke3d_main(argc,argv);
}
