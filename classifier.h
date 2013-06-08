#ifndef __CLASSIFIER_H
#define __CLASSIFIER_H

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct LanguageClassifier {
  float *transition_frequencies;
  float *character_frequencies;
  int *transition_counts;
  int *character_counts;
  int alphabet_width;
} LanguageClassifier;

// belongs in utils, but for simplicity it's here
int for_line_in_file(const char *filepath,
                     int (*callback)(void* line, void* len, 
                                      void* context),
                                      void* context);

LanguageClassifier* create_language_classifier(
    const char* words_file, int alphabet_width);

int delete_language_classifier(LanguageClassifier* classifier);

float score_as_language_custom(const LanguageClassifier* classifier,
                               const char *buf,
                               int buflen,
                               float transition_weight,
                               float frequency_weight);


float score_transitions(const LanguageClassifier* classifier,
                        const char *buf, int buflen);

float score_frequencies(const LanguageClassifier* classifier,
                        const char *buf, int buflen);

float score_as_language(const LanguageClassifier* classifier,
                        const char *buf, int buflen);

#endif /* __CLASSIFIER_H */ 

