#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

static const char *answers[] = {
    "This software loads and starts running the kernel.",
    "PCIe, SATA, and Lightning are examples of this computing interconnect.",
    "This hardware translates signals from the CPU into commands to an I/O device.",
    "The program counter is set to this value when the CPU is interrupted."
};

static const char *questions[] = {
    "What is a bootloader?",
    "What are buses?",
    "What is a device controller?",
    "What is the interrupt service routine?"
};

static struct contestant {
    pthread_t tid;
    const char *name;
    int score;
    bool can_buzz_in;
} contestants[] = {
    {.name = "Ken"}, {.name = "Watson"}, {.name = "Brad"}
};

static unsigned ready, num, failed_tries;
static pthread_mutex_t lock;
static pthread_cond_t cont_cv, alex_cv;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define set_all_buzzers(v) \
    for (size_t i = 0; i < ARRAY_SIZE(contestants); i++) \
        contestants[i].can_buzz_in = (v);

static void *play(void *arg) {
    struct contestant *c = (struct contestant *)(arg);
    pthread_mutex_lock(&lock);
    ready++;
    while (true) {
        pthread_cond_wait(&cont_cv, &lock);
        if (num >= ARRAY_SIZE(answers))
            break;
        if (!c->can_buzz_in)
            continue;
        unsigned int ans = rand() % ARRAY_SIZE(questions);
        printf("  %s: %s\n", c->name, questions[ans]);
        c->can_buzz_in = false;
        if (ans == num) {
            printf("  Alex: Correct.\n");
            c->score += 200 * (num + 1);
            set_all_buzzers(false);
            pthread_cond_signal(&alex_cv);
        }
        else {
            failed_tries++;
            printf("  Alex: No.\n");
            c->score -= 200 * (num + 1);
            if (failed_tries >= ARRAY_SIZE(contestants))
                pthread_cond_signal(&alex_cv);
        }
        sleep(1);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(void) {
    srand(getpid());
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cont_cv, NULL);
    pthread_cond_init(&alex_cv, NULL);
    for (size_t i = 0; i < ARRAY_SIZE(contestants); i++)
        pthread_create(&contestants[i].tid, NULL, play, contestants + i);

    pthread_mutex_lock(&lock);
    while (ready < ARRAY_SIZE(contestants)) {
        pthread_mutex_unlock(&lock);
        pthread_mutex_lock(&lock);
    }

    for ( ; num < ARRAY_SIZE(answers); num++) {
        failed_tries = 0;
        printf("Alex: %s\n", answers[num]);
        sleep(rand() % 4 + 1);
        set_all_buzzers(true);
        pthread_cond_broadcast(&cont_cv);

        /* wait for contestants to finish guessing */
        pthread_cond_wait(&alex_cv, &lock);

        set_all_buzzers(false);
        if (failed_tries == ARRAY_SIZE(contestants)) {
            printf("  Alex: The correct response was \"%s\"\n", questions[num]);
            sleep(2);
        }
    }

    pthread_cond_broadcast(&cont_cv);
    pthread_mutex_unlock(&lock);
    printf("Alex: Let's check the scores after the first round:\n");
    for (size_t i = 0; i < ARRAY_SIZE(contestants); i++) {
        pthread_join(contestants[i].tid, NULL);
        printf("  %s is at %d.\n", contestants[i].name, contestants[i].score);
    }
    return 0;
}
