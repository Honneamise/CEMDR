/*
  Copyright (c) 2013 Alberto Glarey
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

//standard include files
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

#include "zmq.h"//zmq library
#include "zlib.h"//zlib library
#include "cemdr.h"//cemdr library


/******************************/
void cemdrLog (char *string)
{
	FILE *fp = NULL;
	char *msg = NULL ;
	char *time_string = NULL ;

	time_t rawtime ;
	struct tm *timeinfo ;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	time_string=(char *)malloc( 32 *sizeof(char));
	strftime(time_string, 32 ,"[%d-%m-%Y %H:%M:%S]",timeinfo);

	if(string!=NULL)
	{
		msg=(char *)malloc( ( strlen(time_string) + strlen(string) + 2 )*sizeof(char));
		sprintf(msg,"%s:%s",time_string,string);
		msg[strlen(time_string) + strlen(string) +1]='\0';
	}
	else
	{
		msg=(char *)malloc( ( strlen(time_string) +strlen(":NULL")+ 1 )*sizeof(char));
		sprintf(msg,"%s:NULL",time_string);
		msg[strlen(time_string) +strlen(":NULL")]='\0';
	};

	printf("%s",msg);
	fp=fopen(CEMDR_LOG_FILE,"a");
	fprintf(fp,"%s\n",msg);
	fclose(fp);

	free(time_string);
	free(msg);

}

/******************************/
PCemdr cemdrConnect(char *server_address, int timeout, int buffer_size)
{
	PCemdr cemdr = NULL;
	cemdr=(PCemdr) malloc(1*sizeof(Cemdr));

	//Create a new zmq context, return NULL on errors
	cemdr->z_context = zmq_ctx_new();
	if(cemdr->z_context==NULL)
	{
		cemdrLog("cemdrConnect (error) : zmq_ctx_new(...) failed");
		return NULL;
	};

	//Create the zmq socket, return NULL on errors
	cemdr->z_socket = zmq_socket(cemdr->z_context,ZMQ_SUB);
	if(cemdr->z_context==NULL)
	{
		cemdrLog("cemdrConnect (error) : zmq_socket(...) failed");
		return NULL;
	};

	//Disable filtering, return zero if successful, -1 on errors
	if( zmq_setsockopt(cemdr->z_socket,ZMQ_SUBSCRIBE,"",0) != 0 )
	{
		cemdrLog("cemdrConnect (error) : zmq_setsockopt(ZMQ_SUBSCRIBE...) failed");
		return NULL;
	};

	//set user timeout only if != 0
	if( timeout != 0 )
	{
		cemdr->z_timeout = timeout;

		//Set desired timeout, return zero if successful, -1 on errors
		if( zmq_setsockopt(cemdr->z_socket,ZMQ_RCVTIMEO,&cemdr->z_timeout,sizeof(cemdr->z_timeout)) != 0 )
		{
			cemdrLog("cemdrConnect (error) : zmq_setsockopt(ZMQ_RCVTIMEO...) failed");
			return NULL;
		};
	};

	if(server_address!=NULL)
	{
		cemdr->z_server_address=(char *)malloc( (strlen(server_address) + 1) *sizeof(char));
		memcpy(cemdr->z_server_address,server_address,strlen(server_address));
		cemdr->z_server_address[strlen(server_address)]='\0';
	}
	else
	{
		cemdrLog("cemdrConnect (error) : server_address is NULL");
		return NULL;
	};

	//Connect to the desired relay, return zero if successful, -1 on errors
	if ( zmq_connect(cemdr->z_socket,cemdr->z_server_address) != 0 )
	{
		cemdrLog("cemdrConnect (error) : zmq_connect(...) failed");
		return NULL;
	};

	//adjust the buffer size, or use default
	if( buffer_size != 0 )
	{
		cemdr->z_buffer_size = buffer_size;
	}
	else
	{
		cemdr->z_buffer_size = CEMDR_BUFFER_SIZE ;
	};

	//reset the counter
	cemdr->z_received=0;

	return cemdr;
}

