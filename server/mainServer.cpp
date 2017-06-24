#include "mainServer.h"
#include <string>


inline int authorised_connection(struct MHD_Connection* connection){
	int fail;
	char* pass = NULL;
	char* user = MHD_basic_auth_get_username_password(connection, &pass);
	fail = ( (user == NULL) ||
				 (0 != strcmp (user, config::HTTPSERVER_USER)) ||
				 (0 != strcmp (pass, config::HTTPSERVER_PASS)) );  
	if (user != NULL) free (user);
	if (pass != NULL) free (pass);
	return !fail;
}


int answer_to_connection(void* cls,struct MHD_Connection* connection, const char* url,
		                     const char* method, const char* version, const char* upload_data,
		                     size_t* upload_data_size, void** con_cls) {
  int ret;  
  int fail;
  struct MHD_Response *response;	

 	//if first time connection return 
	if(con_cls == NULL){
		*con_cls = connection;
		return MHD_YES;
	}
	//correct password, repond dependig on url
	if(authorised_connection(connection)){

		if(method == "GET"){			
			//present diffrent pages to diffrent url's
			if(*url == "/test"){
				response = MHD_create_response_from_buffer(strlen (test_page), 
				(void *) test_page, MHD_RESPMEM_PERSISTENT);			
			}
			else{
				response = MHD_create_response_from_buffer(strlen (unknown_page), 
				(void *) unknown_page, MHD_RESPMEM_PERSISTENT);					
			}
		}
		//unrecognised or unallowed protocol, break connection
		else{ return MHD_NO;}
	}
	else{//incorrect password, present go away page
		const char* page = "<html><body>Incorrect password.</body></html>";	
		response = MHD_create_response_from_buffer(strlen (page), (void*) page, 
							 MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_basic_auth_fail_response(connection, "test",response);	
	}
	
	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response); //free memory of the respons
  return ret;
}

//used by load_file to find out the file size
//FIXME was static 
long get_file_size (const char *filename)
{
  FILE *fp;

  fp = fopen (filename, "rb");
  if (fp)
    {
      long size;

      if ((0 != fseek (fp, 0, SEEK_END)) || (-1 == (size = ftell (fp))))
        size = 0;

      fclose (fp);

      return size;
    }
  else
    return 0;
}

//used to load the key files into memory
//FIXME was static and not used wanted to get rid of warning
char* load_file (const char *filename)
{
  FILE *fp;
  char* buffer;
  unsigned long size;

  size = get_file_size(filename);
  if (0 == size)
    return NULL;

  fp = fopen(filename, "rb");
  if (! fp)
    return NULL;

  buffer = (char*)malloc(size + 1);
  if (! buffer)
    {
      fclose (fp);
      return NULL;
    }
  buffer[size] = '\0';

  if (size != fread (buffer, 1, size, fp))
    {
      free (buffer);
      buffer = NULL;
    }

  fclose (fp);
  return buffer;
}
