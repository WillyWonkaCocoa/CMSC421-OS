/*
* File: mastermind-test2.c
 * UMBC Email: wgao1@umbc.edu
 * Author: William Gao
 * OS Fall 2017 - Jason Tang - Proj 1
 * contains unit tests for mastermind.c

commands to test normal functionality
insmod mastermind.ko 
./mastermind-test random

commands to test random functinality - extra credit !!!
insmod mastermind.ko random_code=Y
./mastermind-test random

 write beyond max
 read beyond max
 read more than the max bytes allowed
 read more bytes than game status has
 not using newline or non "start"
 too long of a guess (1000000)
 values out of range 
 
*/

#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>

static unsigned int test_passed = 0;
static unsigned int test_failed = 0;
static int ret;
static char *mm = "/dev/mm";
static char *mm_ctl = "/dev/mm_ctl";
static char *start = "start";
static char *quit = "quit";
static char buf[80];

void happy_path_test(void); /* valid number of black and white pegs */
void mmap_test(void); /* correct user_view contents */
void invalid_entry_test(void); /* write invalid argument to /dev/mm_ctl */
void negative_entry_test(void); /* write negative numbers to /dev/mm */
void out_of_range_entry_test(void); /* write numbers outside of 0 - 5 to /dev/mm */
void oveflow_write_test(void); /* write a string longer than NUM_PEGS to /dev/mm */
void overflow_read_test(void); /* tries to read more bytes than valid from /dev/mm */
void guess_with_no_game(void); /* writes guess to /dev/mm with no active game */

void random_test(void); /* target code does not equal 0012 */

int main(int argc, char *argv[]) {
  memset(buf, 0, 80);
  if(argc > 1){
    /* random target code */
    random_test();
  }else{
    /* target code set to 0012 */
    mmap_test();
    happy_path_test();
  }
  
  invalid_entry_test();
  negative_entry_test();
  out_of_range_entry_test();
  oveflow_write_test();
  overflow_read_test();
  guess_with_no_game();
  
  fprintf(stdout, "%d test(s) passed, %d test(s) failed\n",
	  test_passed, test_failed);
  
  return 0;
}

/* validates number of black and white pegs with a target code of 0012 */
void happy_path_test(void){
  int fd_mm = open(mm, O_RDWR);
  char *guess = "Guess 1: 0 black peg(s), 4 white peg(s)\n";
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }

  if(write(fd_mm_ctl, start, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm, "1200", 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  if (read(fd_mm, buf, 41) < 0){
    perror("read");
    exit(EXIT_FAILURE);
  }

  ret = strcmp(guess, buf);
   
  if(ret == 0){
    fprintf(stdout, "+ happy path test passed\n");
    test_passed++;
  }else{
    fprintf(stdout, "- happy path test failed\n");
    test_failed++;
  }

  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  close(fd_mm);
  close(fd_mm_ctl);
}

/* validates mmap contents with target code of 0012 */
void mmap_test(void){
  char * expected_output = "Guess 1: 0022  | B3 W0\nGuess 2: 2010  | B2 W2\nGuess 3: 5432  | B1 W0\n";
  int fd_mm = open(mm, O_RDWR);
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }

  if(write(fd_mm_ctl, start, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm, "0022", 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }

  if(write(fd_mm, "2010", 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }

  if(write(fd_mm, "5432", 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  void *mmap_buf = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_PRIVATE, fd_mm, 0);
  if(mmap_buf == MAP_FAILED){
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  char *string = (char *)mmap_buf;
  
  ret = strcmp(expected_output, string);
   
  if(ret == 0){
    fprintf(stdout, "+ mmap test passed\n");
    test_passed++;
  }else{
    fprintf(stdout, "- mmap test failed\n");
    test_failed++;
  }
  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }

  munmap(NULL, PAGE_SIZE);
  
  close(fd_mm);
  close(fd_mm_ctl);
}

/* enters invalid argument to /dev/mm_ctl */
void invalid_entry_test(void){
  int fd_mm = open(mm, O_RDWR);
  char *invalid_arg1 = "start\n";
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm_ctl, invalid_arg1, 7) < 0){
    if(errno == EINVAL){
      fprintf(stdout, "+ invalid entry to mm_ctl test passed\n");
      test_passed++;
    }
  }else{
    fprintf(stdout, "- invalid entry to mm_ctl test failed\n");
    test_failed++;
  }
  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  close(fd_mm);
  close(fd_mm_ctl);
}

/* enters negative numbers to /dev/mm */
void negative_entry_test(void){
  char * negative_num = "-1201";

  int fd_mm = open(mm, O_RDWR);
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm_ctl, start, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm, negative_num, 4) < 0){
    if(errno == EINVAL){
      fprintf(stdout, "+ negative number to mm test passed\n");
      test_passed++;
    }
  }else{
    fprintf(stdout, "- negative number to mm test failed\n");
    test_failed++;
  }
  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  close(fd_mm);
  close(fd_mm_ctl);
}

