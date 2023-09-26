/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void transpose_submit_32(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

// 32 x 32矩阵转置
void transpose_32(int M, int N, int A[N][M], int B[M][N]) {
    int temp;
    for (int n=0; n < N; n += 8) {
        for (int m=0; m < M; m += 8) {
            for (int i=n; i < n+8; ++i) {
                for (int j=m; j < m+8; ++j) {
                    temp = A[i][j];
                    B[j][i] = temp;
                }
            }
        }
    }
}

// 32 x 32矩阵转置
void transpose_submit_32(int M, int N, int A[N][M], int B[M][N]) {
    int temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    for (int n=0; n < N; n += 8) {
        for (int m=0; m < M; m += 8) {
            for (int i=n; i < n+8; ++i) {
                // 对角矩阵的情况
                if (m==n) {
                    // 一次性取出A的一行
                    temp0 = A[i][m];
                    temp1 = A[i][m+1];
                    temp2 = A[i][m+2];
                    temp3 = A[i][m+3];
                    temp4 = A[i][m+4];
                    temp5 = A[i][m+5];
                    temp6 = A[i][m+6];
                    temp7 = A[i][m+7];

                    B[m][i] = temp0;
                    B[m+1][i] = temp1;
                    B[m+2][i] = temp2;
                    B[m+3][i] = temp3;
                    B[m+4][i] = temp4;
                    B[m+5][i] = temp5;
                    B[m+6][i] = temp6;
                    B[m+7][i] = temp7;
                } else {    // 非对角矩阵的情况
                    for (int j=m; j < m+8; ++j) {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
    }
}

// 采用4x4子矩阵规模分块转置
void transpose_64(int M, int N, int A[N][M], int B[M][N]) {
    int temp0, temp1, temp2, temp3;
    for (int n=0; n < N; n += 4) {
        for (int m=0; m < M; m += 4) {
            for (int i=n; i < n+4; ++i) {
                // 对角矩阵的情况
                if (m==n) {
                    // 一次性取出A的一行
                    temp0 = A[i][m];
                    temp1 = A[i][m+1];
                    temp2 = A[i][m+2];
                    temp3 = A[i][m+3];

                    B[m][i] = temp0;
                    B[m+1][i] = temp1;
                    B[m+2][i] = temp2;
                    B[m+3][i] = temp3;
                } else {    // 非对角矩阵的情况
                    for (int j=m; j < m+4; ++j) {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
    }
}

void transpose_submit_64(int M, int N, int A[N][M], int B[M][N]){
    int i, j, x, y;
    int x1, x2, x3, x4, x5, x6, x7, x8;
    // 还是8x8分块，这个没有变
    // n和m初始时一定指向8x8矩阵的左上顶端。
    for (i = 0; i < N; i += 8)
        for (j = 0; j < M; j += 8)
        {
            // 这个循环做两件事：翻转A的左上并放入B的左上，翻转A的右上并放入B的右上
            // 这个循环走完后，缓存中剩余什么？B的前4行。
            for (x = i; x < i + 4; ++x)
            {
                x1 = A[x][j]; x2 = A[x][j+1]; x3 = A[x][j+2]; x4 = A[x][j+3];
                x5 = A[x][j+4]; x6 = A[x][j+5]; x7 = A[x][j+6]; x8 = A[x][j+7];
                
                // 翻转A的左上并放入B的左上
                B[j][x] = x1; B[j+1][x] = x2; B[j+2][x] = x3; B[j+3][x] = x4;
                // 翻转A的右上并放入B的右上
                B[j][x+4] = x5; B[j+1][x+4] = x6; B[j+2][x+4] = x7; B[j+3][x+4] = x8;
            }
            // 这个循环做两件事：翻转A左下到B的右上；将B原本的右上移动到右下
            for (y = j; y < j + 4; ++y)
            {
                // 读A的左下，采用列读的方式是为了更快将A后四行载入缓存
                x1 = A[i+4][y]; x2 = A[i+5][y]; x3 = A[i+6][y]; x4 = A[i+7][y];
                // 读B的右上，该操作完全命中缓存
                x5 = B[y][i+4]; x6 = B[y][i+5]; x7 = B[y][i+6]; x8 = B[y][i+7];
                
                // 将A的左下翻转到B的右上去，该操作完全命中缓存
                B[y][i+4] = x1; B[y][i+5] = x2; B[y][i+6] = x3; B[y][i+7] = x4;
                // 将B原本的右上移动到右下
                B[y+4][i] = x5; B[y+4][i+1] = x6; B[y+4][i+2] = x7; B[y+4][i+3] = x8;
            }
            // 这个循环做一件事：翻转A右下并放到B右下
            for (x = i + 4; x < i + 8; ++x)
            {
                x1 = A[x][j+4]; x2 = A[x][j+5]; x3 = A[x][j+6]; x4 = A[x][j+7];
                B[j+4][x] = x1; B[j+5][x] = x2; B[j+6][x] = x3; B[j+7][x] = x4;
            }
        }
}

void transpose_submit_61x67(int M, int N, int A[N][M], int B[M][N]) {
    int size=17;
    int temp;
    for (int n=0; n<N; n+=size) {
        for (int m=0; m<M; m+=size) {
            for (int i=n; i<n+size&&i<N; i++) {
                for (int j=m; j<m+size&&j<M; j++) {
                    temp = A[i][j];
                    B[j][i]=temp;
                }
            }
        }
    }
}

// void transpose_submit_64(int M, int N, int A[N][M], int B[M][N]) {
//     int i,j;
//     int temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
//     // 还是8x8分块，这个没有变
//     // n和m初始时一定指向8x8矩阵的左上顶端。
//     for (int n=0; n<N; n++) {
//         for (int m=0; m<M; m++) {
//             // 这个循环做两件事：翻转A的左上并放入B的左上，翻转A的右上并放入B的右上
//             // 这个循环走完后，缓存中剩余什么？B的前4行。
//             for(i=n; i<n+4; i++) {
//                 temp0=A[i][m];temp1=A[i][m+1];temp2=A[i][m+2];temp3=A[i][m+3];
//                 temp4=A[i][m+4];temp5=A[i][m+5];temp6=A[i][m+6];temp7=A[i][m+7];
//                 // 翻转A的左上并放入B的左上
//                 B[m][i]=temp0;B[m+1][i]=temp1;B[m+2][i]=temp2;B[m+3][i]=temp3;
//                 // 翻转A的右上并放入B的右上
//                 B[m][i+4]=temp4;B[m+1][i+4]=temp5;B[m+2][i+4]=temp6;B[m+3][i+4]=temp7;
//             }
//             // 这个循环做两件事：翻转A左下到B的右上；将B原本的右上移动到右下
//             for (j=m; j<m+4; j++) {
//                 // 读A的左下，采用列读的方式是为了更快将A后四行载入缓存
//                 temp0=A[n+4][j];temp1=A[n+5][j];temp2=A[n+6][j];temp3=A[n+7][j];
//                 // 读B的右上，该操作完全命中缓存
//                 temp4=B[j][n+4];temp5=B[j][n+5];temp6=B[j][n+6];temp7=B[j][n+7];

//                 // 将A的左下翻转到B的右上去，该操作完全命中缓存
//                 B[j][n+4]=temp0;B[j][n+5]=temp1;B[j][n+6]=temp2;B[j][n+7]=temp3;
//                 // 将B原本的右上移动到右下
//                 B[j+4][n]=temp4;B[j+4][n+1]=temp5;B[j+4][n+2]=temp6;B[j+4][n+3]=temp7;
//             }
//             // 这个循环做一件事：翻转A右下并放到B右下
//             for (i=n+4; i<n+8; i++) {
//                 temp0=A[i][m+4];temp1=A[i][m+5];temp2=A[i][m+6];temp3=A[i][m+7];
//                 B[m+4][i]=temp0;B[m+5][i]=temp1;B[m+6][i]=temp2;B[m+7][i]=temp3;
//             }
//         }
//     }
// }

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    // registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register your solution function 32 x 32 */
    registerTransFunction(transpose_submit_61x67, transpose_submit_desc); 

    /* Register any additional transpose functions */
    // registerTransFunction(trans, trans_desc); 
    // registerTransFunction(transpose_submit_32, transpose_submit_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

