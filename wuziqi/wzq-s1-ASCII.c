#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define SIZE 15
#define CHARSIZE 2
void initRecordBorard(void);
void innerLayoutToDisplayArray(void);
void displayBoard(void);

//棋盘使用的是ASCII编码，每一个位置包含一个空格和一个字符，因此占2个字节。

//空棋盘模板 
char arrayForEmptyBoard[SIZE][SIZE*CHARSIZE] = 
{
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
		". . . . . . . . . . . . . . .",
};
//此数组存储用于显示的棋盘 
char arrayForDisplayBoard[SIZE][SIZE*CHARSIZE+1];
 
char play1Pic='x';//黑棋子;
char play1CurrentPic='+'; 

char play2Pic='o';//白棋子;
char play2CurrentPic='*';

//此数组用于记录当前的棋盘的格局 
int arrayForInnerBoardLayout[SIZE][SIZE];

int main()

{

    initRecordBorard();    //初始化一个空棋盘

    arrayForInnerBoardLayout[0][0]=1;    //在棋盘的左上角落一个黑色棋子
    innerLayoutToDisplayArray();  //将心中的棋盘转成用于显示的棋盘
    displayBoard();          //显示棋盘
    getchar();   

    arrayForInnerBoardLayout[5][9]=2;
    innerLayoutToDisplayArray();
    displayBoard();
    getchar();

    arrayForInnerBoardLayout[3][4]=2;
    innerLayoutToDisplayArray();
    displayBoard();
    getchar();

    arrayForInnerBoardLayout[6][1]=1;
    innerLayoutToDisplayArray();
    displayBoard();
    getchar();

    arrayForInnerBoardLayout[9][4]=2;
    innerLayoutToDisplayArray();
    displayBoard();
    getchar();
    return 0;
}

//初始化一个空棋盘格局 
void initRecordBorard(void){
	//通过双重循环，将arrayForInnerBoardLayout清0
	for (int i = 0; i < SIZE; i++){
		for (int j = 0; j < SIZE; j++){
			arrayForInnerBoardLayout[i][j] = 0;
		}
	}
}


//将arrayForInnerBoardLayout中记录的棋子位置，转化到arrayForDisplayBoard中
void innerLayoutToDisplayArray(void){
//第一步：将arrayForEmptyBoard中记录的空棋盘，复制到arrayForDisplayBoard中
    for (int i = 0; i < SIZE; i++){
		for (int j = 0; j < SIZE; j++){
			if (j == SIZE-1){
				arrayForDisplayBoard[i][2*j] = '.';
				arrayForDisplayBoard[i][2*j] = '\0';
			} 
			arrayForDisplayBoard[i][2*j] = '.';
			arrayForDisplayBoard[i][2*j+1] = ' ';
		}
	}
//第二步：扫描arrayForInnerBoardLayout，当遇到非0的元素，将+或者*复制到arrayForDisplayBoard的相应位置上
    for (int i = 0; i < SIZE; i++){
		for (int j = 0; j < SIZE; j++){
			if (arrayForInnerBoardLayout[i][j] == 1) arrayForDisplayBoard[i][2*j] = '+';
			else if (arrayForInnerBoardLayout[i][j] == 2) arrayForDisplayBoard[i][2*j] = '*';
		}
	}
}


//显示棋盘格局 
void displayBoard(void){
	int i;
	//第一步：清屏
	system("clear");   //清屏  
	//第二步：将arrayForDisplayBoard输出到屏幕上
	for (int i = 0; i < SIZE; i++){
		printf("%s\n", arrayForDisplayBoard[i]);
	}

	
	//第三步：输出最下面的一行字母A B .... 
	for (int j = 0; j < SIZE; j++) {
        printf("%c", 'A' + j);
        if (j < SIZE - 1) {
            printf(" ");
        }
	}
} 


