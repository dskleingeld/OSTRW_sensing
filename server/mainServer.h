#ifndef MAINSERVER
#define MAINSERVER

#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iostream>

#include "config.h"

//following this tutorial:
//https://www.gnu.org/software/libmicrohttpd/tutorial.html

/* used by load_file to find out the file size */
long get_file_size (const char *filename);

/* used to load the key files into memory */
char* load_file (const char *filename);

/*inline void convert_Pointers(void* cls);*/
/*inline void convert_Objects(void* cls);*/

/* check if a the password and username are correct */
inline int authorised_connection(struct MHD_Connection* connection);
												 
/* This funct is called when a url is requested, it handles what happens on the
   side. At the end of this function a respons is send to the user */
int answer_to_connection(void* cls,struct MHD_Connection* connection, const char* url,
												 const char* method, const char* version, const char* upload_data,
												 size_t* upload_data_size, void** con_cls);


#endif

