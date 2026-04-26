#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define SIZE 15
#define CHARSIZE 2
void initRecordBorard(void);
void innerLayoutToDisplayArray(void);

void displayBoard(void);
int currentPlayer = 1;

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

int col, row;

void input(void){
	printf("Player%i: ",currentPlayer);
	scanf("%d %d",&row, &col);
	row --;
	col --;
}

//此数组用于记录当前的棋盘的格局 
int arrayForInnerBoardLayout[SIZE][SIZE];

int inBoard(int r, int c) {
    return r >= 0 && r < SIZE && c >= 0 && c < SIZE;
}

int countOneSide(int board[SIZE][SIZE], int dr, int dc) {
    int count = 0;

    row += dr;
    col += dc;

    while (inBoard(row, col) && board[row][col] == currentPlayer) {
        count++;
        row += dr;
        col += dc;
    }
	return count;
}

int countLine(int board[SIZE][SIZE], int dr, int dc) {
    return 1+countOneSide(board, dr, dc)+countOneSide(board, -dr, -dc);
}

int win(int board[SIZE][SIZE]) {
    int dirs[4][2] = {
        {0, 1},   // col
        {1, 0},   // row
        {1, 1},   // duijiao
        {-1, 1}   
    };
    for (int i = 0; i < 4; i++) {
        int count = countLine(board, dirs[i][0],dirs[i][1]);
        if (currentPlayer == 1 && count == 5) return 1;   
		else if (count >= 5) return 1;  
    }
	return 0;
}

int longf(int board[SIZE][SIZE]){
	int dirs[4][2] = {
        {0, 1},   // col
        {1, 0},   // row
        {1, 1},   // duijiao
        {-1, 1}   
    };
    for (int i = 0; i < 4; i++) {
        int count = countLine(board, dirs[i][0],dirs[i][1]);
        if (count > 5) {
            printf("Attention long forbidden rules! Please input again.\n");
            return 1;
        } 
	}
	return 0;
}


int df(int board[SIZE][SIZE]){
	
}


int dth(int board[SIZE][SIZE]){
	return 0;
}

int forbiden(int board[SIZE][SIZE]){
	// 形成禁手为1，合法则为0
	if (longf(board)==0 && df(board) == 0 && dth(board) == 0) return 0;
	return 1;
}

int isValidInput(int row, int col){
	if (row < 0 || row >= SIZE || col < 0 || col >= SIZE) {
        printf("Invalid place! Please input again.\n");
		return 1;
	}
	else if (arrayForInnerBoardLayout[row][col] != 0){
		printf("This place already has a piece.\n");
		return 1;
	}
	return 0;
	// 0是可以被接受的
}

void changePlayer(void) {
    if (currentPlayer == 1) {
        currentPlayer = 2;
    } else {
        currentPlayer = 1;
    }
}


int main()
{
	initRecordBorard();

	while(1){
	innerLayoutToDisplayArray();
	displayBoard();
	
	input();

	if (isValidInput(row, col) == 1)  continue;
	
	arrayForInnerBoardLayout[row][col] = currentPlayer;
	if (win(arrayForInnerBoardLayout) == 1) {
		printf("player%i wins!", currentPlayer);
		return 0;
	}
	if (currentPlayer == 1 && forbiden(arrayForInnerBoardLayout) == 1){
		arrayForInnerBoardLayout[row][col] = 0;
		continue;
		}
		changePlayer(); 
	}

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
			if (arrayForInnerBoardLayout[i][j] == 1) arrayForDisplayBoard[i][2*j] = play1Pic;//黑棋子;
			else if (arrayForInnerBoardLayout[i][j] == 2) arrayForDisplayBoard[i][2*j] = play2Pic;
		}
	}
	if (currentPlayer == 1) arrayForDisplayBoard[row][2*col] = play1CurrentPic;
	else if (currentPlayer == 2) arrayForDisplayBoard[row][2*col] = play2CurrentPic;
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
