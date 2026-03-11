from rich import print

WID = 15
HEI = 19


def norm_pos(pos):
    y, x = pos
    y %= HEI
    x %= WID
    return y, x


class GameState:
    def __init__(
        self,
        board=None,
        current_player=0,
        legal_actions_O=set([(4, 10), (3, 11), (5, 11), (4, 12)]),
        legal_actions_X=set([(14, 2), (13, 3), (15, 3), (14, 4)]),
        state=(0, -1),
        last_action=None,
    ):
        if board == None:
            board = [None] * WID * HEI
            board[4 * WID + 11] = 0
            board[14 * WID + 3] = 1
        self.current_player = current_player
        self.board = board
        self.legal_actions_O = legal_actions_O
        self.legal_actions_X = legal_actions_X
        self.state = state
        self.last_action = last_action

    def get_legal_actions(self) -> list:
        if self.current_player:
            return list(self.legal_actions_X)
        return list(self.legal_actions_O)

    # TODO
    def apply_action(self, action) -> "GameState":
        directions = [(0, 1), (0, -1), (1, 0), (-1, 0)]
        board = self.board[:]
        board[action[0] * WID + action[1]] = self.current_player
        legal_actions_O = self.legal_actions_O.copy()
        legal_actions_X = self.legal_actions_X.copy()

        def cnt_suc(start_pos, direction, target):
            max_cnt = 0
            if direction[0]:
                max_cnt = HEI
            elif direction[1]:
                max_cnt = WID
            cnt = 0
            y, x = start_pos
            while board[y * WID + x] == target and cnt < max_cnt:
                cnt += 1
                y += direction[0]
                x += direction[1]
                y, x = norm_pos((y, x))
            return cnt

        def is_legal_pos(pos):
            y, x = pos
            if board[y * WID + x] != None:
                return (0, 0)
            O_legal = True
            X_legal = True
            for direction in directions:
                O_cnt = cnt_suc(
                    norm_pos((y + direction[0], x + direction[1])), direction, 0
                )
                X_cnt = cnt_suc(
                    norm_pos((y - direction[0], x - direction[1])),
                    (-direction[0], -direction[1]),
                    1,
                )
                if O_cnt + 1 < X_cnt:
                    O_legal = False
                elif O_cnt > X_cnt + 1:
                    X_legal = False
            return (O_legal, X_legal)

        def is_beside(pos):
            y, x = pos
            beside_O = False
            beside_X = False
            for direction in directions:
                y, x = pos
                y += direction[0]
                x += direction[1]
                y, x = norm_pos((y, x))
                if board[y * WID + x] == 0:
                    beside_O = True
                elif board[y * WID + x] == 1:
                    beside_X = True
            return (beside_O, beside_X)

        def is_legal_pos_strict(pos):
            beside_info = is_beside(pos)
            legal_info = is_legal_pos(pos)
            return (beside_info[0] and legal_info[0], beside_info[1] and legal_info[1])

        def update_legal_moves(pos):
            legal_state = is_legal_pos_strict(pos)
            if legal_state[0]:
                legal_actions_O.add(pos)
            else:
                legal_actions_O.discard(pos)
            if legal_state[1]:
                legal_actions_X.add(pos)
            else:
                legal_actions_X.discard(pos)

        def apply_remove(removed_pos, removed):
            board[removed_pos[0] * WID + removed_pos[1]] = None
            update_legal_moves(removed_pos)
            for direction in directions:
                y, x = removed_pos
                y += direction[0]
                x += direction[1]
                y, x = norm_pos((y, x))
                update_legal_moves((y, x))
                suc_cnt = cnt_suc((y, x), direction, removed)
                y += suc_cnt * direction[0]
                x += suc_cnt * direction[1]
                y, x = norm_pos((y, x))
                if board[y * WID + x] != None:
                    y, x = removed_pos
                    y += direction[0]
                    x += direction[1]
                    y, x = norm_pos((y, x))
                    for _ in range(suc_cnt):
                        apply_remove((y, x), removed)
                        y += direction[0]
                        x += direction[1]
                        y, x = norm_pos((y, x))

        for direction in directions:
            y, x = action
            suc_lenth = cnt_suc(action, direction, self.current_player)
            y += direction[0] * suc_lenth
            x += direction[1] * suc_lenth
            y, x = norm_pos((y, x))
            if board[y * WID + x] == 1 - self.current_player:
                while board[y * WID + x] == 1 - self.current_player:
                    apply_remove((y, x), 1 - self.current_player)
                    y += direction[0]
                    x += direction[1]
                    y, x = norm_pos((y, x))
            else:
                update_legal_moves((y, x))
        if board[4 * WID + 11] == None:
            return GameState(
                board, 1 - self.current_player, set(), set(), (1, 1), action
            )
        if board[14 * WID + 3] == None:
            return GameState(
                board, 1 - self.current_player, set(), set(), (1, 0), action
            )
        update_legal_moves(action)
        if (not legal_actions_O) or (not legal_actions_X):
            return GameState(
                board, 1 - self.current_player, set(), set(), (1, -1), action
            )
        return GameState(
            board,
            1 - self.current_player,
            legal_actions_O,
            legal_actions_X,
            (0, -1),
            action,
        )

    def get_current_player(self) -> int:
        return self.current_player

    def is_terminal(self) -> bool:
        return self.state[0]

    def get_reward(self, player_id: int) -> float:
        if self.state[1] == -1:
            return 0.0
        winner = self.state[1]
        return 1.0 if player_id == winner else -1.0

    def get_board(self):
        return self.board

    # TODO:
    # too ugly colors!
    def __repr__(self):
        for x in range(WID):
            print(chr(x + 65) + " ", end="")
        print()
        for y in range(HEI):
            for x in range(WID):
                if (y, x) == self.last_action:
                    print("[yellow]@ [/yellow]", end="")
                    continue
                if self.board[y * WID + x] == None:
                    legal_actions = (
                        self.legal_actions_X
                        if self.current_player
                        else self.legal_actions_O
                    )
                    if (y, x) in legal_actions:
                        print("+ ", end="")
                        continue
                    print(". ", end="")
                    continue
                if self.board[y * WID + x] == 0:
                    print("[red]O [red]", end="")
                    continue
                if self.board[y * WID + x] == 1:
                    print("[green]X [/green]", end="")
                    continue
            print(y + 1)
        print()
