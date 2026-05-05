#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 15
#define CHARSIZE 2
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define FEATURE_COUNT 10
#define HUMAN 0
#define AI 1
#define MAX_GROUPS 64
#define MAX_FORBIDDEN_DEPTH 4

void initRecordBorard(void);
void innerLayoutToDisplayArray(void);
void displayBoard(void);
void input(void);
int inBoard(int r, int c);
int isValidInput(int row, int col);
void changePlayer(void);
int win(int board[SIZE][SIZE], int check_r, int check_c, int player);
int forbiden(int board[SIZE][SIZE], int move_r, int move_c, int player);
int forbidenWithDepth(int board[SIZE][SIZE], int move_r, int move_c, int player, int depth);
int isLegalBlackMove(int board[SIZE][SIZE], int move_r, int move_c, int depth);
void getFeatures(int board[SIZE][SIZE], int move_r, int move_c, int player, int features[FEATURE_COUNT]);
int evaluatePoint(int board[SIZE][SIZE], int move_r, int move_c, int player);
void getAIMove(int board[SIZE][SIZE], int player, int *best_r, int *best_c);
void printUsage(const char *programName);
void parseArguments(int argc, char *argv[], int *mode, int *modeProvided, int *maxMoves);
void openGameLog(void);
void closeGameLog(void);
void logMove(int moveNumber, int player, int move_r, int move_c, const char *source);
void logForbiddenMove(int player, int move_r, int move_c, const char *source);
void logResult(const char *result);

char arrayForDisplayBoard[SIZE][SIZE * CHARSIZE + 1];

typedef struct {
	int stones[4][2];
	int winPoints[8][2];
	int winCount;
} FourGroup;

typedef struct {
	int stones[3][2];
} ThreeGroup;

char play1Pic = 'x';
char play1CurrentPic = '+';
char play2Pic = 'o';
char play2CurrentPic = '*';

int arrayForInnerBoardLayout[SIZE][SIZE];
int currentPlayer = BLACK;
int row = 7;
int col = 7;
int clearScreen = 1;
char logPath[260] = "game_log.txt";
FILE *logFile = NULL;

int playerType[3] = {0, HUMAN, AI};

int weights[FEATURE_COUNT] = {
	80065,    // features[0]: 成五
	-183603,  // features[1]: 长连，黑棋会被跳过，保留作惩罚项
	10793,    // features[2]: 活四
	1842,     // features[3]: 冲四
	2212,     // features[4]: 活三
	233,      // features[5]: 眠三
	31,       // features[6]: 活二
	59258,    // features[7]: 阻止对方成五
	6324,     // features[8]: 阻止对方的四
	10        // features[9]: 靠近中心
};

// 换对手
int opponent(int player) {
	if (player == BLACK) return WHITE;
	return BLACK;
}

// 下在棋盘内
int inBoard(int r, int c) {
	return r >= 0 && r < SIZE && c >= 0 && c < SIZE;
}

// 用户输入
void input(void) {
	printf("Player%i input row col: ", currentPlayer);
	scanf("%d %d", &row, &col);
	row--;
	col--;
}

// 计算一侧的长度
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

// 计算某方向两侧的总长度
int countLine(int board[SIZE][SIZE], int check_r, int check_c, int dr, int dc, int player) {
	return 1
		+ countOneSide(board, check_r, check_c, dr, dc, player)
		+ countOneSide(board, check_r, check_c, -dr, -dc, player);
}

// 判断（curr，curc）为核心形成的五连是否含有（mover，movec）即本轮落子
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
			// 表示成功
		}
	}

	return 0;
}

int win(int board[SIZE][SIZE], int check_r, int check_c, int player) {
	int dirs[4][2] = {
		{0, 1},
		{1, 0},
		{1, 1},
		{-1, 1}
	};
// 检查（checkr，checkc）处是否获胜 注意黑白不一样的处理逻辑
	for (int i = 0; i < 4; i++) {
		int count = countLine(board, check_r, check_c, dirs[i][0], dirs[i][1], player);
		if (player == BLACK && count == 5) return 1;
		if (player == WHITE && count >= 5) return 1;
	}

	return 0;
}

