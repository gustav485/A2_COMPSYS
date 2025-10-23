// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>
#include <pthread.h>

#include "job_queue.h"

pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;

int fauxgrep_file(char const *needle, char const *path) {
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    assert(pthread_mutex_lock(&stdout_mutex) == 0);
    warn("failed to open %s", path);
    assert(pthread_mutex_unlock(&stdout_mutex) == 0);
    return -1;
  }

  char *line = NULL;
  size_t linelen = 0;
  int lineno = 1;
  while (getline(&line, &linelen, f) != -1) {
    if (strstr(line, needle) != NULL) {
      assert(pthread_mutex_lock(&stdout_mutex) == 0);
      printf("%s:%d: %s", path, lineno, line);
      assert(pthread_mutex_unlock(&stdout_mutex) == 0);
    }
    lineno++;
  }
  free(line);
  fclose(f);
  return 0;
}

void *worker(void *arg) {
  struct job_queue *q = ((void **)arg)[0];
  char const *needle = ((void **)arg)[1];
  void **args = arg;
  free(args);

  void *data;
  while (job_queue_pop(q, &data) == 0) {
    if (data == NULL) break;
      char *path = data;
      fauxgrep_file(needle, path);
      free(path);
  }
  return NULL;
}

int main(int argc, char * const *argv) {
  if (argc < 2) {
    err(1, "usage: [-n INT] STRING paths...");
    exit(1);
  }

  int num_threads = 1;
  char const *needle = argv[1];
  char * const *paths = &argv[2];

  struct job_queue q;
  int capacity = num_threads * 2;

  if (argc > 3 && strcmp(argv[1], "-n") == 0) {
  // Since atoi() simply returns zero on syntax errors, we cannot
  // distinguish between the user entering a zero, or some
  // non-numeric garbage.  In fact, we cannot even tell whether the
  // given option is suffixed by garbage, i.e. '123foo' returns
  // '123'.  A more robust solution would use strtol(), but its
  // interface is more complicated, so here we are.
    num_threads = atoi(argv[2]);

    if (num_threads < 1) {
      err(1, "invalid thread count: %s", argv[2]);
    }
      needle = argv[3];
      paths = &argv[4];
    } else {
      needle = argv[1];
      paths = &argv[2];
    }


    if (job_queue_init(&q, capacity) != 0) {
      err(1, "Failed to initialize job queue.\n");
    }

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
      void **args = malloc(2 * sizeof(void*));
      args[0] = &q;
      args[1] = (void*)needle;
      pthread_create(&threads[i], NULL, worker, args);
    }
  // FTS_LOGICAL = follow symbolic links
  // FTS_NOCHDIR = do not change the working directory of the process
  //
  // (These are not particularly important distinctions for our simple
  // uses.)
    int fts_options = FTS_LOGICAL | FTS_NOCHDIR;

    FTS *ftsp;
    if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL) {
    err(1, "fts_open() failed");
    return -1;
    }
    FTSENT *p;
    while ((p = fts_read(ftsp)) != NULL) {
    switch (p->fts_info) {
    case FTS_D:
      break;
    case FTS_F:
      job_queue_push(&q, strdup(p->fts_path));
      break;
    default:
      break;
    }
  }

  fts_close(ftsp);

  for (int i = 0; i < num_threads; i++) {
    job_queue_push(&q, NULL);
  }

  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  if (job_queue_destroy(&q) != 0) {
    err(1, "Failed to destroy job queue");
  }

  free(threads);
  return 0;
}
