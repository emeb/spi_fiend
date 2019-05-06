/*
 * cdc_prog.c - send fpga bitstream via USB CDC -> SPI bridge
 * 01-25-19 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#define VERSION "0.2"

static void help(void) __attribute__ ((noreturn));

int cdc_file;		/* CDC device */
char buffer[64];

/*
 * print help
 */
static void help(void)
{
	fprintf(stderr,
	    "Usage: cdc_prog <options> [FILE]\n"
		"  FILE is a file containing the FPGA bitstream or flash data to download\n"
		"  -a <addr> address for flash erase/read/write (default = 0x0)\n"
		"  -l <len> length for flash read/write (default = 0x8000)\n"
		"  -e erase 32k block at <addr>\n"
		"  -i get flash id\n"
		"  -n disable erase before write\n"
		"  -p <port> specify the CDC port (default = /dev/ttyACM0)\n"
		"  -r read from flash at <addr> for <len> to stdout\n"	
		"  -w write from FILE to flash at <addr>\n"	
		"  -v enables verbose progress messages\n"
		"  -V prints the tool version\n");
	exit(1);
}

/*
 * handle erase
 */
void erase(unsigned int addr)
{
	fprintf(stderr, "Erasing: 0x%08X\n", addr);
	
	/* send command */
	buffer[0] = 'e';
	buffer[1] = (addr>>24)&0xff;
	buffer[2] = (addr>>16)&0xff;
	buffer[3] = (addr>>8)&0xff;
	buffer[4] = addr&0xff;
	write(cdc_file, buffer, 5);

	/* get result */
	read(cdc_file, buffer, 1);
	fprintf(stderr, "Result: %d\n", buffer[0]);
}