/* enters numbers not 0 - 5 to /dev/mm */
void out_of_range_entry_test(void){
  char *out_of_range = "9270";
  int fd_mm = open(mm, O_RDWR);
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }

  if(write(fd_mm_ctl, start, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm, out_of_range, 4) < 0){
    if(errno == EINVAL){
      fprintf(stdout, "+ out of range entry to mm test passed\n");
      test_passed++;
    }
  }else{
    fprintf(stdout, "- out of range entry to mm test failed\n");
    test_failed++;
  }
  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  close(fd_mm);
  close(fd_mm_ctl);
}

/* enters string longer than NUM_PEGS to /dev/mm */
void oveflow_write_test(void){
  int fd_mm = open(mm, O_RDWR);
  char *overflow = "123483292819";
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm_ctl, start, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm, overflow, 12) < 0){
    if(errno == EINVAL){
      fprintf(stdout, "- overflow write to mm test failed\n");
      test_failed++;
    }
  }else{
    fprintf(stdout, "+ overflow write to mm test passed\n");
    test_passed++;
  }
  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
   }
   
  close(fd_mm);
  close(fd_mm_ctl);  
}

/* tries to read more than available valid bytes from /dev/mm */
void overflow_read_test(void){
  int fd_mm = open(mm, O_RDWR);
 if(fd_mm < 0){
   perror("open");
   exit(EXIT_FAILURE);
 }
 
 int fd_mm_ctl = open(mm_ctl, O_WRONLY);
 if(fd_mm_ctl < 0){
   perror("open");
   exit(EXIT_FAILURE);
 }
 
 if(write(fd_mm_ctl, start, 4) < 0){
   perror("write");
   exit(EXIT_FAILURE);
 }
 
 ret = read(fd_mm, buf, 20);
 if (ret < 0){
   perror("read");
   exit(EXIT_FAILURE);
 }
 
 if(errno == EINVAL) {
   fprintf(stdout, "+ overflow read from mm test passed\n");
   test_passed++;
 }else{
   fprintf(stdout, "- overflow read from mm test failed\n");
   test_failed++;
 }

  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }
 
 close(fd_mm);
 close(fd_mm_ctl);
}

/* writes guess to /dev/mm with no active game */
void guess_with_no_game(void){
  char *guess = "2100";
  int fd_mm = open(mm, O_RDWR);
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  if(write(fd_mm, guess, 4) < 0){
    if(errno == EINVAL){
      fprintf(stdout, "+ guess with no game to mm test passed\n");
      test_passed++;
    }
  }else{
    fprintf(stdout, "- guess with no game to mm test failed\n");
    test_failed++;
  }
  
  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
   }

  close(fd_mm);
  close(fd_mm_ctl);
}

/* validates target code is not 0012 */
void random_test(void){
  char *game_over = "Game over. The code was 0012.\n";
  int fd_mm = open(mm, O_RDWR);
  if(fd_mm < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }
  
  int fd_mm_ctl = open(mm_ctl, O_WRONLY);
  if(fd_mm_ctl < 0){
    perror("open");
    exit(EXIT_FAILURE);
  }

  if(write(fd_mm_ctl, start, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }

  if(write(fd_mm_ctl, quit, 4) < 0){
    perror("write");
    exit(EXIT_FAILURE);
  }

  if (read(fd_mm, buf, 31) < 0){
    perror("read");
    exit(EXIT_FAILURE);
  }

  ret = strcmp(game_over, buf);
  
  if(ret == 0){
    fprintf(stdout, "- random test failed\n");
    test_failed++;
  }else{
     fprintf(stdout, "+ random test passed\n");
    test_passed++;
  }
  
  close(fd_mm);
  close(fd_mm_ctl);
}