int longf(int board[SIZE][SIZE], int move_r, int move_c, int player) {
	int dirs[4][2] = {
		{0, 1},
		{1, 0},
		{1, 1},
		{-1, 1}
	};
// 长连禁手 大于五时要被禁止返回1
	for (int i = 0; i < 4; i++) {
		int count = countLine(board, move_r, move_c, dirs[i][0], dirs[i][1], player);
		if (count > 5) return 1;
	}

	return 0;
}

int samePoint(int r1, int c1, int r2, int c2) {
	return r1 == r2 && c1 == c2;
}

// 判断是否是同一个四连避免活四被重复计算
int sameFourGroup(FourGroup *group, int stones[4][2]) {
	for (int i = 0; i < 4; i++) {
		if (!samePoint(group->stones[i][0], group->stones[i][1], stones[i][0], stones[i][1])) {
			return 0;
		}
	}
	return 1;
}

// 判断当前四连能形成5连的地方（winpoint）是否已经在之前的四连的胜点中记载过
// 若发生重复则为1
int fourGroupHasWinPoint(FourGroup *group, int r, int c) {
	for (int i = 0; i < group->winCount; i++) {
		if (samePoint(group->winPoints[i][0], group->winPoints[i][1], r, c)) {
			return 1;
		}
	}
	return 0;
}


void addFourGroup(FourGroup groups[MAX_GROUPS], int *groupCount, int stones[4][2], int win_r, int win_c) {
	for (int i = 0; i < *groupCount; i++) {
		if (sameFourGroup(&groups[i], stones)) {
			// 这四个点已经重复出现
			if (!fourGroupHasWinPoint(&groups[i], win_r, win_c) && groups[i].winCount < 8) {
				// 但必胜点不重复且所有必胜点的数目不超过8
				groups[i].winPoints[groups[i].winCount][0] = win_r;
				groups[i].winPoints[groups[i].winCount][1] = win_c;
				groups[i].winCount++;
			}
			return;
		}
	}

	// 避免越界
	if (*groupCount >= MAX_GROUPS) return;

	// 新增连四
	for (int i = 0; i < 4; i++) {
		groups[*groupCount].stones[i][0] = stones[i][0];
		groups[*groupCount].stones[i][1] = stones[i][1];
	}
	groups[*groupCount].winPoints[0][0] = win_r;
	groups[*groupCount].winPoints[0][1] = win_c;
	groups[*groupCount].winCount = 1;
	(*groupCount)++;
}

// 判断某个四连中是否有（r，c）
int groupContainsPoint(FourGroup *group, int r, int c) {
	for (int i = 0; i < 4; i++) {
		if (samePoint(group->stones[i][0], group->stones[i][1], r, c)) {
			return 1;
		}
	}
	return 0;
}


int getFourGroups(int board[SIZE][SIZE], int move_r, int move_c, int player, FourGroup groups[MAX_GROUPS]) {
	int dirs[4][2] = {
		{0, 1},
		{1, 0},
		{1, 1},
		{-1, 1}
	};
	int groupCount = 0;

	for (int i = 0; i < 4; i++) {
		// 规定四个方向
		int dr = dirs[i][0];
		int dc = dirs[i][1];

		for (int start = -4; start <= 0; start++) {
			// 相当于相对当前落子起始位置的规定
			int playerCount = 0;
			// 自己的
			int emptyCount = 0;
			// 空位
			int stones[4][2];
			int win_r = -1;
			int win_c = -1;
			int blocked = 0;
			// 对方棋子或出界
			int containsMove = 0;
			// 包含本轮落子

			for (int k = 0; k < 5; k++) {
				// 当前位置的所有可能长度
				int r = move_r + (start + k) * dr;
				int c = move_c + (start + k) * dc;

				if (!inBoard(r, c)) {
					blocked = 1;
					break;
				}

				if (board[r][c] == player) {
					if (playerCount < 4) {
						stones[playerCount][0] = r;
						stones[playerCount][1] = c;
					}
					playerCount++;
					if (samePoint(r, c, move_r, move_c)) {
						containsMove = 1;
					}
				} else if (board[r][c] == EMPTY) {
					// 遇到空位就先记着 用playercount判断是否是真的
					emptyCount++;
					win_r = r;
					win_c = c;
				} else {
					blocked = 1;
					break;
				}
			}

			if (blocked || !containsMove || playerCount != 4 || emptyCount != 1) {
			// 被挡或者不包含本轮落子或者没形成连四或者没有空位可以填充都不形成活四或者冲四
				continue;
			}

			// 看看是不是真的能赢，真能赢再加，避免黑子长连等问题
			board[win_r][win_c] = player;
			if (win(board, win_r, win_c, player)) {
				addFourGroup(groups, &groupCount, stones, win_r, win_c);
			}
			board[win_r][win_c] = EMPTY;
		}
	}

	return groupCount;
}

