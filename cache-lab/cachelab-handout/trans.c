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
    registerTransFunction(transpose_submit_32, "Transpose 32 x 32"); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

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

