#include <linux/completion.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/slab.h>

static const char *answers[] = {
	"This Finn wrote Linux as a hobby during college.",
	"Google's smartphones run this robotic OS, based upon Linux.",
	"Tux, the mascot for Linux, is this type of aquatic animal.",
	"This Linux distribution is named after its founder and his girlfriend."
};
static const char *questions[] = {
	"Who is Linus Torvalds?",
	"What is Android?",
	"What is a penguin?",
	"What is Debian?"
};
static const char *names[] = { "Ken", "Watson", "Brad" };
static unsigned ready, num, failed_tries;

struct contestant {
	const char *name;
	int score;
	bool can_buzz_in;
	struct list_head list;
};
static LIST_HEAD(contestants);
static DEFINE_MUTEX(lock);
static DECLARE_COMPLETION(cont_cv);
static DECLARE_COMPLETION(alex_cv);

#define set_all_buzzers(v) { \
    struct contestant *entry; \
    list_for_each_entry(entry, &contestants, list) { \
      entry->can_buzz_in = (v); \
    } \
  }

static int play_kthread(void *data)
{
	struct contestant *c = (struct contestant *)data;
	unsigned int ans;
	mutex_lock(&lock);
	ready++;
	while (true) {
		mutex_unlock(&lock);
		wait_for_completion_interruptible(&cont_cv);
		mutex_lock(&lock);
		reinit_completion(&cont_cv);
		if (num >= ARRAY_SIZE(answers))
			break;
		if (!c->can_buzz_in)
			continue;
		ans = get_random_int() % ARRAY_SIZE(questions);
		pr_info("  %s: %s\n", c->name, questions[ans]);
		c->can_buzz_in = false;
		if (ans == num) {
			pr_info("  Alex: Correct.\n");
			c->score += 200 * (num + 1);
			set_all_buzzers(false);
			complete(&alex_cv);
		} else {
			failed_tries++;
			pr_info("  Alex: No.\n");
			c->score -= 200 * (num + 1);
			if (failed_tries >= ARRAY_SIZE(names))
				complete(&alex_cv);
			else
				complete(&cont_cv);
		}
		schedule_timeout_interruptible(HZ);
	}
	mutex_unlock(&lock);
	complete(&cont_cv);
	return 0;
}

static int alex_kthread(void *data)
{
	mutex_lock(&lock);
	while (ready < ARRAY_SIZE(names)) {
		mutex_unlock(&lock);
		mutex_lock(&lock);
	}

	for (; num < ARRAY_SIZE(answers); num++) {
		failed_tries = 0;
		pr_info("Alex: %s\n", answers[num]);
		schedule_timeout_interruptible((get_random_int() % 4 + 1) * HZ);
		set_all_buzzers(true);
		complete(&cont_cv);

		/* wait for contestants to finish guessing */
		mutex_unlock(&lock);
		wait_for_completion_interruptible(&alex_cv);
		mutex_lock(&lock);
		reinit_completion(&alex_cv);

		set_all_buzzers(false);
		if (failed_tries == ARRAY_SIZE(names)) {
			pr_info("  Alex: The correct response was \"%s\"\n", questions[num]);
			schedule_timeout_interruptible(2 * HZ);
		}
	}
	mutex_unlock(&lock);
	complete(&cont_cv);
	pr_info("Alex: The scores going into final Jeopardy! are:\n");
	return 0;
}

static int __init jeop_init(void)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(names); i++) {
		struct contestant *entry = kzalloc(sizeof(*entry), GFP_KERNEL);
		entry->name = names[i];
		list_add_tail(&entry->list, &contestants);
		kthread_run(play_kthread, entry, entry->name);
	}
	kthread_run(alex_kthread, NULL, "Alex");
	return 0;
}

static void __exit jeop_exit(void)
{
	struct contestant *entry, *tmp;
	list_for_each_entry_safe(entry, tmp, &contestants, list) {
		pr_info("  %s is at %d.\n", entry->name, entry->score);
		kfree(entry);
	}
}

module_init(jeop_init);
module_exit(jeop_exit);
MODULE_DESCRIPTION("CS421 Final");
MODULE_LICENSE("GPL");