// 判断四四禁手
int df(int board[SIZE][SIZE], int move_r, int move_c) {
	FourGroup groups[MAX_GROUPS];
	return getFourGroups(board, move_r, move_c, BLACK, groups) >= 2;
}

// 三同
int sameThreeGroup(ThreeGroup *group, int stones[3][2]) {
	for (int i = 0; i < 3; i++) {
		if (!samePoint(group->stones[i][0], group->stones[i][1], stones[i][0], stones[i][1])) {
			return 0;
		}
	}
	return 1;
}

void addThreeGroup(ThreeGroup groups[MAX_GROUPS], int *groupCount, int stones[3][2]) {
	for (int i = 0; i < *groupCount; i++) {
		if (sameThreeGroup(&groups[i], stones)) {
			return;
		}
	}

	if (*groupCount >= MAX_GROUPS) return;

	for (int i = 0; i < 3; i++) {
		groups[*groupCount].stones[i][0] = stones[i][0];
		groups[*groupCount].stones[i][1] = stones[i][1];
	}
	(*groupCount)++;
}

// 数活三的个数
int countLiveThreeGroups(int board[SIZE][SIZE], int move_r, int move_c, int player, int depth) {
	int dirs[4][2] = {
		{0, 1},
		{1, 0},
		{1, 1},
		{-1, 1}
	};
	ThreeGroup threeGroups[MAX_GROUPS];
	int threeCount = 0;

	for (int i = 0; i < 4; i++) {
		int dr = dirs[i][0];
		int dc = dirs[i][1];

		for (int offset = -4; offset <= 4; offset++) {
			// 假装在当前位置以外再延申一颗当前颜色的棋 以此判断是否形成活四来看原来是否是活三
			int ext_r = move_r + offset * dr;
			int ext_c = move_c + offset * dc;
			FourGroup fourGroups[MAX_GROUPS];
			int fourCount;


			// 判断可以放
			if (!inBoard(ext_r, ext_c) || board[ext_r][ext_c] != EMPTY) {
				continue;
			}

			// 当为黑方时还要额外判断是否被禁手限制
			if (player == BLACK && !isLegalBlackMove(board, ext_r, ext_c, depth + 1)) {
				continue;
			}

			// 检查通过，放棋
			board[ext_r][ext_c] = player;
			fourCount = getFourGroups(board, move_r, move_c, player, fourGroups);

			for (int g = 0; g < fourCount; g++) {
				int threeStones[3][2];
				int threeIndex = 0;

				// 只形成冲四不形成活四的不算活三
				if (fourGroups[g].winCount < 2) {
					continue;
				}
				// 不包含延申点和本轮落子的不算
				if (!groupContainsPoint(&fourGroups[g], ext_r, ext_c) ||
					!groupContainsPoint(&fourGroups[g], move_r, move_c)) {
					continue;
				}

				for (int s = 0; s < 4; s++) {
					// 除去延伸点其他店加入数组
					int stone_r = fourGroups[g].stones[s][0];
					int stone_c = fourGroups[g].stones[s][1];
					if (!samePoint(stone_r, stone_c, ext_r, ext_c) && threeIndex < 3) {
						threeStones[threeIndex][0] = stone_r;
						threeStones[threeIndex][1] = stone_c;
						threeIndex++;
					}
				}

				if (threeIndex == 3) {
					// 该组有三个时可以加入活三列表
					addThreeGroup(threeGroups, &threeCount, threeStones);
				}
			}

			// 撤回假设！
			board[ext_r][ext_c] = EMPTY;
		}
	}

	// 返回形成的活三数目
	return threeCount;
}

