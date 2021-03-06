/* 
 * File: mastermind.c
 * UMBC Email: wgao1@umbc.edu
 * Author: William Gao
 * OS Fall 2017 - Jason Tang - Proj 1
 * This file contains the implementation of mastermind with 4 pegs, 
 * each between the values of 0 - 5 (inclusive)
 * user writes "start" or "quit" to /dev/mm_ctl to start or end a game
 * user writes to guess to /dev/mm to guess a possible code combination
 * user reads from /dev/mm the number of black and white pegs in their guess
 * extra credit is implemented, add the following to generate random target code
 * >> insmod mastermind.ko random_code=Y
 */

/*
 * This file uses kernel-doc style comments, which is similar to
 * Javadoc and Doxygen-style comments.  See
 * ~/linux/Documentation/kernel-doc-nano-HOWTO.txt for details.
 */

/*
 * Getting compilation warnings?  The Linux kernel is written against
 * C89, which means:
 *  - No // comments, and
 *  - All variables must be declared at the top of functions.
 * Read ~/linux/Documentation/CodingStyle to ensure your project
 * compiles without warnings.
 */

#define pr_fmt(fmt) "MM: " fmt

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#define NUM_PEGS 4
#define NUM_COLORS 6

/** true if command line specifies a randomized target code*/
static bool random_code = false;
module_param(random_code, bool, 0644);
MODULE_PARM_DESC(random_code,
		 "boolean determines whether target code is randomized\n");
/** true if user is in the middle of a game */
static bool game_active;

/** code that player is trying to guess */
static int target_code[NUM_PEGS];

/** tracks number of guesses user has made */
static unsigned num_guesses;

/** current status of the game */
static char game_status[80];

/** buffer that logs guesses for the current game */
static char *user_view;

static ssize_t mm_read(struct file *filp,
		       char __user * ubuf, size_t count, loff_t * ppos);

static ssize_t mm_write(struct file *filp,
			const char __user * ubuf, size_t count, loff_t * ppos);

static ssize_t mm_ctl_write(struct file *filp,
			    const char __user * ubuf,
			    size_t count, loff_t * ppos);

static int mm_mmap(struct file *filp, struct vm_area_struct *vma);

static const struct file_operations fileOp_mm = {
	.read = mm_read,
	.write = mm_write,
	.mmap = mm_mmap,
};

static struct miscdevice device_mm = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mm",
	.fops = &fileOp_mm,
	.mode = 0666,
};

static const struct file_operations fileOp_mm_ctl = {
	.write = mm_ctl_write,
};

static struct miscdevice device_mm_ctl = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mm_ctl",
	.fops = &fileOp_mm_ctl,
	.mode = 0666,
};

/**
 * mm_read() - callback invoked when a process reads from
 * /dev/mm
 * @filp: process's file object that is reading from this device (ignored)
 * @ubuf: destination buffer to store output
 * @count: number of bytes in @ubuf
 * @ppos: file offset (in/out parameter)
 *
 * Write to @ubuf the current status of the game, offset by
 * @ppos. Copy the lesser of @count and (string length of @game_status
 * - *@ppos). Then increment the value pointed to by @ppos by the
 * number of bytes copied. If @ppos is greater than or equal to the
 * length of @game_status, then copy nothing.
 *
 * Return: number of bytes written to @ubuf, or negative on error
 */
static ssize_t mm_read(struct file *filp, char __user * ubuf, size_t count,
		       loff_t * ppos)
{
	unsigned int bytes_copied = count;
	int ret_value;

	if (*ppos >= strlen(game_status)) {
		return 0;
	} else {

		if (bytes_copied > strlen(game_status) - *ppos) {
			bytes_copied = strlen(game_status) - *ppos;
		}

		ret_value =
		  copy_to_user((ubuf), game_status + *ppos, bytes_copied);
		/* ubuf, game_status + *ppos*/
		if (ret_value == 0) {
			*ppos += bytes_copied;

			return bytes_copied;
		} else {
			return -EINVAL;
		}
	}
}

