#include "classifier.h"

int for_line_in_file(const char *filepath,
                     int (*callback)(void* line, void* len, 
                                      void* context),
                     void* context) {
  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(filepath, "r");
  if (fp == NULL)
    exit(EXIT_FAILURE);

  while ((read = getline(&line, &len, fp)) != -1) {
    if (callback(line, (void *)read, context) != EXIT_SUCCESS) {
      break;
    }
  }
  
  if (line) {
    free(line);
  }
  
  if (fp) {
    fclose(fp);
  }
  
  return EXIT_SUCCESS;
}

float dumb_diff_matrices(const float* a, const float* b, int mlen) {
  float diffsum = 0;
  for (int i = 0; i < mlen; ++i) {
    if (a[i] <= 0 || b[i] <= 0) {
      continue;
    }
    float diff = fabs(a[i] - b[i]);
    if (diff > 0) {
      diffsum += diff;
    }
  }
  return diffsum;
}

int index_2d(int x, int y, int x_dim, int y_dim) {
  return x*y_dim + y;
}

int count_word_transitions(const char* word, int word_length,
                           int alphabet_width,
                           int **transition_counts,
                           int **character_counts) {
  for (int i = 0; i < word_length-1; ++i) {
    char x = word[i];
    char y = word[i+1];
    if (x < alphabet_width && y < alphabet_width) {
      (*transition_counts)[index_2d(x, y, alphabet_width,
                                       alphabet_width)] += 1;
      (*character_counts)[(int)x] += 1;
    } else {
      fprintf(stderr, "WARNING: chars out of range: %c -> %c\n", x,
              y);
    }
    if (i == word_length-2) {
      (*character_counts)[(int)y] += 1; // last char
    }
  }
  return 0;
}

int convert_transition_counts_to_freqs(const int *counts,
                                       int alphabet_width,
                                       float **freqs) {
  float row_sums[alphabet_width];
  for (int i = 0; i < alphabet_width; i++) {
    row_sums[i] = 0;
    for (int j = 0; j < alphabet_width; j++) {
      int index = index_2d(i, j, alphabet_width, alphabet_width);
      row_sums[i] += counts[index];
    }
    for (int j = 0; j < alphabet_width; j++) {
      int index = index_2d(i, j, alphabet_width, alphabet_width);
      if (row_sums[i] > 0) {
        (*freqs)[index] = counts[index] / row_sums[i];
      }
    }
  }
  return EXIT_SUCCESS;
}

int convert_character_counts_to_freqs(const int *counts,
                                      int alphabet_width,
                                      float** freqs) {
  float sum = 0.0;
  for (int i = 0; i < alphabet_width; ++i) {
    sum += counts[i];
  }
  for (int i = 0; i < alphabet_width; ++i) {
    (*freqs)[i] = counts[i] / sum;
  }
  return EXIT_SUCCESS;
}

int build_classifier_from_line(void *line, void *read, void *ctx) {
  LanguageClassifier *c = ctx;

  count_word_transitions((char *)line, (size_t)read-1,
                         c->alphabet_width,
                         &(c->transition_counts),
                         &(c->character_counts));
  return EXIT_SUCCESS;
}

LanguageClassifier* create_language_classifier(
    const char* words_file, int alphabet_width) {
  LanguageClassifier *c;
  if (NULL == (c = malloc(sizeof(LanguageClassifier)))) {
    return NULL;
  }
  c->alphabet_width = alphabet_width;
  c->transition_counts = calloc(c->alphabet_width *
                                c->alphabet_width,
                                sizeof(int));
  c->transition_frequencies = calloc(c->alphabet_width *
                                     c->alphabet_width,
                                     sizeof(float));
  c->character_counts = calloc(c->alphabet_width,
                               sizeof(int));
  c->character_frequencies = calloc(c->alphabet_width,
                                    sizeof(float));

  if (for_line_in_file(words_file,
                       build_classifier_from_line, c)) {
    return NULL;
  }
  
  convert_transition_counts_to_freqs(c->transition_counts,
                                     c->alphabet_width,
                                     &(c->transition_frequencies));
  
  convert_character_counts_to_freqs(c->character_counts,
                                    c->alphabet_width,
                                    &(c->character_frequencies));
  return c;
}

int delete_language_classifier(LanguageClassifier* classifier) {
  free(classifier->character_counts);
  free(classifier->transition_counts);
  free(classifier->character_frequencies);
  free(classifier->transition_frequencies);
  free(classifier);
  return EXIT_SUCCESS;
}

float score_transitions(const LanguageClassifier* classifier,
                        const char *buf, int buflen) {
  const LanguageClassifier *c = classifier;

  float score = 0.0;
  for (int i = 0; i < buflen-1; ++i) {
    char a = buf[i];
    char b = buf[i+1];
    int index = index_2d(a, b, c->alphabet_width,
                            c->alphabet_width);

    if (a < c->alphabet_width && b < c->alphabet_width) {
      score += (100.0 * c->transition_frequencies[ index ]);
    } else {
      printf("WARNING: skipping chars: %c -> %c\n", a, b);
    }
  }
  return score / buflen;
}

float score_frequencies(const LanguageClassifier* classifier,
                        const char *buf, int buflen) {
  const LanguageClassifier *c = classifier;
  float buf_freqs[c->alphabet_width];
  for (int i = 0; i < c->alphabet_width; i++) {
    buf_freqs[i] = 0.0;
  }
  for (int i = 0; i < buflen; ++i) {
    buf_freqs[ (int) buf[i] ] += (1.0 / buflen);
  }
  return dumb_diff_matrices(c->character_frequencies,
                            buf_freqs, c->alphabet_width);
}

float score_as_language_custom(const LanguageClassifier* classifier,
                               const char *buf,
                               int buflen,
                               float transition_weight,
                               float frequency_weight) {
  float frequency_score = score_frequencies(classifier, buf, buflen);
  float transition_score = score_transitions(classifier, buf, buflen);
  float combined_score = ((frequency_score * frequency_weight) +
                          (transition_score * transition_weight));
  return combined_score;
}

float score_as_language(const LanguageClassifier* classifier,
                        const char *buf, int buflen) {
  return score_as_language_custom(classifier, buf, buflen, 1.0, 1.0);
}

