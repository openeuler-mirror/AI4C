#include <iostream>
#include <numeric>
#include <vector>

#define SIZE 512

using Dims = std::vector<size_t>;

struct Mat {
  Dims dims;
  double *data;

  ~Mat() { delete data; }
};

Mat *init_matrix(Dims dims) {
  srand(time(NULL));
  Mat *matrix = (Mat *)malloc(sizeof(Mat));
  matrix->dims = dims;
  size_t dim_size = std::accumulate(dims.begin(), dims.end(), (size_t)1,
                                    std::multiplies<size_t>());
  matrix->data = (double *)malloc(dim_size * sizeof(double));
  for (size_t i = 0; i < dim_size; i++) {
    matrix->data[i] = (double)rand() / RAND_MAX * 2 - 1;
  }
  return matrix;
}

Mat *matmul(Mat *mat1, Mat *mat2) {
  size_t m, k, n;
  m = mat1->dims[0];
  k = mat1->dims[1];
  n = mat2->dims[1];

  Mat *result = (Mat *)malloc(sizeof(Mat));
  result->data = (double *)malloc(m * n * sizeof(double));

  for (size_t i = 0; i < m; i++) {
    for (size_t j = 0; j < n; j++) {
      size_t r_idx = i * n + j;
      result->data[r_idx] = 0.0f;
      for (int x = 0; x < k; x++) {
        result->data[r_idx] += mat1->data[i * k + x] * mat2->data[x * n + j];
      }
    }
  }
  return result;
}

int main(int argc, char *argv[]) {
  int run_matmul_num = 1;
  if (argc > 1) {
    run_matmul_num = atoi(argv[1]);
  }

  Mat *mat1 = init_matrix({SIZE, SIZE});
  Mat *mat2 = init_matrix({SIZE, SIZE});
  Mat *result;
  for (int i = 0; i < run_matmul_num; i++) {
    result = matmul(mat1, mat2);
    printf("MatMul run time = %d\n", i);
  }

  delete mat1;
  delete mat2;
  delete result;

  printf("Done\n");
  return 0;
}