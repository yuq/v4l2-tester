#include <stdio.h>
#include "video.h"

int main(int argc, char** argv)
{
	video_init(stdout);
	video_start();
	getchar();
	video_stop();

	return 0;
}