int main(int argc, char **argv)
{
	int flags = 0, verbose = 0, sz = 0, id = 0, er = 0, rd = 0, wr = 0, ne = 0,
		rsz, wsz, buffs;
	unsigned int addr = 0, len = 0x8000;
    char *port = "/dev/ttyACM0";
    FILE *infile;
    struct termios termios_p;
    
	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-')
	{
		switch (argv[1+flags][1])
		{
		case 'a':
			if (2+flags < argc)
                addr = strtol(argv[flags+2], NULL, 0);
			fprintf(stderr, "addr = 0x%08X\n",addr);
			flags++;
			break;
		case 'e':
			er = 1;
			break;
		case 'i':
			id = 1;
			break;
		case 'l':
			if (2+flags < argc)
                len = strtol(argv[flags+2], NULL, 0);
			fprintf(stderr, "length = 0x%08X\n",len);
			flags++;
			break;
		case 'n':
			ne = 1;
			break;
		case 'p':
			if (2+flags < argc)
                port = argv[flags+2];
			flags++;
			break;
		case 'r':
			rd = 1;
			break;
		case 'w':
			wr = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'V':
			fprintf(stderr, "cdc_prog version %s\n", VERSION);
			exit(0);
		default:
			fprintf(stderr, "Error: Unsupported option "
				"\"%s\"!\n", argv[1+flags]);
			help();
		}
		flags++;
	}
    
    /* open output port */
    cdc_file = open(port, O_RDWR);
    if(cdc_file<0)
	{
		fprintf(stderr,"Couldn't open cdc device %s\n", port);
        exit(1);
	}
	else
	{
        /* port opened so set up termios for raw binary */
        fprintf(stderr,"opened cdc device %s\n", port);
        
        /* get TTY attribs */
        tcgetattr(cdc_file, &termios_p);
        
        /* input modes */
        termios_p.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IUCLC | INLCR| IXANY );

        /* output modes - clear giving: no post processing such as NL to CR+NL */
        termios_p.c_oflag &= ~(OPOST|OLCUC|ONLCR|OCRNL|ONLRET|OFDEL);

        /* control modes - set 8 bit chars */
        termios_p.c_cflag |= ( CS8 ) ;

        /* local modes - clear giving: echoing off, canonical off (no erase with 
        backspace, ^U,...), no extended functions, no signal chars (^Z,^C) */
        termios_p.c_lflag &= ~(ECHO | ECHOE | ICANON | IEXTEN | ISIG);

        termios_p.c_cflag |= CRTSCTS;	// using flow control via CTS/RTS
        
        /* set attribs */
        tcsetattr(cdc_file, 0, &termios_p);		
    }
	
	/* handle Erase cmd */
	if(er)
	{
		/* do erase */
		erase(addr);
		
		/* finish */
        close(cdc_file);
		exit(0);
	}
	
	/* handle ID cmd */
	if(id)
	{
		/* send command */
		buffer[0] = 'i';
		write(cdc_file, buffer, 1);
    
		/* get result */
		read(cdc_file, buffer, 4);
		fprintf(stderr, "ID0: 0x%02X\n", buffer[0]&0xff);
		fprintf(stderr, "ID1: 0x%02X\n", buffer[1]&0xff);
		fprintf(stderr, "ID2: 0x%02X\n", buffer[2]&0xff);
		fprintf(stderr, "Result: %d\n", buffer[3]);
		
		/* finish */
        close(cdc_file);
		exit(0);
	}
    
	/* handle Flash Read cmd */
	if(rd)
	{
		/* send command */
		buffer[0] = 'r';
		buffer[1] = (addr>>24)&0xff;
		buffer[2] = (addr>>16)&0xff;
		buffer[3] = (addr>>8)&0xff;
		buffer[4] = addr&0xff;
		buffer[5] = (len>>24)&0xff;
		buffer[6] = (len>>16)&0xff;
		buffer[7] = (len>>8)&0xff;
		buffer[8] = len&0xff;
		write(cdc_file, buffer, 9);
		
		/* get data, send to stdout */
		while(len)
		{
			unsigned int rlen, alen;
			rlen = (len > 64) ? 64 : len;
			alen = read(cdc_file, buffer, rlen);
#if 1
			write(STDOUT_FILENO, buffer, alen);
#else
			{
				unsigned int cnt = 0;
				while(alen--)
				{
					fprintf(stdout,"0x%02X\n", buffer[cnt++]&0xff);
				}
			}
#endif
			len -= rlen;
		}
    
		/* get result */
		fprintf(stderr, "Waiting for Result...\n");
		read(cdc_file, buffer, 1);
		fprintf(stderr, "Result: %d\n", buffer[0]);
		
		/* finish */
        close(cdc_file);
		exit(0);		
	}
	
    /* get file */
    if (2+flags > argc)
    {
        /* missing argument */
        fprintf(stderr, "Error: Missing bitstream file\n");
        close(cdc_file);
        help();
    }
    else
    {
        /* open file */
        if(!(infile = fopen(argv[flags + 1], "rb")))
        {
            fprintf(stderr, "Error: unable to open file %s for read\n", argv[flags + 1]);
            close(cdc_file);
            exit(1);
        }
        
        /* get length */
        fseek(infile, 0L, SEEK_END);
        sz = ftell(infile);
        fseek(infile, 0L, SEEK_SET);
        fprintf(stderr, "Opened file %s with size %d\n", argv[flags + 1], sz);
    }
    
	/* send the header */
	if(wr)
	{
		/* erase? */
		if(ne==1)
			fprintf(stderr, "Pre-erase disabled\n");
		else
		{
			int esz = sz;
			unsigned int eaddr = addr;
			
			while(esz > 0)
			{
				erase(eaddr);
				eaddr += 0x8000;
				esz -= 0x8000;
			}
		}
		
		/* Flash write header */
		buffer[0] = 'w';
		buffer[1] = (addr>>24)&0xff;
		buffer[2] = (addr>>16)&0xff;
		buffer[3] = (addr>>8)&0xff;
		buffer[4] = addr&0xff;
		buffer[5] = (sz>>24)&0xff;
		buffer[6] = (sz>>16)&0xff;
		buffer[7] = (sz>>8)&0xff;
		buffer[8] = sz&0xff;
		write(cdc_file, buffer, 9);
	}
	else
    {
		/* FPGA configuration header */
		buffer[0] = 's';
		buffer[1] = (sz>>24)&0xff;
		buffer[2] = (sz>>16)&0xff;
		buffer[3] = (sz>>8)&0xff;
		buffer[4] = sz&0xff;
		write(cdc_file, buffer, 5);
    }
	
    /* send the body 64 bytes at a time */
    wsz = 0;
    buffs = 0;
    while((rsz = fread(buffer, 1, 64, infile)) > 0)
    {
        write(cdc_file, buffer, rsz);
        wsz += rsz;
        buffs++;
    }
    fprintf(stderr, "Sent %d bytes, %d buffers\n", wsz, buffs);
	
	/* get result */
	read(cdc_file, buffer, 1);
	fprintf(stderr, "Result: %d\n", buffer[0]);
    
    fclose(infile);
    close(cdc_file);
    return 0;
}