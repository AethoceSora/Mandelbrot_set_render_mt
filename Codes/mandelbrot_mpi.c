#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define W 6400
#define H 6400
#define IMG_FILE "mandelbrot.ppm"
#define STEP 2

int main(int argc, char **argv) {
  int NT; // 进程数
  int my_rank; // 进程编号

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &NT);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  int work[W/STEP][H/STEP];  //存储每个像素点的计算结果
  int start = my_rank * W / NT / STEP;
  int end = (my_rank + 1) * W / NT / STEP;
  double a, b, c, d;

  for (int i = start; i < end; i++) {
    for (int j = 0; j < H/STEP; j++) {
      a = b = 0;
      int n = 0;
      while((c = a*a) + (d = b*b) < 4 && n < 880) {
        b = 2 * a * b + (j * 1024.0 / H) * 8e-9 - 0.645411;
        a = c - d + (i * STEP * 1024.0 / W) * 8e-9 + 0.356888;
        n++;
      }
      work[i-start][j] = n;
    }
  }

  if (my_rank == 0) {
    FILE *fp = fopen(IMG_FILE, "w"); 
    fprintf(fp, "P6\n%d %d 255\n", W/STEP, H/STEP);

    for (int j = 0; j < H/STEP; j++) {
      for (int i = 0; i < W/STEP; i++) {
        int n =  work[i][j];
        //根据计算结果，计算 RGB 值
        int r = 255 * pow((n - 80) / 800.0, 3);
        int g = 255 * pow((n - 80) / 800.0, 0.7);
        int b = 255 * pow((n - 80) / 800.0, 0.5);
        //将 RGB 值写入文件
        fputc(r, fp); fputc(g, fp); fputc(b, fp);
      }
    }

    fclose(fp);
  }

  for (int i = 0; i < W/STEP; i++) {
    for (int j = 0; j < H/STEP; j++) {
      MPI_Bcast(&work[i][j], 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();
  return 0;
}