/**
 * mm_write() - callback invoked when a process writes to /dev/mm
 * @filp: process's file object that is reading from this device (ignored)
 * @ubuf: source buffer from user
 * @count: number of bytes in @ubuf
 * @ppos: file offset (ignored)
 *
 * If the user is not currently playing a game, then return -EINVAL.
 *
 * If @count is less than NUM_PEGS, then return -EINVAL. Otherwise,
 * interpret the first NUM_PEGS characters in @ubuf as the user's
 * guess. For each guessed peg, calculate how many are in the correct
 * value and position, and how many are simply the correct value. Then
 * update @num_guesses, @game_status, and @user_view.
 *
 * <em>Caution: @ubuf is NOT a string; it is not necessarily
 * null-terminated.</em> You CANNOT use strcpy() or strlen() on it!
 *
 * Return: @count, or negative on error
 */
static ssize_t mm_write(struct file *filp, const char __user * ubuf,
			size_t count, loff_t * ppos)
{
	size_t i, j;
	int ret_value;
	char user_guess[NUM_PEGS];
	int peg_color[NUM_PEGS];
	char guess_log[24];
	bool valid_input = true;
	int zero = 48;
	int five = 53;
	unsigned int num_black = 0;
	unsigned int num_white = 0;

	memset(user_guess, 0, NUM_PEGS);
	memset(peg_color, 0, NUM_PEGS * 4);	//default value is incorrect value and position

	if (!game_active || count < NUM_PEGS) {
		if (!game_active) {
			pr_err("No active game\n");
		} else {
			pr_err("Not enough numbers entered\n");
		}
		return -EINVAL;
	} else {
		/* check the first four digits are within bounds */
		ret_value = copy_from_user(user_guess, (void *)ubuf, NUM_PEGS);
		for (i = 0; i < NUM_PEGS; i++) {
			if (user_guess[i] < zero || user_guess[i] > five) {
				valid_input = false;
			} else {
				user_guess[i] = user_guess[i] - zero;
			}
		}

		if (valid_input) {
			num_guesses++;

			/* calculate black pegs */
			for (i = 0; i < NUM_PEGS; i++) {
				if (target_code[i] == user_guess[i]) {
					num_black++;
					peg_color[i] = 1;	//set peg to be marked
				}
			}

			/* calculate white pegs */
			for (i = 0; i < NUM_PEGS; i++) {
				for (j = 0; j < NUM_PEGS; j++) {
					if (peg_color[j] == 0) {
						if (target_code[j] ==
						    user_guess[i]) {

							num_white++;
							peg_color[j] = 1;	//set peg to marked
							continue;
						}	/* check if the digits were the same */
					}	/*if peg color has been defined */
				}	/* loop through the target code */
			}	/* loop through all of the user guesses */

			/* update game_status & user_view */
			scnprintf(game_status, 41,
				  "Guess %d: %d black peg(s), %d white peg(s)\n",
				  num_guesses, num_black, num_white);

			scnprintf(guess_log, 24,
				  "Guess %d: %d%d%d%d  | B%d W%d\n",
				  num_guesses, user_guess[0], user_guess[1],
				  user_guess[2], user_guess[3], num_black,
				  num_white);

			strcat(user_view, guess_log);
			pr_err("%s", user_view);
			return count;
		} else {
			return -EINVAL;
		}
	}
}

/**
 * mm_mmap() - callback invoked when a process mmap()s to /dev/mm
 * @filp: process's file object that is mapping to this device (ignored)
 * @vma: virtual memory allocation object containing mmap() request
 *
 * Create a read-only mapping from kernel memory (specifically,
 * @user_view) into user space.
 *
 * Code based upon
 * <a href="http://bloggar.combitech.se/ldc/2015/01/21/mmap-memory-between-kernel-and-userspace/">http://bloggar.combitech.se/ldc/2015/01/21/mmap-memory-between-kernel-and-userspace/</a>
 *
 * You do not need to modify this function.
 *
 * Return: 0 on success, negative on error.
 */
