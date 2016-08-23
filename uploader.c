/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/ 
/* <DESC>
 * HTTP Multipart formpost with file upload and two additional parts.
 * </DESC>
 */ 
/* Example code that uploads a file name 'foo' to a remote script that accepts
 * "HTML form based" (as described in RFC1738) uploads using HTTP POST.
 *
 * The imaginary form we'll fill in looks like:
 *
 * <form method="post" enctype="multipart/form-data" action="examplepost.cgi">
 * Enter file: <input type="file" name="sendfile" size="40">
 * Enter file name: <input type="text" name="filename" size="30">
 * <input type="submit" value="send" name="submit">
 * </form>
 *
 * This exact source code has not been verified to work.
 */ 
 
#include <stdio.h>
#include <string.h>
// this is to list all the file 
#include <dirent.h> 
#include <curl/curl.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>

 int sendFile(char * filename) {
   //printf("fullpath %s\n", filename);
   CURL *curl;
   CURLcode res;
   struct curl_httppost *formpost=NULL;
   struct curl_httppost *lastptr=NULL;
   struct curl_slist *headerlist=NULL;
   static const char buf[] = "Expect:";
     curl_global_init(CURL_GLOBAL_ALL);
  
   /* Fill in the file upload field */ 
   curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "sound",
                CURLFORM_FILE, filename,
                CURLFORM_END);
  curl = curl_easy_init();
  /* initialize custom header list (stating that Expect: 100-continue is not
     wanted */ 
  headerlist = curl_slist_append(headerlist, buf);
  if(curl) {
    /* what URL that receives this POST */ 
    curl_easy_setopt(curl, CURLOPT_URL, "https://digital-stethoscope-m18tran.c9users.io/upload");

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
  
    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK){
      //fprintf(stderr, "curl_easy_perform() failed: %s\n",
              //curl_easy_strerror(res));
      return -1;
    }
  
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  
    /* then cleanup the formpost chain */ 
    curl_formfree(formpost);
    /* free slist */ 
    curl_slist_free_all (headerlist);
    return 1;
  }
  return -1;
 }

int main(int argc, char *argv[])
{


  DIR *dir;
  struct dirent *ent;
  
  /* print all the files and directories within directory */
  while (1) { // running continously uploading file
    sleep(10);
    if ((dir = opendir ("records")) != NULL) { // refreshing directory
      while ((ent = readdir (dir)) != NULL) {
         if (strlen(ent->d_name) < 3) {
          
         }
         else {
           char * isWAV = strstr (ent->d_name,".wav");
           if (isWAV == NULL) {
              
           } else {
              // allocate memory
              char filename[strlen(ent->d_name)];
              char lock_filename[strlen(ent->d_name)+1];
              // make a copy of file name
              strncpy(filename,ent->d_name,strlen(ent->d_name)); 
              //make a lock file name
              strncpy(lock_filename,ent->d_name,strlen(ent->d_name)); 
              // find wav position
              char * mustWAV = strstr (lock_filename,".wav");
              strncpy(mustWAV,".lock",5);

              // make a full path
              char lock_fullpath[30];
              sprintf(lock_fullpath,"records/%s", lock_filename);

              struct stat st;
              int result = stat(lock_fullpath, &st);
               if (result == 0) // file exist
               {
             
                
                 
               } else {
                 char fullpath[30];
                 sprintf(fullpath,"records/%s", filename);
                 if (sendFile(fullpath) == 1){ // success sending
            
                   remove(fullpath);
                 }
               }
             
           }
         } 
      }
      closedir (dir);
    } else {
      /* could not open directory */
      perror ("");
      return -1;
    }
  }
    

    // prevemt segmentation fault
  CURL *curl;

    curl = curl_easy_init();

 





  return 0;
}