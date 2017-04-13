
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "imx_pwm.h"

static int brightness = -1;
static int ess = 0;

int
pwm_options(int argc, char *argv[])
{
    int     			c;
    int     			prev_optind;
    int     			done = 0;

    while (!done) {
        prev_optind = optind;
        c = getopt(argc, argv, "l:sv:");
        switch (c) {
	case 'l':
            brightness = strtoul(optarg, &optarg, NULL);
	    break;
        case 'v':
            break;
	case 's':
	    ess = 1;
	    break;
        case -1:
            if (prev_optind < optind) { /* -- */ 
                return -1;
	    }
            if (argv[optind] == NULL) {
                done = 1;
                break;
            } 
            if (*argv[optind] != '-') {
                ++optind;
                break;
            }
            return -1;
        case ':':
        default:
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    imx_pwm_init();
    pwm_options(argc, argv);

    if (ess) {
	/* enabled spread spectrum */
	imx_enable_spread_spectrum();
    }
    else if (brightness < 0 || brightness > 16) {
    //else if (brightness < 0 || brightness > 7) {
	//printf("Please input brightness level from  0 - 7\n");
	printf("usage: pwm -l [brightness 0-16]\n");
    }
    else {
	//brightness = 7 - brightness;
	brightness = 16 - brightness;
    	pwm_backlight_update_status(brightness);
    }

    return 0;
}