static int mm_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);
	unsigned long page = vmalloc_to_pfn(user_view);
	if (size > PAGE_SIZE)
		return -EIO;
	vma->vm_pgoff = 0;
	vma->vm_page_prot = PAGE_READONLY;
	if (remap_pfn_range(vma, vma->vm_start, page, size, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

/**
 * mm_ctl_write() - callback invoked when a process writes to
 * /dev/mm_ctl
 * @filp: process's file object that is writing to this device (ignored)
 * @ubuf: source buffer from user
 * @count: number of bytes in @ubuf
 * @ppos: file offset (ignored)
 *
 * Copy the contents of @ubuf, up to the lesser of @count and 8 bytes,
 * to a temporary location. Then parse that character array as
 * following:
 *
 *  start - Start a new game. If a game was already in progress, restart it.
 *  quit  - Quit the current game. If no game was in progress, do nothing.
 *
 * If the input is none of the above, then return -EINVAL.
 *
 * <em>Caution: @ubuf is NOT a string; it is not necessarily
 * null-terminated.</em> You CANNOT use strcpy() or strlen() on it!
 *
 * Return: @count, or negative on error
 */
static ssize_t mm_ctl_write(struct file *filp, const char __user * ubuf,
			    size_t count, loff_t * ppos)
{
	char *start = "start";
	char *quit = "quit";
	int ret_value;
	size_t i;
	unsigned int digit;
	unsigned int bytes_copied = 8;
	char user_input[bytes_copied];

	if (count < bytes_copied) {
		bytes_copied = count;
	}

	ret_value = copy_from_user(user_input, (void *)ubuf, bytes_copied);

	if (ret_value != 0) {
		pr_err("Could not read user input to mm_ctl\n");
		return -EINVAL;
	} else {
		if (memcmp(user_input, start, bytes_copied) == 0) {
			/* if user writes start */
			/*
			   set target code to 0012
			   set number of guesses to 0
			   set game_active to true
			   clear contents of user_view
			   set game status message
			 */
			game_active = true;
			num_guesses = 0;
			memset(game_status, 0, 80);
			memset(user_view, 0, 80); /* set to PAGE_SIZE */
			scnprintf(game_status, 15, "Starting game\n");

			if (random_code) {
				for (i = 0; i < NUM_PEGS; i++) {
					get_random_bytes(&digit, sizeof(digit));
					digit = digit % 6;
					target_code[i] = digit;
					pr_err("setting code to %d\n",
					       target_code[i]);
				}
			} else {
				target_code[0] = 0;
				target_code[1] = 0;
				target_code[2] = 1;
				target_code[3] = 2;
			}
			return count;
		} else if (memcmp(user_input, quit, bytes_copied) == 0) {

			if (game_active) {
				/* if user quits an existing game */
				scnprintf(game_status, 31,
					  "Game over. The code was %d%d%d%d.\n",
					  target_code[0], target_code[1],
					  target_code[2], target_code[3]);
				game_active = false;
			}

			return count;
		} else {
			return -EINVAL;
		}
	}
}

/**
 * mastermind_init() - entry point into the Mastermind kernel module
 * Return: 0 on successful initialization, negative on error
 */
static int __init mastermind_init(void)
{
	pr_info("Initializing the game.\n");
	user_view = vmalloc(PAGE_SIZE);
	if (!user_view) {
		pr_err("Could not allocate memory\n");
		return -ENOMEM;
	}

	if (misc_register(&device_mm) != 0) {
		pr_err("Could not register device_mm\n");
		goto device_mm_failure;
	}

	if (misc_register(&device_mm_ctl) != 0) {
		pr_err("Could not register device_mm_ctl\n");
		goto device_mm_ctl_failure;
	}

	memset(game_status, 0, 80);
	memset(user_view, 0, 80); /* set to PAGE_SIZE */
	scnprintf(game_status, 13, "No game yet\n");
	return 0;
	/* use goto statements to handle error & cleanup" */

device_mm_ctl_failure:
	misc_deregister(&device_mm_ctl);
device_mm_failure:
	misc_deregister(&device_mm);
	vfree(user_view);
	pr_info("Freeing resources.\n");
	return -ENODEV;
}

/**
 * mastermind_exit() - called by kernel to clean up resources
 */
static void __exit mastermind_exit(void)
{
	pr_info("Freeing resources.\n");
	vfree(user_view);

	misc_deregister(&device_mm);
	misc_deregister(&device_mm_ctl);
}

module_init(mastermind_init);
module_exit(mastermind_exit);

MODULE_DESCRIPTION("CS421 Mastermind Game");
MODULE_LICENSE("GPL");
