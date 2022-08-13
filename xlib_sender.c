#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4950"	// the port users will be connecting to
//#define serverIP "exp"    //or hostname
//#define serverIP "192.168.178.100"
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))


Display* display ;
int sockfd;
struct addrinfo hints, *servinfo, *p;

/* Signal Handler for SIGINT */
void sigint_handler(int sig_num)
{
    /* Reset handler to catch SIGINT next time.
       Refer http://en.cppreference.com/w/c/program/signal */
    printf("\n User provided signal handler for Ctrl+C \n");

    /* Do a graceful cleanup of the program like: free memory/resources/etc and exit */
    sleep(1);
    XCloseDisplay(display);
    freeaddrinfo(servinfo);
    freeaddrinfo(p);
    close(sockfd);
    exit(0);
}

int main(int argc, char *argv[])
{
		if (argc != 2) {
		fprintf(stderr,"usage: ./xlib_sender IP\n");
		exit(1);
	}
    signal(SIGINT, sigint_handler);
    int Width = 0;
    int Height = 0;
    int oledWidth= 128;
    int oledHeight= 64;
    int Bpp = 0;
    int BitsPerPixel;
    unsigned long pixel;
    unsigned char image_data_singleFrame[oledWidth*oledHeight/8];
    unsigned char threshold=128;
    uint8_t sizeUDP[3]={0x87,oledWidth,oledHeight};
    unsigned char copy_of_Ximage[oledWidth][oledHeight];
    unsigned int histogram[256];
    
    int rv;
    int numbytes;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
    
    
    if ((numbytes = sendto(sockfd, &sizeUDP, 3, 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
    display = XOpenDisplay(NULL);
    sleep(1);
    while(1){
	    //Display* display = XOpenDisplay(NULL);
	    //display = XOpenDisplay(NULL);
	    //Window root = DefaultRootWindow(display);
	    Window root = RootWindow(display, DefaultScreen(display));

	    XWindowAttributes attributes = {0};
	    XGetWindowAttributes(display, root, &attributes);

	    Width = attributes.width;
	    Height = attributes.height;

	    XImage* img = XGetImage(display, root, 0, 0 , Width, Height, AllPlanes, ZPixmap);
	    
	    unsigned char byte_value = 0;
	    unsigned int index_frames = 0;
	    unsigned char opturi=0;
	    
	    memset(histogram,0,256*sizeof(unsigned int));
	    
	    for (int y = 0; y < oledHeight; y++)
		{
			for (int x = 0; x < oledWidth; x++)
			{
			pixel = XGetPixel(img, (int)(Width/oledWidth*x), (int)(Height/oledHeight*y));
			unsigned char  gray_px_value = ((pixel & img->red_mask) >> 16)*0.11 + ((pixel & img->green_mask) >> 8)*0.56 + (pixel & img->blue_mask)*0.33;
			++histogram[gray_px_value];
			copy_of_Ximage[x][y]=gray_px_value;
			
			}
		}
	    const float cutOffPercentage = 0.05;
	    unsigned char lowerBound, upperBound ;
	    unsigned int histAccu = 0;
	    const unsigned int lowerPercentile = cutOffPercentage*oledWidth*oledHeight ;
	    const unsigned int upperPercentile = (1-cutOffPercentage)*oledWidth*oledHeight;
	    for ( int h = 0 ; h < 256 ; h++ )
	    {
		    histAccu += histogram[h] ;
		    if( histAccu <= lowerPercentile)
		    {
		    	lowerBound = h ;continue ;
		    }
		    if ( histAccu >= upperPercentile){
		    	upperBound = h ;break ;
		    }
	    }
	    float histScale = 255. / (upperBound - lowerBound);
	    threshold = (upperBound - lowerBound)/2;
	    for (int y = 0; y < oledHeight; y++)
	    {
	      for (int x = 0; x < oledWidth; x++)
		{
		   int new_px_value = histScale * ( (int)copy_of_Ximage[x][y] - lowerBound);
		   copy_of_Ximage[x][y] = MIN(255, MAX(0,new_px_value));
			if (copy_of_Ximage[x][y] >= threshold)  {byte_value+=pow(2,8-opturi);}
			if (opturi==8) 
			{
			   image_data_singleFrame[index_frames]=byte_value;
			   index_frames++;
			   //printf("  byte_value %d\n",byte_value);
			   //printf("index_frames %d\n",index_frames);
			   byte_value = 0;
			   opturi=0;
			   }
			opturi++;
			}
		}
	    XDestroyImage(img);
	    if ((numbytes = sendto(sockfd, &image_data_singleFrame, oledWidth*oledHeight/8, 0,
		p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
    }
    XCloseDisplay(display);
    return 0;
}

