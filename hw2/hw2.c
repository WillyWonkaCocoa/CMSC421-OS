/* File: hw2.c
 * Author: William Gao
 * Email: wgao1@umbc.edu
 * Last Edited: 09-16-2017
 * Description: 
 */

/*
 * Question 1: It is necessary to kill and reap child processes to avoid creating orphans. 
 * Describing the consequences of having orphans on your system. 

 * Question 2: Suppose the user creates two children processes, 
 * by selecting the first menu option twice. The first child process will have three 
 * opened file descriptors. Why does the second child have four descriptors instead of three? 
 */

int main(int argc, char *argv[]){
  //if there is only 1 command line argument
  if(argc == 1){ //run in parent mode

  }else{

  }
  
  return 0;
}

/* this functin displays the following menu and returns the user's choice
 * repeated until valid user input
 * Main Menu:
 * 0. Spawn a child
 * 1. Kill a child
 * 2. Kill a random child
 * 3. End program
 */
int showMenu(){

}