int dth(int board[SIZE][SIZE], int move_r, int move_c) {
	// 两个以上的活三是禁手
	return countLiveThreeGroups(board, move_r, move_c, BLACK, 0) >= 2;
}


// 聪明的使用depth避免无限次的递归判断是否被禁
int isLegalBlackMove(int board[SIZE][SIZE], int move_r, int move_c, int depth) {
	int legal;

	// 最基本的要求
	if (!inBoard(move_r, move_c) || board[move_r][move_c] != EMPTY) {
		return 0;
	}

	// 试一试 只要能赢就合法 
	board[move_r][move_c] = BLACK;
	if (win(board, move_r, move_c, BLACK)) {
		legal = 1;
	} else if (depth >= MAX_FORBIDDEN_DEPTH) {
		// 递归了 只判断长连和四四禁
		legal = !longf(board, move_r, move_c, BLACK) && !df(board, move_r, move_c);
	} else {
		// depth运算在这里！判断一次加一次
		legal = !forbidenWithDepth(board, move_r, move_c, BLACK, depth + 1);
	}
	// 现场还原
	board[move_r][move_c] = EMPTY;

	return legal;
}

int forbidenWithDepth(int board[SIZE][SIZE], int move_r, int move_c, int player, int depth) {
	if (player != BLACK) return 0;   //白的无所谓
	if (win(board, move_r, move_c, player)) return 0;  //赢了不管
	if (longf(board, move_r, move_c, player)) return 1;
	if (df(board, move_r, move_c)) return 1;
	if (depth >= MAX_FORBIDDEN_DEPTH) return 0;   //不判断三三了
	if (countLiveThreeGroups(board, move_r, move_c, BLACK, depth) >= 2) return 1;
	return 0;
}

// 禁手api
int forbiden(int board[SIZE][SIZE], int move_r, int move_c, int player) {
	return forbidenWithDepth(board, move_r, move_c, player, 0);
}


// 当前落子后某一个方向的空位数（0，1，2）
int openEndCount(int board[SIZE][SIZE], int move_r, int move_c, int dr, int dc, int player) {
	int count1 = countOneSide(board, move_r, move_c, dr, dc, player);
	int count2 = countOneSide(board, move_r, move_c, -dr, -dc, player);
	int open = 0;

	int r1 = move_r + (count1 + 1) * dr;
	int c1 = move_c + (count1 + 1) * dc;
	if (inBoard(r1, c1) && board[r1][c1] == EMPTY) open++;

	int r2 = move_r - (count2 + 1) * dr;
	int c2 = move_c - (count2 + 1) * dc;
	if (inBoard(r2, c2) && board[r2][c2] == EMPTY) open++;

	return open;
}

// 清零
void clearFeatures(int features[FEATURE_COUNT]) {
	for (int i = 0; i < FEATURE_COUNT; i++) {
		features[i] = 0;
	}
}

void addShapeFeatures(int board[SIZE][SIZE], int move_r, int move_c, int player, int features[FEATURE_COUNT]) {
	int dirs[4][2] = {
		{0, 1},
		{1, 0},
		{1, 1},
		{-1, 1}
	};

	for (int i = 0; i < 4; i++) {
		int dr = dirs[i][0];
		int dc = dirs[i][1];
		int count = countLine(board, move_r, move_c, dr, dc, player);
		int open = openEndCount(board, move_r, move_c, dr, dc, player);

		// 特征提取是简化了的逻辑
		if ((player == BLACK && count == 5) || (player == WHITE && count >= 5)) {
			features[0]++;
			// 胜利
		} else if (count > 5) {
			features[1]++;
			// 长连
		} else if (count == 4 && open == 2) {
			features[2]++;
			// 活四
		} else if (count == 4 && open == 1) {
			features[3]++;
			// 冲四
		} else if (count == 3 && open == 2) {
			features[4]++;
			// 活三
		} else if (count == 3 && open == 1) {
			features[5]++;
			// 眠三
		} else if (count == 2 && open == 2) {
			features[6]++;
			// 活二
		}
	}
}