/******************************/
void cemdrDisconnect(PCemdr cemdr)
{

	if(cemdr!=NULL)
	{

		//Close the zmq socket, return zero if successful -1 on errors
		if( zmq_close(cemdr->z_socket) !=0 )
		{
			cemdrLog("cemdrDisconnect (warning) : zmq_close(...) failed");
		};

		//Free the zmq context, return zero if successful -1 on errors
		if(  zmq_ctx_destroy(cemdr->z_context) !=0 )
		{
			cemdrLog("cemdrDisconnect (warning) : zmq_ctx_destroy(...) failed");
		};


		if(cemdr->z_server_address!=NULL)
		{
			free(cemdr->z_server_address);
		};

		free(cemdr);
	}
	else
	{
		cemdrLog("cemdrDisconnect (warning) : cemdr is NULL");
	};
}


/******************************/
char *cemdrGetJsonString(PCemdr cemdr)
{
	//The required data buffers and size indicators to hold and expand the received zmq messages
	unsigned char *compressed_msg = NULL;
	int compressed_msg_size = 0;
	unsigned char *uncompressed_msg = NULL;
	int uncompressed_msg_size = 0;
	int err = 0 ;
	char *return_buffer=NULL;

	if(cemdr==NULL)
	{
		cemdrLog("cemdrGetJsonString (error) : cemdr is NULL");
		return NULL;
	};

	//Initialize the  zmq message, return zero if successful, -1 on errors
	if (zmq_msg_init(&cemdr->z_msg) !=0 )
	{
		cemdrLog("cemdrGetJsonString (error) : zmq_msg_init(...) failed");
		return NULL;
	};

	//Receive a zmq message, return the number of bytes received if successful, -1 on errors
	compressed_msg_size = zmq_msg_recv(&cemdr->z_msg,cemdr->z_socket,0);

	switch(compressed_msg_size)
	{
	case -1:
		cemdrLog("cemdrGetJsonString (error) : zmq_msg_recv(...) failed");
		return NULL;
		break;

	case 0:
		cemdrLog("cemdrGetJsonString (warning) : zmq_msg_recv(...) received empty message");
		return NULL;
		break;

	default :
		cemdr->z_received++;
		break;
	};

	//Allocate a buffer and copy the compressed data received in the zmq message
	//The zmq documentation explicitly say :
	//"Never access zmq_msg_t members directly, instead always use the zmq_msg(...) family of functions"
	compressed_msg = (unsigned char *)malloc(compressed_msg_size*sizeof(unsigned char));
	memcpy(compressed_msg,zmq_msg_data(&cemdr->z_msg),compressed_msg_size);

	//Allocate a buffer where to hold the uncompressed data
	uncompressed_msg_size = cemdr->z_buffer_size;
	uncompressed_msg = (unsigned char *)malloc(uncompressed_msg_size*sizeof(unsigned char));

	//The zlib function uncompress(...) return Z_OK if successful, on errors it can return :
	//- Z_BUF_ERROR : The buffer destination buffer was not large enough to hold the uncompressed data
	//- Z_MEM_ERROR : Insufficient memory
	//- Z_DATA_ERROR : The compressed data was corrupted
	err = uncompress(uncompressed_msg,(uLongf *)&uncompressed_msg_size,compressed_msg,compressed_msg_size);
	//NOTE : After the call to this function, uncompressed_msg hold the uncompressed data, uncompressed_msg_size hold the size of uncompressed_msg

	switch(err)
	{
	case Z_BUF_ERROR:
		cemdrLog("cemdrGetJsonString (error) : uncompress(...) failed - buffer too small");
		return NULL;
		break;

	case Z_MEM_ERROR:
		cemdrLog("cemdrGetJsonString (error) : uncompress(...) failed - insufficient memory");
		return NULL;
		break;

	case Z_DATA_ERROR:
		cemdrLog("cemdrGetJsonString (error) : uncompress(...) failed - compressed data corrupted");
		return NULL;
		break;

	case Z_OK:
		//correct decompression, do nothing and proceed
		break;

	default :
		//unknow value , try to continue
		cemdrLog("cemdrGetJsonString (warning) : uncompress(...) unknow error - trying to continue");
		break;
	};


	return_buffer=(char*)malloc( (uncompressed_msg_size+1)*sizeof(char));
	memcpy(return_buffer,uncompressed_msg,uncompressed_msg_size);
	return_buffer[uncompressed_msg_size]='\0';

	//Close the zmq message, return zero if successful, -1 on errors
	if( zmq_msg_close(&cemdr->z_msg) != 0 )
	{
		cemdrLog("cemdrGetJsonString (warning) : zmq_msg_close(...) failed");
	};

	free(compressed_msg);
	free(uncompressed_msg);

	return return_buffer;
}
