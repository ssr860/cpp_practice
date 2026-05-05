# 后续改进方向：优化scores的计算方式 增加区分度（如利用steps）

import argparse
import json
import random
from pathlib import Path

SIZE = 15
EMPTY = 0
BLACK = 1
WHITE = 2
FEATURE_COUNT = 10
DIRS = ((0, 1), (1, 0), (1, 1), (-1, 1))
MAX_FORBIDDEN_DEPTH = 1
FORBIDDEN_CACHE = {}

BASE_WEIGHTS = [
    100000,   # self win
    -50000,   # self overline
    12000,    # self live four
    6000,     # self blocked four
    1500,     # self live three
    300,      # self blocked three
    80,       # self live two
    90000,    # block opponent win
    10000,    # block opponent four
    10,       # center value
]


def opponent(player):
    return WHITE if player == BLACK else BLACK


def board_key(board):
    return tuple(tuple(row) for row in board)


def in_board(r, c):
    return 0 <= r < SIZE and 0 <= c < SIZE


def count_one_side(board, start_r, start_c, dr, dc, player):
    count = 0
    cur_r = start_r + dr
    cur_c = start_c + dc
    while in_board(cur_r, cur_c) and board[cur_r][cur_c] == player:
        count += 1
        cur_r += dr
        cur_c += dc
    return count


def count_line(board, r, c, dr, dc, player):
    return (
        1 + count_one_side(board, r, c, dr, dc, player) + count_one_side(board, r, c, -dr, -dc, player)
    )


def win(board, r, c, player):
    for dr, dc in DIRS:
        count = count_line(board, r, c, dr, dc, player)
        if player == BLACK and count == 5:
            return True
        if player == WHITE and count >= 5:
            return True
    return False


def long_forbidden(board, r, c, player):
    if player != BLACK:
        return False
    for dr, dc in DIRS:
        if count_line(board, r, c, dr, dc, player) > 5:
            return True
    return False


def four_groups(board, move_r, move_c, player):
    groups = {}
    for dr, dc in DIRS:
        for start in range(-4, 1):
            cells = [(move_r + (start + i) * dr, move_c + (start + i) * dc) for i in range(5)]
            if not all(in_board(r, c) for r, c in cells):
                continue

            player_cells = []
            empty_cells = []
            blocked = False
            for r, c in cells:
                if board[r][c] == player:
                    player_cells.append((r, c))
                elif board[r][c] == EMPTY:
                    empty_cells.append((r, c))
                else:
                    blocked = True
                    break
            if blocked or len(player_cells) != 4 or len(empty_cells) != 1:
                continue
            if (move_r, move_c) not in player_cells:
                continue

            win_r, win_c = empty_cells[0]
            board[win_r][win_c] = player
            makes_five = win(board, win_r, win_c, player)
            board[win_r][win_c] = EMPTY
            if not makes_five:
                continue

            key = tuple(player_cells)
            groups.setdefault(key, set()).add((win_r, win_c))
    return groups


def double_four(board, r, c):
    return len(four_groups(board, r, c, BLACK)) >= 2


def legal_black_move(board, r, c, depth):
    if not in_board(r, c) or board[r][c] != EMPTY:
        return False

    board[r][c] = BLACK
    if win(board, r, c, BLACK):
        legal = True
    elif depth >= MAX_FORBIDDEN_DEPTH:
        legal = not long_forbidden(board, r, c, BLACK) and not double_four(board, r, c)
    else:
        legal = not forbidden(board, r, c, BLACK, depth + 1)
    board[r][c] = EMPTY
    return legal


def live_three_groups(board, move_r, move_c, player, depth=0):
    groups = set()
    for dr, dc in DIRS:
        for offset in range(-4, 5):
            ext_r = move_r + offset * dr
            ext_c = move_c + offset * dc
            if not in_board(ext_r, ext_c) or board[ext_r][ext_c] != EMPTY:
                continue
            if player == BLACK and not legal_black_move(board, ext_r, ext_c, depth + 1):
                continue

            board[ext_r][ext_c] = player
            for four_key, win_points in four_groups(board, move_r, move_c, player).items():
                if len(win_points) < 2:
                    continue
                if (ext_r, ext_c) not in four_key or (move_r, move_c) not in four_key:
                    continue
                three_key = tuple(cell for cell in four_key if cell != (ext_r, ext_c))
                if len(three_key) == 3 and (move_r, move_c) in three_key:
                    groups.add(three_key)
            board[ext_r][ext_c] = EMPTY
    return groups


