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

int countOneSide(int board[SIZE][SIZE], int start_r, int start_c, int dr, int dc, int player) {
    int count = 0;
    int cur_r = start_r + dr;
    int cur_c = start_c + dc;

    while (inBoard(cur_r, cur_c) && board[cur_r][cur_c] == player) {
        count++;
        cur_r += dr;
        cur_c += dc;
    }
	return count;
}

int countLine(int board[SIZE][SIZE], int check_r, int check_c, int dr, int dc, int player) {
    return 1
        + countOneSide(board, check_r, check_c, dr, dc, player)
        + countOneSide(board, check_r, check_c, -dr, -dc, player);
}

int isExactFiveInDirection(int board[SIZE][SIZE], int check_r, int check_c, int dr, int dc, int player) {
    return countLine(board, check_r, check_c, dr, dc, player) == 5;
}

int isExactFiveIncludeMove(int board[SIZE][SIZE], int cur_r, int cur_c, int move_r, int move_c, int dr, int dc, int player) {
    int count1 = countOneSide(board, cur_r, cur_c, dr, dc, player);
    int count2 = countOneSide(board, cur_r, cur_c, -dr, -dc, player);

    if (count1 + count2 + 1 != 5) {
        return 0;
    }

    for (int k = -count2; k <= count1; k++) {
        int r = cur_r + k * dr;
        int c = cur_c + k * dc;

        if (r == move_r && c == move_c) {
            return 1;
        }
    }

    return 0;
}

int win(int board[SIZE][SIZE], int check_r, int check_c, int player) {
    int dirs[4][2] = {
        {0, 1},   // col
        {1, 0},   // row
        {1, 1},   // duijiao
        {-1, 1}   
    };
    for (int i = 0; i < 4; i++) {
        int count = countLine(board, check_r, check_c, dirs[i][0], dirs[i][1], player);
        if (player == 1 && count == 5) return 1;
		else if (player == 2 && count >= 5) return 1;
    }
	return 0;
}

int longf(int board[SIZE][SIZE], int check_r, int check_c, int player){
	int dirs[4][2] = {
        {0, 1},   // col
        {1, 0},   // row
        {1, 1},   // duijiao
        {-1, 1}   
    };
    for (int i = 0; i < 4; i++) {
        int count = countLine(board, check_r, check_c, dirs[i][0], dirs[i][1], player);
        if (count > 5) {
            printf("Attention long forbidden rules! Please input again.\n");
            return 1;
        } 
	}
	return 0;
}


int hasFourInDirection(int board[SIZE][SIZE], int move_r, int move_c, int dr, int dc) {
	for (int k = -4; k <= 4; k++) {
		int cur_r = move_r + k * dr;
		int cur_c = move_c + k * dc;

		if (inBoard(cur_r, cur_c)) {
			if (board[cur_r][cur_c] == 0){
				board[cur_r][cur_c] = 1;
				if (isExactFiveIncludeMove(board, cur_r, cur_c, move_r, move_c, dr, dc, 1)) {
					board[cur_r][cur_c] = 0;
					return 1;
				}
				board[cur_r][cur_c] = 0;
			}
		}	
	}
	return 0;
}

int df(int board[SIZE][SIZE], int move_r, int move_c){
	int dirs[4][2] = {{0,1}, {1,0}, {1,1}, {-1,1}};
	int count = 0;
	for (int i = 0; i < 4; i++) {
		if (hasFourInDirection(board, move_r, move_c, dirs[i][0], dirs[i][1])) {
			count++;
		}
	}
	if (count >= 2) {
		printf("Attention double-four forbidden rules! Please input again.\n");
		return 1;
	}
	return 0;
}


int dth(int board[SIZE][SIZE], int move_r, int move_c){
	return 0;
}

int forbiden(int board[SIZE][SIZE], int move_r, int move_c, int player){
	// 形成禁手为1，合法则为0
	if (player != 1) return 0;
	if (longf(board, move_r, move_c, player)==0 && df(board, move_r, move_c) == 0 && dth(board, move_r, move_c) == 0) return 0;
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
	if (win(arrayForInnerBoardLayout, row, col, currentPlayer) == 1) {
		printf("player%i wins!", currentPlayer);
		return 0;
	}
	if (forbiden(arrayForInnerBoardLayout, row, col, currentPlayer) == 1){
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
			arrayForDisplayBoard[i][2*j] = '.';
			if (j < SIZE - 1) {
				arrayForDisplayBoard[i][2*j+1] = ' ';
			}
		}
		arrayForDisplayBoard[i][SIZE * CHARSIZE - 1] = '\0';
	}
//第二步：扫描arrayForInnerBoardLayout，当遇到非0的元素，将+或者*复制到arrayForDisplayBoard的相应位置上
    for (int i = 0; i < SIZE; i++){
		for (int j = 0; j < SIZE; j++){
			if (arrayForInnerBoardLayout[i][j] == 1) arrayForDisplayBoard[i][2*j] = play1Pic;//黑棋子;
			else if (arrayForInnerBoardLayout[i][j] == 2) arrayForDisplayBoard[i][2*j] = play2Pic;
		}
	}
	if (inBoard(row, col) && arrayForInnerBoardLayout[row][col] == 0) {
		if (currentPlayer == 1) arrayForDisplayBoard[row][2*col] = play1CurrentPic;
		else if (currentPlayer == 2) arrayForDisplayBoard[row][2*col] = play2CurrentPic;
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
