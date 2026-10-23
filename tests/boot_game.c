/* Run the real boot path and report a completed game tic from its private copy. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

extern int duke3d_main(int argc, char **argv);
void boot_test_frame(void)
{
    static int reported;
    if(!reported) {
        int i=ps[myconnectindex].i;
        printf("BOOTFRAME E%dL%d player=%d health=%d\n",ud.volume_number+1,
            ud.level_number+1,sprite[i].picnum,sprite[i].extra);
        fflush(stdout);
        reported=1;
    }
}
int main(int argc, char **argv) { return duke3d_main(argc,argv); }
