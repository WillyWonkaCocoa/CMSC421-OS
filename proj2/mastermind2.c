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

#define pr_fmt(fmt) "mastermind2: " fmt

#include <linux/capability.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
#include <linux/vmalloc.h>

#include "xt_cs421net.h"

/* Copy mm_read(), mm_write(), mm_mmap(), and mm_ctl_write(), along
 * with all of your global variables and helper functions here.
 */
#define NUM_PEGS 4
static int NUM_COLORS = 6;
static int num_game_started = 0;
static int times_target_code_changed = 0;
static int num_invalid_change_attempts = 0;
static int zero_ascii = 48;

struct mm_game {
	bool game_active; /** true if user is in the middle of a game */
	unsigned num_guesses; /** tracks number of guesses user has made */
	char game_status[80]; /** current status of the game */
	char *user_view; /** buffer that logs guesses for the current game */
	kuid_t player_id;
	struct list_head list;
};

static LIST_HEAD(games_list);
static DEFINE_SPINLOCK(lock);

/** true if command line specifies a randomized target code*/
static bool random_code = false;
module_param(random_code, bool, 0644);
MODULE_PARM_DESC(random_code,
		 "boolean determines whether target code is randomized\n");

/** code that player is trying to guess */
static int target_code[NUM_PEGS];

static ssize_t mm_read(struct file *filp,
		       char __user * ubuf, size_t count, loff_t * ppos);

static ssize_t mm_write(struct file *filp,
			const char __user * ubuf, size_t count, loff_t * ppos);

static ssize_t mm_ctl_write(struct file *filp,
			    const char __user * ubuf,
			    size_t count, loff_t * ppos);

static int mm_mmap(struct file *filp, struct vm_area_struct *vma);

static struct mm_game *find_game(kuid_t player);

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
 * find_game() - function to find stats about a specific player
 * @player: user ID passed in via macro current_uid()
 *
 * Search through linked list for node whose UID matches the one passed in. 
 * If one exists, return a pointer to the node. Otherwise, allocate and 
 * return a new node with the passed in UID.
 *
 * Return: a pointer to a node with the same UID as @player, or a newly allocated node with UID set to @player
 */
static struct mm_game *find_game(kuid_t player)
{
	struct mm_game *entry;
	struct mm_game *new_game;
	unsigned long flags;
	spin_lock_irqsave(&lock, flags);
	list_for_each_entry(entry, &games_list, list) {
		if (uid_eq(player, entry->player_id)) {
			spin_unlock_irqrestore(&lock, flags);
			return entry;
		}
	}

	new_game = kzalloc(sizeof(*new_game), GFP_KERNEL);
	if (!new_game) {
		spin_unlock_irqrestore(&lock, flags);
		return NULL;
	}
	new_game->user_view = vmalloc(PAGE_SIZE);
	if (!(new_game->user_view)) {
		spin_unlock_irqrestore(&lock, flags);
		return NULL;
	}

	memset(new_game->user_view, 0, 80);
	memset(new_game->game_status, 0, 80);

	scnprintf(new_game->game_status, 13, "No game yet\n");
	new_game->player_id = player;
	new_game->game_active = false;
	new_game->num_guesses = 0;

	list_add_tail(&new_game->list, &games_list);

