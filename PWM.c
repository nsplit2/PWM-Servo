//./output/host/usr/bin/arm-linux-gnueabihf-gcc -o PWMservo PWMservo.c -lpigpio -lpthread
//PWMservo --gpio 17 --min 750 --max 2300 --multi 10.3 --deg 18
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pigpio.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

#define SERVO_GPIO 17
//#define SERVOPOS "/var/run/servo.pos"
char SERVOPOS[100];

void usage() {
	printf(
		 "Usage: PWMgpio [options]\n\n"
		 "Options:\n"
		 "-g | --gpio PIN      Signal PIN of Motor/Servo\n"
		 "-s | --max PWM       Max possible PWM\n"
		 "-e | --min PWM       Min possible PWM\n"
		 "-f | --multi float   Multiplier, -+PWM per step (10.1)\n"
		 "-d | --deg           Angle degree 0-180\n"
		 "-a | --speed         Rotation speed 5-1\n"
		 "");
}

const char short_options[] = "g:s:e:f:d:a:";
const struct option
long_options[] = {
	{ "gpio",  required_argument, NULL, 'g' },
	{ "max",   required_argument, NULL, 's' },
	{ "min",   required_argument, NULL, 'e' },
	{ "multi", required_argument, NULL, 'f' },
	{ "deg",   required_argument, NULL, 'd' },
	{ "speed", required_argument, NULL, 'a' },
	{ 0, 0, 0, 0 }
};

// mini 9g servo
// 150 degs
// min pulse: 750, max: 2300

int main(int argc, char *argv[]) {
	int prev,pw=0;
	FILE *fp;
	int GPIO = SERVO_GPIO;
	int width = 1000;
	int deg = 1;
	char conv[256];
	int INIT = 1677;
	int MIN,MAX,SPEED;
	float MULTI;
	
	if ( argc != 13) {
		printf ("* %d\n",argc);
		usage();
		return 0;
	}
	
	for (;;) {
		int idx;
		int c;

		c = getopt_long(argc, argv, short_options, long_options, &idx);
		
		if (-1 == c) {
			break;
		}
		
		switch (c) {
			
			case 'g':
				GPIO = atoi(optarg);
				break;
			case 's':
				MAX = atoi(optarg);
				break;
			case 'e':
				MIN = atoi(optarg);
			case 'f':
				MULTI = atof(optarg);
			case 'd':
				deg = atoi(optarg);
				break;
			case 'a':
				SPEED = atoi(optarg);
				break;
			default:
				usage(stderr, argc, argv);
				exit(EXIT_FAILURE);
		}
	}

	if (deg<0||deg>180) {
		printf ("* Error bad deg range.\n");
		return 0;
	}
	if (SPEED<1||SPEED>5) {
		printf ("* Error bad speed range.\n");
		return 0;
	}
	
	// calculate speed
	if (SPEED == 5)
		SPEED = 1000000 / 1000;
	if (SPEED == 4)
		SPEED = 1000000 / 800;
	if (SPEED == 3)
		SPEED = 1000000 / 600;
	if (SPEED == 2)
		SPEED = 1000000 / 400;
	if (SPEED == 1)
		SPEED = 1000000 / 100;
	
	// calculate deg to width
	printf ("* Deg: %d, Step: %f, Speed: %d\n",deg,MULTI,SPEED);
	//width = deg*10.3+750;
	width = (deg*MULTI+MIN);
	
	
	sprintf(SERVOPOS,"/var/run/servo%d.pos",GPIO);
	
	if( access(SERVOPOS, F_OK ) != -1 ) {
		// file exists
		fp = fopen (SERVOPOS,"r");
		fgets(conv,5,fp);
		fclose(fp);
		prev = atoi(conv);
		printf ("* current position: %d, new %d\n",prev, width);
		
		fp = fopen (SERVOPOS,"w");
		sprintf(conv,"%d",width);
		fputs (conv,fp);
		fclose(fp);
	} else {
		fp = fopen (SERVOPOS,"w");
		sprintf(conv,"%d",INIT);
		fputs (conv,fp); //  set default 90 degs
		fclose(fp);
		prev=INIT;
		printf ("* current position: %d, new %d\n",prev, width);
		
		fp = fopen (SERVOPOS,"w");
		sprintf(conv,"%d",width);
		fputs (conv,fp);
		fclose(fp);
	}
	
	if (gpioInitialise()<0) {
		printf ("* Error init GPIO\n");
		return 0;
	}
	
	if (prev<width) {
		for (pw=prev; pw<width; pw+=5) {
			gpioServo(GPIO, pw);
			usleep(SPEED);
		}
	} else {
		for (pw=prev; pw>width; pw-=5) {
			gpioServo(GPIO, pw);
			usleep(SPEED);
		}
	}
	
	printf ("quit..\n");
	gpioTerminate();

	return 1;
}