def double_three(board, r, c, depth=0):
    return len(live_three_groups(board, r, c, BLACK, depth)) >= 2


def forbidden(board, r, c, player, depth=0):
    key = (board_key(board), r, c, player, depth)
    if key in FORBIDDEN_CACHE:
        return FORBIDDEN_CACHE[key]

    if player != BLACK:
        FORBIDDEN_CACHE[key] = False
        return False
    if win(board, r, c, player):
        FORBIDDEN_CACHE[key] = False
        return False
    if long_forbidden(board, r, c, player) or double_four(board, r, c):
        FORBIDDEN_CACHE[key] = True
        return True
    if depth >= MAX_FORBIDDEN_DEPTH:
        FORBIDDEN_CACHE[key] = False
        return False

    result = double_three(board, r, c, depth)
    FORBIDDEN_CACHE[key] = result
    return result


def open_end_count(board, r, c, dr, dc, player):
    count1 = count_one_side(board, r, c, dr, dc, player)
    count2 = count_one_side(board, r, c, -dr, -dc, player)
    open_ends = 0
    r1 = r + (count1 + 1) * dr
    c1 = c + (count1 + 1) * dc
    r2 = r - (count2 + 1) * dr
    c2 = c - (count2 + 1) * dc
    if in_board(r1, c1) and board[r1][c1] == EMPTY:
        open_ends += 1
    if in_board(r2, c2) and board[r2][c2] == EMPTY:
        open_ends += 1
    return open_ends


def add_shape_features(board, r, c, player, features):
    for dr, dc in ((0, 1), (1, 0), (1, 1), (-1, 1)):
        count = count_line(board, r, c, dr, dc, player)
        open_ends = open_end_count(board, r, c, dr, dc, player)
        if (player == BLACK and count == 5) or (player == WHITE and count >= 5):
            features[0] += 1
        elif count > 5:
            features[1] += 1
        elif count == 4 and open_ends == 2:
            features[2] += 1
        elif count == 4 and open_ends == 1:
            features[3] += 1
        elif count == 3 and open_ends == 2:
            features[4] += 1
        elif count == 3 and open_ends == 1:
            features[5] += 1
        elif count == 2 and open_ends == 2:
            features[6] += 1


def get_features(board, r, c, player):
    features = [0] * FEATURE_COUNT
    opp_features = [0] * FEATURE_COUNT
    if not in_board(r, c) or board[r][c] != EMPTY:
        return features

    board[r][c] = player
    add_shape_features(board, r, c, player, features)
    board[r][c] = EMPTY

    opp = opponent(player)
    board[r][c] = opp
    add_shape_features(board, r, c, opp, opp_features)
    board[r][c] = EMPTY

    features[7] = opp_features[0]
    features[8] = opp_features[2] + opp_features[3]
    center = SIZE // 2
    features[9] = SIZE - (abs(r - center) + abs(c - center))
    return features


def evaluate_point(board, r, c, player, weights):
    if not in_board(r, c) or board[r][c] != EMPTY:
        return -10**12
    board[r][c] = player
    if player == BLACK and not win(board, r, c, player) and forbidden(board, r, c, player):
        board[r][c] = EMPTY
        return -10**12
    board[r][c] = EMPTY
    features = get_features(board, r, c, player)
    return sum(w * f for w, f in zip(weights, features))


