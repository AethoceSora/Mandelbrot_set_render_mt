#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

//全局变量 NT，表示线程数量
int NT;
//画布宽 6400
#define W 6400
//画布高 6400
#define H 6400
//输出图片文件名
#define IMG_FILE "mandelbrot.ppm"

//判断 (x,y) 点是否属于第 t 个线程处理的范围
static inline int belongs(int x, int y, int t) {
  return (x / (W / NT)) + 1 == t;
}

int x[W][H];  //存储每个像素点的计算结果
int volatile done = 0;  //完成线程数量
pthread_mutex_t lock;

//按步长 step 展示图片，将结果写入文件
void display(FILE *fp, int step) {
  int w = W / step, h = H / step;

  //在文件中写入头信息
  fprintf(fp, "P6\n%d %d 255\n", w, h);
  for (int j = 0; j < H; j += step) 
    for (int i = 0; i < W; i += step) {
      int n = x[i][j];
      //根据计算结果，计算 RGB 值
      int r = 255 * pow((n - 80) / 800.0, 3);
      int g = 255 * pow((n - 80) / 800.0, 0.7);
      int b = 255 * pow((n - 80) / 800.0, 0.5);
      //将 RGB 值写入文件
      fputc(r, fp); fputc(g, fp); fputc(b, fp);
    }
  }


void *Tworker(void *arg) {
  int tid = *(int *)arg;  //当前线程的编号
  for (int i = 0; i < W; i++) {
    for (int j = 0; j < H; j++) {
      if (belongs(i, j, tid)) {  //判断当前像素点是否属于当前线程处理的范围
        double a = 0, b = 0, c, d;
        while ((c = a * a) + (d = b * b) < 4 && x[i][j]++ < 880) {  //Mandelbrot 算法迭代计算像素点的值
          b = 2 * a * b + j * 1024.0 / H * 8e-9 - 0.645411;
          a = c - d + i * 1024.0 / W * 8e-9 + 0.356888;
        }
      }
    }
  }

  //计算完成线程数量
  pthread_mutex_lock(&lock);
  done++;
  pthread_mutex_unlock(&lock);

  return NULL;
}

//在终端展示图片，并输出渲染所需时间
void *Tdisplay(void *arg) {
  float ms = 0;
  while (1) {
    pthread_mutex_lock(&lock);
    //如果所有线程都计算完成，则退出展示
    if (done == NT) {
      pthread_mutex_unlock(&lock);
      break;
    }
    pthread_mutex_unlock(&lock);

    FILE *fp = fopen("temp.ppm", "w"); assert(fp);  //临时文件，展示用
    display(fp, W / 256);
    fclose(fp);

    system("viu temp.ppm");  //通过 viu 在终端展示图片

    usleep(1000000 / 5);  //等待 0.2s
    ms += 1000.0 / 5;  //计算渲染用时
  }
  printf("Approximate render time: %.1lfs\n", ms / 1000);  //输出渲染所需时间

  FILE *fp = fopen(IMG_FILE, "w"); assert(fp);  //正式的图片文件
  display(fp, 2);  //按 2 的步长展示图片
  fclose(fp);

  return NULL;
}

int main(int argc, char *argv[]) {
  assert(argc == 2);

  NT = atoi(argv[1]);  //读取线程数量
  pthread_t workers[NT];
  pthread_t display_thread;

  pthread_mutex_init(&lock, NULL);

  int thread_ids[NT];
  for (int i = 0; i < NT; i++) {
    thread_ids[i] = i + 1;
    pthread_create(&workers[i], NULL, Tworker, &thread_ids[i]);  //创建线程
  }
  pthread_create(&display_thread, NULL, Tdisplay, NULL);

  //等待所有线程完成
  for (int i = 0; i < NT; i++) {
    pthread_join(workers[i], NULL);
  }
  pthread_join(display_thread, NULL);

  pthread_mutex_destroy(&lock);

  return 0;
}
