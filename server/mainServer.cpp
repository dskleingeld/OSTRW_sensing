#include "mainServer.h"

const char* test_page = "<html><body>Test Page.</body></html>";
const char* unknown_page = "<html><body>Unknown Page.</body></html>";


const char *askpage = "<html><body>\
                       What's your name, Sir?<br>\
                       <form action=\"/namepost\" method=\"post\">\
                       <input name=\"name\" type=\"text\">\
                       <input type=\"submit\" value=\" Send \"></form>\
                       </body></html>";

std::mutex outfile_mutex;  // protects g_i

inline uint32_t unix_timestamp() {
  time_t t = std::time(0);
  uint32_t now = static_cast<uint32_t> (t);
  return now;
}

inline int authorised_connection(struct MHD_Connection* connection){
  bool fail = true;
  char* pass = NULL;
  char* user = MHD_basic_auth_get_username_password(connection, &pass);
  fail = ( (user == NULL) ||
         (0 != strcmp(user, config::HTTPSERVER_USER)) ||
         (0 != strcmp(pass, config::HTTPSERVER_PASS)) );  
  if (user != NULL) delete user;
  if (pass != NULL) delete pass;

  return !fail ; //FIXME TODO SECURITY RISK
  //return true;
}


int answer_to_connection(void* cls,struct MHD_Connection* connection, const char* url,
                         const char* method, const char* version, const char* upload_data,
                         size_t* upload_data_size, void** con_cls) {
  int ret;
  struct MHD_Response* response;  
	struct connection_info_struct* con_info;

//  std::ofstream* outfile;
//  fromVoidArr(cls, outfile);

  //if start of connection, remember connection and store usefull info about
	//connection if post create post processor.
  if (NULL == *con_cls) {
		con_info = new connection_info_struct;
 		con_info->answerstring = NULL;
		
		//set up post processor if post request
		if(0 == strcmp (method, MHD_HTTP_METHOD_POST)){
			con_info->postprocessor = MHD_create_post_processor(connection, 
			config::POSTBUFFERSIZE, iterate_post, (void *) con_info);
			con_info->connectiontype = POST;
		}
		//als geen post request set connectiontype to GET
		else con_info->connectiontype = GET;
		
		//in all cases make sure con_cls has a value 
		*con_cls = (void*)con_info;
    return MHD_YES;
  }
  
  //correct password, repond dependig on url
  if (authorised_connection(connection)){
    
    if (0 == strcmp (method, "GET")){
      //create diffrent pages (responses) to different url's
      if (strcmp(url, "/askpage") == 0) {
        response = MHD_create_response_from_buffer(strlen (askpage), 
        (void *) askpage, MHD_RESPMEM_PERSISTENT);
//        std::lock_guard<std::mutex> lock(outfile_mutex);
//        (*outfile)<<"test\n"; //std::to_string(unix_timestamp())+
      }
      else if (strcmp(url, "/test") == 0) {
        response = MHD_create_response_from_buffer(strlen (test_page), 
        (void *) test_page, MHD_RESPMEM_PERSISTENT);      
      }
      else {
        response = MHD_create_response_from_buffer(strlen (unknown_page), 
        (void *) unknown_page, MHD_RESPMEM_PERSISTENT);          
      }
    //prepare respons to be send to server
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    }
    
    else if (0 == strcmp (method, "POST")) {
			std::cout<<"inside POST\n"
			<<"upload_data_size: "<<*upload_data_size<<"\n";      
			struct connection_info_struct* con_info = (connection_info_struct*)*con_cls;

      if (*upload_data_size != 0)	{
				
        MHD_post_process(con_info->postprocessor, upload_data,
                         *upload_data_size);
        *upload_data_size = 0;
				std::cout<<"done with POST processor\n";   
        return MHD_YES;
      }
			else if (NULL != con_info->answerstring){
				response = MHD_create_response_from_buffer(strlen (unknown_page),
        (void *) unknown_page, MHD_RESPMEM_PERSISTENT);  
				ret = MHD_queue_response(connection, MHD_HTTP_OK, response); 
			
			}
    }
    
  //incorrect password, present go away page
  }
  else {
    const char* page = "<html><body>Incorrect password.</body></html>";  
    response = MHD_create_response_from_buffer(strlen (page), (void*) page, 
	       MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_basic_auth_fail_response(connection, "test", response);  
  }
  
  MHD_destroy_response(response); //free memory of the respons
  return ret;
}

static int iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
												const char *filename, const char *content_type,
												const char *transfer_encoding, const char *data, 
												uint64_t off, size_t size) {
	
	std::cout<<"iterating post\n";

	char answerstring[config::MAXANSWERSIZE];
  struct connection_info_struct* con_info = (connection_info_struct*)coninfo_cls;

  if (0 == strcmp (key, "name")) {
    if ((size > 0) && (size <= config::MAXNAMESIZE)) {
			std::cout<<"data: "<<data<<"\n";

      con_info->answerstring = "test";      
    } 
    else con_info->answerstring = NULL;    
		return MHD_NO;
  }
  return MHD_YES;
}


void request_completed(void *cls, struct MHD_Connection *connection, 
     		        			 void **con_cls, enum MHD_RequestTerminationCode toe) {

  struct connection_info_struct* con_info = (connection_info_struct*)*con_cls;

  if (NULL == con_info) return;
	//do cleanup for post if the con type was a post
  if (con_info->connectiontype == POST)
    {
      MHD_destroy_post_processor (con_info->postprocessor);        
      if (con_info->answerstring) delete con_info->answerstring;
    }
  
  delete con_info;
  *con_cls = NULL;   
}





//used by load_file to find out the file size
long get_file_size(const char *filename) {
  FILE *fp;

  fp = fopen (filename, "rb");
  if (fp) {
    long size;

    if ((0 != fseek (fp, 0, SEEK_END)) || (-1 == (size = ftell (fp))))
      size = 0;

    fclose(fp);

    return size;
  }
  else
    return 0;
}

//used to load the key files into memory
char* load_file(const char *filename) {
  FILE *fp;
  char* buffer;
  unsigned long size;

  size = get_file_size(filename);
  if (0 == size)
    return NULL;

  fp = fopen(filename, "rb");
  if (!fp)
    return NULL;

  buffer = (char*)malloc(size + 1);
  if (!buffer) {
      fclose (fp);
      return NULL;
  }
  buffer[size] = '\0';

  if (size != fread(buffer, 1, size, fp)) {
      free(buffer);
      buffer = NULL;
  }

  fclose(fp);
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