void getFeatures(int board[SIZE][SIZE], int move_r, int move_c, int player, int features[FEATURE_COUNT]) {
	int opp = opponent(player);
	// 对手的特征图
	int oppFeatures[FEATURE_COUNT];

	clearFeatures(features);
	clearFeatures(oppFeatures);

	if (!inBoard(move_r, move_c) || board[move_r][move_c] != EMPTY) {
		return;
	}

	// 当前点自己下和对手下分别提取特征图
	board[move_r][move_c] = player;
	addShapeFeatures(board, move_r, move_c, player, features);
	board[move_r][move_c] = EMPTY;

	board[move_r][move_c] = opp;
	addShapeFeatures(board, move_r, move_c, opp, oppFeatures);
	board[move_r][move_c] = EMPTY;

	features[7] = oppFeatures[0];  //阻止成五
	features[8] = oppFeatures[2] + oppFeatures[3];  //阻止四

	int center = SIZE / 2;
	int distance = abs(move_r - center) + abs(move_c - center);
	features[9] = SIZE - distance;  //距离中心的距离
}


// 评估函数！
int evaluatePoint(int board[SIZE][SIZE], int move_r, int move_c, int player) {
	int features[FEATURE_COUNT];
	int score = 0;

	// 基本规则否认点
	if (!inBoard(move_r, move_c) || board[move_r][move_c] != EMPTY) {
		return -1000000000;
	}

	// 禁手否认点
	board[move_r][move_c] = player;
	if (player == BLACK && !win(board, move_r, move_c, player) && forbiden(board, move_r, move_c, player)) {
		board[move_r][move_c] = EMPTY;
		return -1000000000;
	}
	board[move_r][move_c] = EMPTY;

	// 成
	getFeatures(board, move_r, move_c, player, features);
	for (int i = 0; i < FEATURE_COUNT; i++) {
		score += weights[i] * features[i];
	}

	return score;
}

// 遍历所有点找评分最高的点
void getAIMove(int board[SIZE][SIZE], int player, int *best_r, int *best_c) {
	int bestScore = -1000000000;
	*best_r = SIZE / 2;
	*best_c = SIZE / 2;

	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			int score = evaluatePoint(board, i, j, player);
			if (score > bestScore) {
				bestScore = score;
				*best_r = i;
				*best_c = j;
			}
		}
	}
}


int isValidInput(int row, int col) {
	if (!inBoard(row, col)) {
		printf("Invalid place! Please input again.\n");
		return 1;
	}

	if (arrayForInnerBoardLayout[row][col] != EMPTY) {
		printf("This place already has a piece.\n");
		return 1;
	}

	return 0;
}

void changePlayer(void) {
	currentPlayer = opponent(currentPlayer);
}

int isBoardFull(int board[SIZE][SIZE]) {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (board[i][j] == EMPTY) return 0;
		}
	}
	return 1;
}

void printUsage(const char *programName) {
	printf("Usage: %s [--mode 1|2] [--log FILE] [--no-clear] [--max-moves N]\n", programName);
	printf("  --mode 1      Human(BLACK) vs AI(WHITE)\n");
	printf("  --mode 2      AI vs AI\n");
	printf("  --log FILE    Write every accepted move and board to FILE\n");
	printf("  --no-clear    Keep board history visible in the terminal\n");
	printf("  --max-moves N Stop after N accepted moves if nobody wins\n");
}