	spin_unlock_irqrestore(&lock, flags);
	return new_game;
}

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
	unsigned int bytes_copied;
	int ret_value;
	unsigned long flags;
	struct mm_game *curr_player;
	spin_lock_irqsave(&lock, flags);
	bytes_copied = count;
	spin_unlock_irqrestore(&lock, flags);

	curr_player = find_game(current_uid());

	spin_lock_irqsave(&lock, flags);
	if (curr_player) {
		if (*ppos >= strlen(curr_player->game_status)) {
			spin_unlock_irqrestore(&lock, flags);
			return 0;
		} else {

			if (bytes_copied >
			    strlen(curr_player->game_status) - *ppos) {
				bytes_copied =
				    strlen(curr_player->game_status) - *ppos;
			}

			ret_value =
			    copy_to_user((ubuf),
					 curr_player->game_status + *ppos,
					 bytes_copied);

			if (ret_value == 0) {
				*ppos += bytes_copied;
				spin_unlock_irqrestore(&lock, flags);
				return bytes_copied;
			} else {
				spin_unlock_irqrestore(&lock, flags);
				return -EINVAL;
			}
		}
	} else {
		spin_unlock_irqrestore(&lock, flags);
		return -EINVAL;
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
	char guess_log[30];
	int highest_digit = zero_ascii + NUM_COLORS - 1;
	bool valid_input = true;
	unsigned int num_black = 0;
	unsigned int num_white = 0;
	unsigned long flags;
	struct mm_game *curr_player;

	curr_player = find_game(current_uid());
	spin_lock_irqsave(&lock, flags);

	if (curr_player) {
		memset(user_guess, 0, NUM_PEGS);
		memset(peg_color, 0, NUM_PEGS * 4);	//default value is incorrect value and position
		
		if (!curr_player->game_active || count < NUM_PEGS) {
			if (!curr_player->game_active) {
				pr_err("No active game\n");
			} else {
				pr_err("Not enough numbers entered\n");
			}
			spin_unlock_irqrestore(&lock, flags);
			return -EINVAL;
		} else {
			/* check the first four digits are within bounds */
			ret_value =
			    copy_from_user(user_guess, (void *)ubuf, NUM_PEGS);
			for (i = 0; i < NUM_PEGS; i++) {
				if (user_guess[i] < zero_ascii
				    || user_guess[i] > highest_digit) {
					valid_input = false;
				} else {
					user_guess[i] =
					    user_guess[i] - zero_ascii;
				}
			}

			if (valid_input) {
				curr_player->num_guesses++;

				/* calculate black pegs */
				for (i = 0; i < NUM_PEGS; i++) {
					if (target_code[i] == user_guess[i]) {
						num_black++;
						peg_color[i] = 1;	//set peg to be marked
					}
				}

				/* calculate white pegs */
				for (i = 0; i < NUM_PEGS; i++) {
				  if(peg_color[i] == 0){
					for (j = 0; j < NUM_PEGS; j++) {
					  if (target_code[j] == user_guess[i]) {
					    if(peg_color[j] == 1){
						break;
					      }else{
					      num_white++;
					      peg_color[j] = 1;	//set peg to marked
					      break;
					    }
					  }	/* check if the digits were the same and if peg color has been defined */
					}	/* loop through the user guess*/
				  } /* check if peg has already been counted*/
				}	/* loop through all of the target code*/

				if (num_black == NUM_PEGS) {
					/* user guesses the correct code */
					scnprintf(curr_player->game_status, 20,
						  "Correct! Game over\n");

					/* update memory map */
					scnprintf(guess_log, 26,
						  "Correct Guess! Game over\n");

					/* end the game */
					curr_player->game_active = false;

				} else {
					/* update game_status & user_view */
					scnprintf(curr_player->game_status, 41,
						  "Guess %d: %d black peg(s), %d white peg(s)\n",
						  curr_player->num_guesses,
						  num_black, num_white);

					scnprintf(guess_log, 24,
						  "Guess %d: %d%d%d%d  | B%d W%d\n",
						  curr_player->num_guesses,
						  user_guess[0], user_guess[1],
						  user_guess[2], user_guess[3],
						  num_black, num_white);

				}
				strcat(curr_player->user_view, guess_log);

				spin_unlock_irqrestore(&lock, flags);
				return count;
			} else {
				spin_unlock_irqrestore(&lock, flags);
				return -EINVAL;
			}
		}
	} else {
		spin_unlock_irqrestore(&lock, flags);
		return -EINVAL;
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
	unsigned long size;
	unsigned long page;
	unsigned long flags;
	struct mm_game *curr_player;

	curr_player = find_game(current_uid());
	spin_lock_irqsave(&lock, flags);
	if (curr_player) {
		size = (unsigned long)(vma->vm_end - vma->vm_start);
		page = vmalloc_to_pfn(curr_player->user_view);
		if (size > PAGE_SIZE) {
			spin_unlock_irqrestore(&lock, flags);
			return -EIO;
		}
		vma->vm_pgoff = 0;
		vma->vm_page_prot = PAGE_READONLY;
		if (remap_pfn_range
		    (vma, vma->vm_start, page, size, vma->vm_page_prot)) {
			spin_unlock_irqrestore(&lock, flags);
			return -EAGAIN;
		}

		spin_unlock_irqrestore(&lock, flags);
		return 0;
	} else {
		spin_unlock_irqrestore(&lock, flags);
		return -EAGAIN;
	}
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
	char *colors = "colors ";
	int ret_value;
	size_t i;
	unsigned int digit;
	unsigned int bytes_copied = 8;
	char user_input[bytes_copied];
	unsigned long flags;
	struct mm_game *curr_player;

	curr_player = find_game(current_uid());
	spin_lock_irqsave(&lock, flags);
	if (curr_player) {
		if (count < bytes_copied)
			bytes_copied = count;

		ret_value =
		    copy_from_user(user_input, (void *)ubuf, bytes_copied);

		if (ret_value != 0) {
			pr_err("Could not read user input to mm_ctl\n");
			spin_unlock_irqrestore(&lock, flags);
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
				curr_player->game_active = true;
				curr_player->num_guesses = 0;
				memset(curr_player->game_status, 0, 80);
				memset(curr_player->user_view, 0, 80);
				scnprintf(curr_player->game_status, 15,
					  "Starting game\n");
				num_game_started++;

				if (random_code) {
					for (i = 0; i < NUM_PEGS; i++) {
						get_random_bytes(&digit,
								 sizeof(digit));
						digit = digit % NUM_COLORS;
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
				spin_unlock_irqrestore(&lock, flags);
				return count;
			} else if (memcmp(user_input, quit, bytes_copied) == 0) {

				if (curr_player->game_active) {
					/* if user quits an existing game */
					scnprintf(curr_player->game_status, 31,
						  "Game over. The code was %d%d%d%d.\n",
						  target_code[0],
						  target_code[1],
						  target_code[2],
						  target_code[3]);
					curr_player->game_active = false;
				}
				spin_unlock_irqrestore(&lock, flags);
				return count;
			} else if (memcmp(user_input, colors, bytes_copied - 1)
				   == 0) {
				/* if user attempts to set the color */
				/* checks if user is system admin */
				if (capable(CAP_SYS_ADMIN)
				    && count == bytes_copied
				    && user_input[bytes_copied - 1] >=
				    (zero_ascii + 2)) {
					NUM_COLORS = user_input[7] - zero_ascii;
					spin_unlock_irqrestore(&lock, flags);
					return count;
				} else {
					if (!capable(CAP_SYS_ADMIN)) {
						spin_unlock_irqrestore(&lock,
								       flags);
						return -EACCES;
					} else {
						spin_unlock_irqrestore(&lock,
								       flags);
						return -EINVAL;
					}
				}
			} else {
				spin_unlock_irqrestore(&lock, flags);
				return -EINVAL;
			}

		}
	} else {
		pr_err("curr_player returned NULL");
		spin_unlock_irqrestore(&lock, flags);
		return -EINVAL;
	}
}

/**
 * cs421net_top() - top-half of CS421Net ISR
 * @irq: IRQ that was invoked (ignored)
 * @cookie: Pointer to data that was passed into
 * request_threaded_irq() (ignored)
 *
 * If @irq is CS421NET_IRQ, then wake up the bottom-half. Otherwise,
 * return IRQ_NONE.
 */
static irqreturn_t cs421net_top(int irq, void *cookie)
{
	if (irq == CS421NET_IRQ) {
		return IRQ_WAKE_THREAD;
	} else {
		return IRQ_NONE;
	}
}

/**
 * cs421net_bottom() - bottom-half to CS421Net ISR
 * @irq: IRQ that was invoked (ignore)
 * @cookie: Pointer that was passed into request_threaded_irq()
 * (ignored)
 *
 * Fetch the incoming packet, via cs421net_get_data(). If:
 *   1. The packet length is exactly equal to the number of digits in
 *      the target code, and
 *   2. If all characters in the packet are valid ASCII representation
 *      of valid digits in the code, then
 * Set the target code to the new code, and increment the number of
 * types the code was changed remotely. Otherwise, ignore the packet
 * and increment the number of invalid change attempts.
 *
 * During Part 5, update this function to change all codes for all
 * active games.
 *
 * Remember to add appropriate spin lock calls in this function.
 *
 * <em>Caution: The incoming payload is NOT a string; it is not
 * necessarily null-terminated.</em> You CANNOT use strcpy() or
 * strlen() on it!
 *
 * Return: always IRQ_HANDLED
 */
static irqreturn_t cs421net_bottom(int irq, void *cookie)
{
	unsigned long flags;
	const char *data;
	bool valid_code = true;
	int i = 0;
	int highest_digit = zero_ascii + NUM_COLORS - 1;
	size_t size = NUM_PEGS;
	spin_lock_irqsave(&lock, flags);

	data = cs421net_get_data(&size);
	/* Part 4: YOUR CODE HERE */
	if (size == NUM_PEGS) {

		for (i = 0; i < NUM_PEGS; i++) {
			if (data[i] < zero_ascii || data[i] > highest_digit) {
				valid_code = false;
				break;
			}
		}

		if (valid_code) {
			/* set new target code */
			target_code[0] = data[0] - zero_ascii;
			target_code[1] = data[1] - zero_ascii;
			target_code[2] = data[2] - zero_ascii;
			target_code[3] = data[3] - zero_ascii;

			times_target_code_changed =
			    times_target_code_changed + 1;
		} else {
			num_invalid_change_attempts =
			    num_invalid_change_attempts + 1;
		}
	} else {
		num_invalid_change_attempts = num_invalid_change_attempts + 1;
	}
	spin_unlock_irqrestore(&lock, flags);
	return IRQ_HANDLED;
}

/**
 * mm_stats_show() - callback invoked when a process reads from
 * /sys/devices/platform/mastermind/stats
 *
 * @dev: device driver data for sysfs entry (ignored)
 * @attr: sysfs entry context (ignored)
 * @buf: destination to store game statistics
 *
 * Write to @buf, up to PAGE_SIZE characters, a human-readable message
 * containing these game statistics:
 *   - Number of pegs (digits in target code)
 *   - Number of colors (range of digits in target code)
 *   - Number of valid network messages (see Part 4)
 *   - Number of invalid network messages (see Part 4)
 *   - Number of active players (see Part 5)
 * Note that @buf is a normal character buffer, not a __user
 * buffer. Use scnprintf() in this function.
 *
 * @return Number of bytes written to @buf, or negative on error.
 */
static ssize_t mm_stats_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int ret_value;
	unsigned long flags;
	spin_lock_irqsave(&lock, flags);
	ret_value =
	    scnprintf(buf, 147,
		      "Number of pegs: %d \nNumber of colors: %d \nNumber of times code was changed: %d\nNumber of invalid code change attempts: %d\nNumber of games started: %d\n",
		      NUM_PEGS, NUM_COLORS, times_target_code_changed,
		      num_invalid_change_attempts, num_game_started);

	spin_unlock_irqrestore(&lock, flags);
	if (ret_value >= 0) {
		return ret_value;
	} else {
		return -EPERM;
	}
}

static DEVICE_ATTR(stats, S_IRUGO, mm_stats_show, NULL);

/**
 * mastermind_probe() - callback invoked when this driver is probed
 * @pdev platform device driver data
 *
 * Return: 0 on successful probing, negative on error
 */
static int mastermind_probe(struct platform_device *pdev)
{
	/*
	 * You will need to integrate the following resource allocator
	 * into your code. That also means properly releasing the
	 * resource if the function fails.
	 */
	int retval;
	char *irq_name = "cs421net";
	retval = device_create_file(&pdev->dev, &dev_attr_stats);
	if (retval) {
		pr_err("Could not create sysfs entry\n");
		goto device_create_file_failure;
	}

	/* Copy the contents of your original mastermind_init() here. */
	pr_info("Initializing the game.\n");

	if (misc_register(&device_mm) != 0) {
		pr_err("Could not register device_mm\n");
		goto device_mm_failure;
	}

	if (misc_register(&device_mm_ctl) != 0) {
		pr_err("Could not register device_mm_ctl\n");
		goto device_mm_ctl_failure;
	}

	/* register a threaded interrupt handler */
	if (request_threaded_irq(CS421NET_IRQ,
				 cs421net_top, cs421net_bottom,
				 0, irq_name, pdev) < 0) {
		goto request_irq_fail;
	}

	/* enable interrupts */
	cs421net_enable();

	return retval;
	/* use goto statements to handle error & cleanup" */

request_irq_fail:
	misc_deregister(&device_mm_ctl);
device_mm_ctl_failure:
	misc_deregister(&device_mm);
device_mm_failure:
	device_remove_file(&pdev->dev, &dev_attr_stats);
device_create_file_failure:
	pr_info("Freeing resources.\n");
	return -ENODEV;
}

/**
 * mastermind_remove() - callback when this driver is removed
 * @pdev platform device driver data
 *
 * Return: Always 0
 */
static int mastermind_remove(struct platform_device *pdev)
{
	/* Copy the contents of your original mastermind_exit() here. */
	/*YOUR CODE - disable interrupt handler here */
	struct mm_game *entry, *tmp;
	pr_info("Freeing resources.\n");

	list_for_each_entry_safe(entry, tmp, &games_list, list) {
		vfree(entry->user_view);
		kfree(entry);
	}

	misc_deregister(&device_mm);
	misc_deregister(&device_mm_ctl);

	/* disable interrupts */
	cs421net_disable();
	/* remove threaded interrupt handler */
	free_irq(CS421NET_IRQ, pdev);

	device_remove_file(&pdev->dev, &dev_attr_stats);
	return 0;
}

static struct platform_driver cs421_driver = {
	.driver = {
		   .name = "mastermind",
		   },
	.probe = mastermind_probe,
	.remove = mastermind_remove,
};

static struct platform_device *pdev;

/**
 * cs421_init() -  create the platform driver
 * This is needed so that the device gains a sysfs group.
 *
 * <strong>You do not need to modify this function.</strong>
 */
static int __init cs421_init(void)
{
	pdev = platform_device_register_simple("mastermind", -1, NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);
	return platform_driver_register(&cs421_driver);
}

/**
 * cs421_exit() - remove the platform driver
 * Unregister the driver from the platform bus.
 *
 * <strong>You do not need to modify this function.</strong>
 */
static void __exit cs421_exit(void)
{
	platform_driver_unregister(&cs421_driver);
	platform_device_unregister(pdev);
}

module_init(cs421_init);
module_exit(cs421_exit);

MODULE_DESCRIPTION("CS421 Mastermind Game++");
MODULE_LICENSE("GPL");
