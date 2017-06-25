#include "mainServer.h"

const char* test_page = "<html><body>Test Page.</body></html>";
const char* unknown_page = "<html><body>Unknown Page.</body></html>";
std::mutex outfile_mutex;  // protects g_i

inline uint32_t unix_timestamp() {
  time_t t = std::time(0);
  uint32_t now = static_cast<uint32_t> (t);
  return now;
}

inline int authorised_connection(struct MHD_Connection* connection){
	std::cout<<"checking if authorised\n";
	bool fail;
	char* pass = NULL;
	char* user = MHD_basic_auth_get_username_password(connection, &pass);
	fail = ( (user == NULL) ||
				 (0 != strcmp (user, config::HTTPSERVER_USER)) ||
				 (0 != strcmp (pass, config::HTTPSERVER_PASS)) );  
	if (user != NULL) delete user;
	if (pass != NULL) delete pass;

	std::cout<<"user: "<<user<<"\n";
	std::cout<<"pass: "<<pass<<"\n";

	std::cout<<"fail: "<<fail<<"\n";
	return !fail ; //FIXME TODO SECURITY RISK
	//return true;
}


int answer_to_connection(void* cls,struct MHD_Connection* connection, const char* url,
		                     const char* method, const char* version, const char* upload_data,
		                     size_t* upload_data_size, void** con_cls) {
	std::cout<<"AWNSERING\n";
  int ret;
  struct MHD_Response *response;	
	std::ofstream* outfile;
	fromVoidArr(cls, outfile);

 	//if first time connection return 
	if(*con_cls == NULL){
		*con_cls = connection;
		return MHD_YES;
	}
	//correct password, repond dependig on url
	if(authorised_connection(connection)){
		std::cout<<"VERIFIED\n";

		if(strcmp(method, MHD_HTTP_METHOD_GET) == 0){
			std::cout<<"PROTOCOL OK\n";	
			//present diffrent pages to diffrent url's
			if(strcmp(url, "/putData") == 0){
				response = MHD_create_response_from_buffer(strlen (test_page), 
				(void *) test_page, MHD_RESPMEM_PERSISTENT);
				std::lock_guard<std::mutex> lock(outfile_mutex);
				(*outfile)<<"test\n"; //std::to_string(unix_timestamp())+
			}
			else if(strcmp(url, "/test") == 0){
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
		std::cout<<"NOT VERIFIED\n";
		const char* page = "<html><body>Incorrect password.</body></html>";	
		response = MHD_create_response_from_buffer(strlen (page), (void*) page, 
							 MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_basic_auth_fail_response(connection, "test", response);	
	}
	
	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response); //free memory of the respons
  return ret;
}

//used by load_file to find out the file size
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


void* toVoidArr(std::ofstream* outfile){
	void* arrayOfPointers[1];

	//convert arguments
	arrayOfPointers[0] = (void*)outfile;

	return (void*)arrayOfPointers;																		 
}

inline void fromVoidArr(void* cls, std::ofstream*& outfile){
	void** arrayOfPointers;
	void* element1;

	//convert arguments back (hope this optimises well),
	arrayOfPointers = (void**)cls;
	element1 = (void*)*(arrayOfPointers+0);
	outfile = (std::ofstream*)element1;
	return;																		 
}
