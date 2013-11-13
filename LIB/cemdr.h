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

#ifndef cemdr__h
#define cemdr__h

#ifdef __cplusplus
extern "C"
{
#endif

#define CEMDR_LOG_FILE "cemdr.log" //default filename to log errors and warnings
#define CEMDR_BUFFER_SIZE 1048576 //default buffer size to hold uncompressed data

#ifndef __ZMQ_H_INCLUDED__
#define __ZMQ_H_INCLUDED__
typedef struct zmq_msg_t {unsigned char _ [32];} zmq_msg_t;
#endif

	//the structure type
	typedef struct
	{
		void *z_context ; 
		void *z_socket ;
		zmq_msg_t z_msg ;
		char *z_server_address ; 
		int z_buffer_size ;
		int z_timeout ;
		int z_received ;

	} Cemdr;

	//pointer to the structure
	typedef Cemdr *PCemdr;

	__declspec(dllexport) void cemdrLog (char *string);
	/*used by cemdr functions, append given string parameter to default log file
	 *if cemdr functions returns errors check CEMDR_LOG_FILE "cemdr.log" for details
	 */
	
	__declspec(dllexport) PCemdr cemdrConnect(char *server_address, int timeout, int buffer_size);
	/*allocate a Cemdr structure and return a valid pointer to it, NULL if error.
	*Parameter list :
	*- server_address : string containing the desired server address in the format : "tcp://relay-us-central-1.eve-emdr.com:8050"
	*- timeout : if 0 the calls to cemdrGetJsonString(...) will wait until a message is received, if set wait for the specified amount of time (in ms)
	*- buffer_size : if 0 use CEMDR_BUFFER_SIZE as max size to handle uncompressed message data, if set use the specified size (in bytes)
	*/
	
	__declspec(dllexport) void cemdrDisconnect(PCemdr cemdr);
	/*given a valid pointer parameter, clean up and free the allocated Cemdr structure.
	*the supplied pointer should be set to null after this function
	*/

	__declspec(dllexport) char *cemdrGetJsonString(PCemdr cemdr);
	/*given a valid pointer parameter, return an allocated string containing data in the JSON format
	*return NULL on errors
	*/

#ifdef __cplusplus
}
#endif

#endif