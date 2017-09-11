/*******************************************************************************
 * Copyright (c) 2012, 2013 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *    Ian Craggs - change delimiter option from char to string
 *    Al Stockdill-Mander - Version using the embedded C client
 *******************************************************************************/

/*
 
 stdout subscriber
 
 compulsory parameters:
 
  topic to subscribe to
 
 defaulted parameters:
 
	--host localhost
	--port 1883
	--qos 2
	--delimiter \n
	--clientid stdout_subscriber
	
	--userid none
	--password none

 for example:

    stdoutsub topic/of/interest --host iot.eclipse.org

*/
#include <stdio.h>
#include "MQTTClient.h"

#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>


typedef unsigned int __u32;

#define SWAP_LONG(x) \
    ((__u32)( \
        (((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
        (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
        (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
		(((__u32)(x) & (__u32)0xff000000UL) >> 24) ))


volatile int toStop = 0;


void usage()
{
	printf("MQTT stdout subscriber\n");
	printf("Usage: stdoutsub topicname <options>, where options are:\n");
	printf("  --host <hostname> (default is localhost)\n");
	printf("  --port <port> (default is 1883)\n");
	printf("  --qos <qos> (default is 2)\n");
	printf("  --delimiter <delim> (default is \\n)\n");
	printf("  --clientid <clientid> (default is hostname+timestamp)\n");
	printf("  --username none\n");
	printf("  --password none\n");
	printf("  --showtopics <on or off> (default is on if the topic has a wildcard, else off)\n");
	exit(-1);
}


void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}


struct opts_struct
{
	char* clientid;
	int nodelimiter;
	char* delimiter;
	enum QoS qos;
	char* username;
	char* password;
	char* host;
	int port;
	int showtopics;
} opts =
{
//	(char*)"stdout-subscriber", 0, (char*)"\n", QOS2, NULL, NULL, (char*)"localhost", 1883, 0
	(char*)"stdout-subscriber", 0, (char*)"\n", QOS0, NULL, NULL, (char*)"183.230.40.39", 6002, 1
};

const char *pathname = NULL;

void getopts(int argc, char** argv)
{
	int count = 2;
	
	while (count < argc)
	{
		if (strcmp(argv[count], "--qos") == 0)
		{
			if (++count < argc)
			{
				if (strcmp(argv[count], "0") == 0)
					opts.qos = QOS0;
				else if (strcmp(argv[count], "1") == 0)
					opts.qos = QOS1;
				else if (strcmp(argv[count], "2") == 0)
					opts.qos = QOS2;
				else
					usage();
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--host") == 0)
		{
			if (++count < argc)
				opts.host = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--port") == 0)
		{
			if (++count < argc)
				opts.port = atoi(argv[count]);
			else
				usage();
		}
		else if (strcmp(argv[count], "--clientid") == 0)
		{
			if (++count < argc)
				opts.clientid = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--username") == 0)
		{
			if (++count < argc)
				opts.username = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--password") == 0)
		{
			if (++count < argc)
				opts.password = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--delimiter") == 0)
		{
			if (++count < argc)
				opts.delimiter = argv[count];
			else
				opts.nodelimiter = 1;
		}
		else if (strcmp(argv[count], "--file") == 0)
		{
			if (++count < argc) {
				pathname = argv[count];
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--showtopics") == 0)
		{
			if (++count < argc)
			{
				if (strcmp(argv[count], "on") == 0)
					opts.showtopics = 1;
				else if (strcmp(argv[count], "off") == 0)
					opts.showtopics = 0;
				else
					usage();
			}
			else
				usage();
		}
		count++;
	}
	
}


void messageArrived(MessageData* md)
{
	MQTTMessage* message = md->message;

	if (opts.showtopics)
//		printf("%.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
		printf("%.*s\t: %dBytes\n", md->topicName->lenstring.len, md->topicName->lenstring.data, (int)message->payloadlen);

#if 0
	if (opts.nodelimiter)
		printf("%.*s", (int)message->payloadlen, (char*)message->payload);
	else
		printf("%.*s%s", (int)message->payloadlen, (char*)message->payload, opts.delimiter);
#endif

	fflush(stdout);
}

int fd;
void* remap_file(const char *file, unsigned int *len)
{
	void *ptr;
	struct stat sbuf;

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		printf ("Can't open %s: %s\n", file, strerror(errno));
		exit (1);
	}

	if (fstat(fd, &sbuf) < 0) {
		printf ("Can't stat %s: %s\n", file, strerror(errno));
		exit (1);
	}

	ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ((caddr_t)ptr == (caddr_t)-1) {
		printf ("Can't read %s: %s\n",file, strerror(errno));
		exit (1);
	}

	*len = sbuf.st_size;

	return ptr;
}

void unremap_file(unsigned char *ptr, unsigned int size)
{
	(void) munmap((void *)ptr, size);
	(void) close (fd);
}


/* the onenet platform only support 64KB (65536-1 = 65535B) */
#define BUFSIZE	5*1024*1024
#if 0
unsigned char buf[BUFSIZE];
unsigned char readbuf[BUFSIZE];
#endif

int main(int argc, char** argv)
{
#if 1
	unsigned char *buf = malloc(BUFSIZE);
	unsigned char *readbuf = malloc(BUFSIZE);
#endif

	int rc = 0;
	if (argc < 2)
		usage();
	
	char* topic = argv[1];

	if (strchr(topic, '#') || strchr(topic, '+'))
		opts.showtopics = 1;
	if (opts.showtopics)
		printf("topic is %s\n", topic);

	getopts(argc, argv);	

	Network n;
	MQTTClient c;

	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);

	NetworkInit(&n);
	NetworkConnect(&n, opts.host, opts.port);
	MQTTClientInit(&c, &n, 1000, buf, BUFSIZE, readbuf, BUFSIZE);

 
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
	data.willFlag = 0;
	data.MQTTVersion = 4;
	data.clientID.cstring = opts.clientid;
	data.username.cstring = opts.username;
	data.password.cstring = opts.password;

	data.keepAliveInterval = 120;
	data.cleansession = 1;
	printf("Connecting to %s %d\n", opts.host, opts.port);
	
	rc = MQTTConnect(&c, &data);
	printf("Connected %d\n", rc);
    
	int count = 0;


#define JSON_BUFSIZ		256
	unsigned char *jsonbuf = malloc(JSON_BUFSIZ);
	uint16_t jsize;

	memset(jsonbuf, 0, JSON_BUFSIZ);
	char *name = "image-name";
	char *time = "2017-09-05 22:44:55";
	char *desc = "imageID";

	strcat (jsonbuf, "{\"ds_id\":\"");
	strcat (jsonbuf, name);
	strcat (jsonbuf, "\",\"at\":\"");
	strcat (jsonbuf, time);
	strcat (jsonbuf, "\",\"desc\":\"");
	strcat (jsonbuf, desc);
	strcat (jsonbuf, "\"}");

	jsize = strlen(jsonbuf);
printf("Json: %d\n%s\n", jsize, jsonbuf);

	unsigned int size, big;
	void* ptr;
	if (pathname != NULL) {
		ptr = remap_file(pathname, &size);
	}

	big = SWAP_LONG(size);

printf("File size: %d\n", size);
printf("Big  size: %x\n", big);


	unsigned char *packet  = malloc(jsize + size + 7);
	unsigned char *p = (unsigned char *)packet;

	*p++ = 2;
	*p++ = (jsize & 0x0000ff00UL) >> 8;
	*p++ = (jsize & 0x000000ffUL);
	memcpy(p, jsonbuf, jsize);
	p += jsize;
	memcpy(p, &big, 4);
	p += 4;
	memcpy(p, ptr, size);

#if 1
	char subtopic[64];
	memset(subtopic, 0, sizeof(subtopic));
	sprintf(subtopic, "/%s/%s", opts.clientid, name);

	printf("Subscribing to %s\n", topic);
	if (MQTTSubscribe(&c, topic, opts.qos, messageArrived) != 0) {
		printf("Subscribed error: %d\n", rc);
	}
#endif

	while (!toStop)
	{
		MQTTMessage message;

		message.qos = 0;
		message.retained = 0;
		message.payload = packet;
		message.payloadlen = size + jsize + 7;

		if ((rc = MQTTPublish(&c, "$dp", &message)) != 0)
			printf("Return code from MQTT publish is %d\n", rc);

		MQTTYield(&c, 5000);
	}

#if 0
	while (!toStop)
	{
		MQTTMessage message;
		char payload[30];

		message.qos = 1;
		message.retained = 0;
		message.payload = payload;
		sprintf(payload, "message number %d", count++);
		message.payloadlen = strlen(payload);

		if ((rc = MQTTPublish(&c, topic, &message)) != 0)
			printf("Return code from MQTT publish is %d\n", rc);
		printf("Published: %s\n", topic);

		MQTTYield(&c, 3000);
	}
#endif
	
	printf("Stopping\n");

	MQTTDisconnect(&c);
//	n.disconnect(&n);

	return 0;
}


