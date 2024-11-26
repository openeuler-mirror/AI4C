#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 512

void generate_matrix(float matrix[SIZE][SIZE]) {
  srand(time(NULL));
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      matrix[i][j] = (float)rand() / RAND_MAX;
    }
  }
}

void matmul(float result[SIZE][SIZE], float matrix1[SIZE][SIZE],
            float matrix2[SIZE][SIZE]) {
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      result[i][j] = 0.0f;
      for (int k = 0; k < SIZE; k++) {
        result[i][j] += matrix1[i][k] * matrix2[k][j];
      }
    }
  }
}

int main(int argc, char *argv[]) {
  int run_matmul_num = 1;
  if (argc > 1) {
    run_matmul_num = atoi(argv[1]);
  }

  for (int i = 0; i < run_matmul_num; i++) {
    float matrix1[SIZE][SIZE];
    float matrix2[SIZE][SIZE];
    float result[SIZE][SIZE];

    generate_matrix(matrix1);
    generate_matrix(matrix2);

    matmul(result, matrix1, matrix2);
  }

  return 0;
}