# 只在有棋子的附近两格做决策，避免重复扫描运算
def candidate_points(board):
    points = []
    has_piece = False
    for r in range(SIZE):
        for c in range(SIZE):
            if board[r][c] != EMPTY:
                has_piece = True
                # 已经有棋子了
                for dr in range(-2, 3):
                    for dc in range(-2, 3):
                        nr, nc = r + dr, c + dc
                        if in_board(nr, nc) and board[nr][nc] == EMPTY:
                            points.append((nr, nc))
    if not has_piece:
        return [(SIZE // 2, SIZE // 2)]
    return list(set(points))


def ai_move(board, player, weights, random_rate=0.03):
    points = candidate_points(board)
    if random.random() < random_rate:
        random.shuffle(points)
        for r, c in points:
            score = evaluate_point(board, r, c, player, weights)
            if score > -10**11:
                return r, c

    best_score = -10**12
    best_points = []
    for r, c in points:
        score = evaluate_point(board, r, c, player, weights)
        if score > best_score:
            best_score = score
            best_points = [(r, c)]
        elif score == best_score:
            best_points.append((r, c))
    return random.choice(best_points) if best_points else (SIZE // 2, SIZE // 2)


def play_game(black_weights, white_weights, max_steps=225, random_rate=0.03):
    FORBIDDEN_CACHE.clear()
    board = [[EMPTY for _ in range(SIZE)] for _ in range(SIZE)]
    player = BLACK
    last_move = None

    for step in range(max_steps):
        weights = black_weights if player == BLACK else white_weights
        r, c = ai_move(board, player, weights, random_rate=random_rate)
        if board[r][c] != EMPTY:
            return opponent(player)
        board[r][c] = player
        last_move = (r, c)
        if win(board, r, c, player):
            return player
        if player == BLACK and forbidden(board, r, c, player):
            return WHITE
        player = opponent(player)
    return 0 if last_move else WHITE


# 随机扰动
def mutate(weights, scale):
    new_weights = []
    for i, w in enumerate(weights):
        if i == 0 or i == 7:
            # 这两个有决定性 扰动小
            ratio = 0.08
        else:
            ratio = scale
        # 改变量
        delta = int(abs(w) * random.uniform(-ratio, ratio))
        # 限制范围
        new_weights.append(max(-200000, min(200000, w + delta)))
    return new_weights


# 新参数和原来的最佳参数各用黑白棋走一轮得出新参数的score
def score_weights(weights, baseline, games, random_rate):
    score = 0
    for _ in range(games):
        result = play_game(weights, baseline, random_rate=random_rate)
        if result == BLACK:
            score += 1
        elif result == WHITE:
            score -= 1
        result = play_game(baseline, weights, random_rate=random_rate)
        if result == WHITE:
            score += 1
        elif result == BLACK:
            score -= 1
    return score

# 有记忆训练
def load_history(out_dir):
    log_path = out_dir / "training_log.json"
    if not log_path.exists():
        return []
    return json.loads(log_path.read_text(encoding="utf-8"))


# 自博弈 + 随机变异搜索
def train(generations, population_size, games, seed, out_dir, resume=False, random_rate=0.03):
# generations      训练多少代
# population_size  每一代生成多少组候选参数
# games            每组候选和基准参数对战多少轮
# out_dir          输出目录
# resume           是否从已有训练记录继续训练
# random_rate      AI 随机下棋概率
    random.seed(seed)
    best = BASE_WEIGHTS[:]
    history = load_history(out_dir) if resume else []
    start_gen = 1

    if history:
        last = history[-1]
        best = last["weights"]
        start_gen = int(last["generation"]) + 1
        print(f"resuming from generation {last['generation']}: weights={best}", flush=True)

    for gen in range(start_gen, generations + 1):
        # 训练早期大幅判断 晚期逐步收敛
        scale = max(0.05, 0.35 * (1 - gen / max(1, generations)))
        population = [best]
        while len(population) < population_size:
            population.append(mutate(best, scale))

        print(
            f"generation {gen:03d}: evaluating {population_size} candidates "
            f"with {games * 2} games each...",
            flush=True,
        )
        # 保存每组参数的成绩
        scored = []
        for weights in population:
            # 保存形式（score，参数）
            scored.append((score_weights(weights, best, games, random_rate), weights))
        scored.sort(key=lambda item: item[0], reverse=True)
        best_score, best = scored[0]
        # 写进weights.txt 和 training_log.json
        history.append({"generation": gen, "score": best_score, "weights": best})
        write_outputs(best, history, out_dir)
        print(f"generation {gen:03d}: score={best_score:4d}, weights={best}", flush=True)

    return best, history


def write_outputs(best, history, out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "weights.txt").write_text(
        "int weights[FEATURE_COUNT] = {\n\t"
        + ", ".join(str(x) for x in best)
        + "\n};\n",
        encoding="utf-8",
    )
    (out_dir / "training_log.json").write_text(
        json.dumps(history, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def main():
    global MAX_FORBIDDEN_DEPTH

    parser = argparse.ArgumentParser()
    parser.add_argument("--generations", type=int, default=20)
    parser.add_argument("--population", type=int, default=12)
    parser.add_argument("--games", type=int, default=4)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--out", type=Path, default=Path("."))
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--random-rate", type=float, default=0.03)
    parser.add_argument("--forbidden-depth", type=int, default=1)
    args = parser.parse_args()
    MAX_FORBIDDEN_DEPTH = max(0, args.forbidden_depth)

    best, history = train(
        args.generations,
        args.population,
        args.games,
        args.seed,
        args.out,
        resume=args.resume,
        random_rate=args.random_rate,
    )
    write_outputs(best, history, args.out)
    print("\nbest weights:")
    print(best)
    print("\nwritten: weights.txt, training_log.json")


if __name__ == "__main__":
    main()
