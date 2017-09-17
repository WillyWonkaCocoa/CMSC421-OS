/*File: hw1.c
 *Author: William Gao
 *Email: wgao1@umbc.edu
 *Section: Jason Tang
 *CMSC421 OS - Fall 2017
 *Description: this program takes a command line argument: a device name
 * Program exits if zero or multiple device names are given
 * if the device name is found in /proc/ioports, all port number(s) 
 * assigned to it will be outputed. If it's not found a message will be printed.
 * if the device name is found in /proc/iomem, all port number(s)
 * assigned to it will be outputed. If it's not found a message will be printed.
 */

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
int chars_per_line = 100;
// it is assumed a line will have 100 characters or less

//compile using the followin flags:
// gcc --std=c99 -Wall -O2 -o hw1 hw1.c

int main(int argc, char *argv[]){
  FILE *ioports;
  FILE *iomem;
  char tempString[chars_per_line];
  char colon[] = ":";
  
  if(argc <= 1){
    printf("No device given.\n");
  }else if(argc > 2){
    printf("Multiple devices given.\n");
  }else{
    char *stringToSearch = argv[1];
    int num_in_ioports = 0;
    int num_in_iomem = 0;

    while(isspace((unsigned char)*stringToSearch)){
      //look for leading whitespace
      stringToSearch++;
    }
    
    char *end = stringToSearch + strlen(stringToSearch) - 1;
    while(end > stringToSearch && isspace((unsigned char)*end)){
      //get rid of trailing whitespace
      end--;
    }
    *(end+1) = 0; //assign null character
    
    ioports = fopen("/proc/ioports","r");
    if(ioports){ //if ioports is opened successfully
      printf("ioports:\n");
      
      while(feof(ioports) == 0){
	
	if(fgets(tempString, chars_per_line, ioports) != NULL){
	  // if the line is successfully read
	  if(strstr(tempString, stringToSearch) != NULL){
	    // if the line contains the device name  
	    char *address = tempString;

	    //shorten string to only memory address
	    end = strstr(tempString, colon);
	    (*end) = 0;

	    //trim leading whitespaces
	    while(isspace((unsigned char)*address)){
	      address++;
	    }
	    printf("%s\n", address);
	    num_in_ioports++;
	  }
	}
      }
    }
    if(num_in_ioports == 0){
      printf("No matching entry found in /proc/ioports\n");
    }
    
    
    iomem = fopen("/proc/iomem", "r");
    if(iomem){ //if iomem is opened successfully
      printf("iomem:\n");

      while(feof(iomem) == 0){
	
	if(fgets(tempString, chars_per_line, iomem) != NULL ){
	  // if the line is successfully read
	  if(strstr(tempString, stringToSearch) != NULL){
	    // if the device name is found in this line
	    char *address = tempString;

	    //shorten string to only memory address
	    end = strstr(tempString, colon);
	    (*end) = 0;

	    //trim leading whitespaces
	    while(isspace((unsigned char)*address)){
	      address++;
	    }
       
	    printf("%s\n", address);
	    num_in_iomem++;
	  }
	}
      }
    }
    if(num_in_iomem == 0){
	printf("No matching entry found in /proc/iomem\n");
    } 
  }
  
  return 0;
}