void parseArguments(int argc, char *argv[], int *mode, int *modeProvided, int *maxMoves) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
			*mode = atoi(argv[++i]);
			*modeProvided = 1;
		} else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
			strncpy(logPath, argv[++i], sizeof(logPath) - 1);
			logPath[sizeof(logPath) - 1] = '\0';
		} else if (strcmp(argv[i], "--no-clear") == 0) {
			clearScreen = 0;
		} else if (strcmp(argv[i], "--max-moves") == 0 && i + 1 < argc) {
			*maxMoves = atoi(argv[++i]);
			if (*maxMoves <= 0 || *maxMoves > SIZE * SIZE) {
				*maxMoves = SIZE * SIZE;
			}
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			printUsage(argv[0]);
			exit(0);
		} else {
			printf("Unknown argument: %s\n", argv[i]);
			printUsage(argv[0]);
			exit(1);
		}
	}

	if (*mode != 1 && *mode != 2) {
		*mode = 1;
	}
}

void openGameLog(void) {
	logFile = fopen(logPath, "w");
	if (logFile == NULL) {
		printf("Warning: cannot open log file %s\n", logPath);
		return;
	}

	fprintf(logFile, "Gomoku game log\n");
	fprintf(logFile, "Board size: %d x %d\n", SIZE, SIZE);
	fprintf(logFile, "Weights:");
	for (int i = 0; i < FEATURE_COUNT; i++) {
		fprintf(logFile, " %d", weights[i]);
	}
	fprintf(logFile, "\n\n");
	fflush(logFile);
}

void closeGameLog(void) {
	if (logFile != NULL) {
		fclose(logFile);
		logFile = NULL;
	}
}

void logBoard(void) {
	if (logFile == NULL) return;

	fprintf(logFile, "   ");
	for (int j = 0; j < SIZE; j++) {
		fprintf(logFile, " %2d", j + 1);
	}
	fprintf(logFile, "\n");

	for (int i = 0; i < SIZE; i++) {
		fprintf(logFile, "%2d ", i + 1);
		for (int j = 0; j < SIZE; j++) {
			char ch = '.';
			if (arrayForInnerBoardLayout[i][j] == BLACK) {
				ch = play1Pic;
			} else if (arrayForInnerBoardLayout[i][j] == WHITE) {
				ch = play2Pic;
			}
			fprintf(logFile, "  %c", ch);
		}
		fprintf(logFile, "\n");
	}
	fprintf(logFile, "\n");
}

void logMove(int moveNumber, int player, int move_r, int move_c, const char *source) {
	if (logFile == NULL) return;

	fprintf(
		logFile,
		"Move %03d: Player%d %s row=%d col=%d source=%s\n",
		moveNumber,
		player,
		player == BLACK ? "BLACK" : "WHITE",
		move_r + 1,
		move_c + 1,
		source
	);
	logBoard();
	fflush(logFile);
}

void logForbiddenMove(int player, int move_r, int move_c, const char *source) {
	if (logFile == NULL) return;

	fprintf(
		logFile,
		"Rejected forbidden move: Player%d %s row=%d col=%d source=%s\n\n",
		player,
		player == BLACK ? "BLACK" : "WHITE",
		move_r + 1,
		move_c + 1,
		source
	);
	fflush(logFile);
}

void logResult(const char *result) {
	if (logFile == NULL) return;
	fprintf(logFile, "Result: %s\n", result);
	fflush(logFile);
}


