#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "classifier.h"

/*
 * Example use of classifier lib
 *
 * How to compile:
 *   $ clang -std=gnu99 -Wall -Werror classifier.c \
 *        classifier_example.c -o classifier_example
 * 
 * Example Usage:
 *   # train model on the classifier source and then score this file
 *   $ ./classifier_example -w classifier.c classifier_example.c
 *   
 *   # train model on dictionary and then score this file (note lower score)
 *   $ ./classifier_example -w /usr/share/dict/words classifier_example.c
 *
 */

bool file_exists(const char *filename) {
  struct stat buffer;
  return (stat(filename, &buffer) == 0);
}

void usage(char *argv[]) {
  printf("%s: [-w word_file] wall_of_text\n",
           argv[0]);
  exit(1);
}

static struct option long_options[] = {
  {"words",    required_argument, 0, 'w'},
  {0, 0, 0, 0}
};


typedef struct ScoringContext {
  LanguageClassifier *classifier;
  float total_score;
  int num_lines;
} ScoringContext;

int score_line_and_accumulate_score(void *line, void *read,
                                     void *ctx) {
  ScoringContext *sc = ctx;
  LanguageClassifier *c = sc->classifier;
  float transition_score = score_transitions(c, line, (ssize_t)read);
  float frequency_score = score_frequencies(c, line, (ssize_t)read);
  float combined_normalized_score = (transition_score +
                                     frequency_score) / (ssize_t)read;  
  printf("score(t: %.2f, f: %.2f): %.2f: \"%.*s\"\n",
         transition_score, frequency_score, combined_normalized_score,
         (int)((ssize_t)read)-1, (char *)line);
  sc->total_score += combined_normalized_score;
  sc->num_lines += 1;
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  int c, option_index;
  char *words_file = NULL;
  char *wall_of_text_file = NULL;

  while (1) {
    option_index = 0;
    c = getopt_long(argc, argv, "w:", long_options, &option_index);
    /* Detect the end of the options. */
    if (c == -1)
      break;
    switch (c) {
      case 0:
        break;
      case 'w':
        words_file = optarg;
        break;
      case '?':
        break;      
      default:
        abort();
    }
  }

  if (optind < argc) {
    wall_of_text_file = argv[optind++];
  } else {
    usage(argv);
  }

  // default words file path
  if (words_file == NULL) {
    words_file = "/usr/share/dict/words";
  }
  
  if (!file_exists(words_file)) {
    printf("training file (%s) does not exist\n", words_file);
  } else {
    printf("training file: \"%s\"\n", words_file);
  }

  if (!file_exists(wall_of_text_file)) {
    printf("target file (%s) does not exist\n", wall_of_text_file);
  } else {
    printf("target file: \"%s\"\n", wall_of_text_file);
  }

  // initialize classifier
  LanguageClassifier *lc = create_language_classifier(words_file,
                                                      256);
  
  // simple usage
  char *s = "hello world";
  float score = score_as_language(lc, s, strlen(s));
  printf("%3f: \"%s\"\n", score, s);

  char *p = "dlrow olleh";
  score = score_as_language(lc, p, strlen(p));
  printf("%3f: \"%s\"\n", score, p);
  
  
  // slightly more complicated example
  ScoringContext *sc = calloc(1, sizeof(ScoringContext));
  sc->classifier = lc;
  
  for_line_in_file(wall_of_text_file,
                   score_line_and_accumulate_score, sc);
  printf("Cumulative score for %s [%d lines]: %2.2f\n",
         wall_of_text_file, sc->num_lines, (sc->total_score /
                                            sc->num_lines));
  
  // clean up
  free(sc);
  delete_language_classifier(lc);
  
  return 0;
}