// 主函数！
int main(int argc, char *argv[]) {
	int mode = 1;
	int modeProvided = 0;
	int maxMoves = SIZE * SIZE;
	int moveNumber = 0;
	initRecordBorard();
	parseArguments(argc, argv, &mode, &modeProvided, &maxMoves);
	openGameLog();

	if (!modeProvided) {
		printf("Choose mode: 1 Human(BLACK) vs AI(WHITE), 2 AI vs AI: ");
		if (scanf("%d", &mode) != 1) {
			mode = 1;
		}
	}

	if (mode == 2) {
		playerType[BLACK] = AI;
		playerType[WHITE] = AI;
	} else {
		playerType[BLACK] = HUMAN;
		playerType[WHITE] = AI;
	}
	if (logFile != NULL) {
		fprintf(logFile, "Mode: %d\n", mode);
		fprintf(logFile, "Max moves: %d\n\n", maxMoves);
		fflush(logFile);
	}

	// 一直循环
	while (1) {
		innerLayoutToDisplayArray();
		displayBoard();

		if (playerType[currentPlayer] == AI) {
			getAIMove(arrayForInnerBoardLayout, currentPlayer, &row, &col);
			printf("AI Player%i: %d %d\n", currentPlayer, row + 1, col + 1);
		} else {
			input();
			if (isValidInput(row, col) == 1) continue;
		}

		if (arrayForInnerBoardLayout[row][col] != EMPTY) {
			printf("No valid move found.\n");
			logResult("No valid move found");
			closeGameLog();
			return 0;
		}

		arrayForInnerBoardLayout[row][col] = currentPlayer;

		if (win(arrayForInnerBoardLayout, row, col, currentPlayer) == 1) {
			moveNumber++;
			logMove(moveNumber, currentPlayer, row, col, playerType[currentPlayer] == AI ? "AI" : "HUMAN");
			innerLayoutToDisplayArray();
			displayBoard();
			printf("Player%i wins!\n", currentPlayer);
			logResult(currentPlayer == BLACK ? "BLACK wins!" : "WHITE wins!");
			closeGameLog();
			return 0;
		}

		if (forbiden(arrayForInnerBoardLayout, row, col, currentPlayer) == 1) {
			arrayForInnerBoardLayout[row][col] = EMPTY;
			logForbiddenMove(currentPlayer, row, col, playerType[currentPlayer] == AI ? "AI" : "HUMAN");
			if (playerType[currentPlayer] == HUMAN) {
				printf("Attention forbidden rules! Please input again.\n");
			}
			continue;
		}

		moveNumber++;
		logMove(moveNumber, currentPlayer, row, col, playerType[currentPlayer] == AI ? "AI" : "HUMAN");

		if (isBoardFull(arrayForInnerBoardLayout)) {
			printf("Draw!\n");
			logResult("Draw");
			closeGameLog();
			return 0;
		}

		if (moveNumber >= maxMoves) {
			printf("Move limit reached.\n");
			logResult("Move limit reached");
			closeGameLog();
			return 0;
		}

		changePlayer();
	}
}

// 初始化棋盘
void initRecordBorard(void) {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			arrayForInnerBoardLayout[i][j] = EMPTY;
		}
	}
}

// 展示
void innerLayoutToDisplayArray(void) {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			arrayForDisplayBoard[i][2 * j] = '.';
			if (j < SIZE - 1) {
				arrayForDisplayBoard[i][2 * j + 1] = ' ';
			}
		}
		arrayForDisplayBoard[i][SIZE * CHARSIZE - 1] = '\0';
	}

	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (arrayForInnerBoardLayout[i][j] == BLACK) {
				arrayForDisplayBoard[i][2 * j] = play1Pic;
			} else if (arrayForInnerBoardLayout[i][j] == WHITE) {
				arrayForDisplayBoard[i][2 * j] = play2Pic;
			}
		}
	}

	if (inBoard(row, col) && arrayForInnerBoardLayout[row][col] == EMPTY) {
		if (currentPlayer == BLACK) {
			arrayForDisplayBoard[row][2 * col] = play1CurrentPic;
		} else if (currentPlayer == WHITE) {
			arrayForDisplayBoard[row][2 * col] = play2CurrentPic;
		}
	}
}

void displayBoard(void) {
	if (clearScreen) {
		system("cls");
	}

	printf("   ");
	for (int j = 0; j < SIZE; j++) {
		printf(" %2d", j + 1);
	}
	printf("\n");

	for (int i = 0; i < SIZE; i++) {
		printf("%2d ", i + 1);
		for (int j = 0; j < SIZE; j++) {
			printf("  %c", arrayForDisplayBoard[i][2 * j]);
		}
		printf("\n");
	}
}
